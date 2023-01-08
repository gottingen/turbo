
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/base/profile.h"
#include <arpa/inet.h>                         // inet_pton, inet_ntop
#include <netdb.h>                             // gethostbyname_r
#include <unistd.h>                            // gethostname
#include <errno.h>                             // errno
#include <string.h>                            // strcpy
#include <stdio.h>                             // snprintf
#include <cstdlib>                            // strtol
#include <gflags/gflags.h>
#include "flare/base/fd_guard.h"                    // fd_guard
#include "flare/base/endpoint.h"                    // ip_t
#include "flare/log/logging.h"
#include "flare/strings/str_format.h"
#include "flare/strings/ends_with.h"
#include "flare/base/endpoint_extended.h"
#include "flare/base/singleton_on_pthread_once.h"
#include <sys/socket.h>                        // SO_REUSEADDR SO_REUSEPORT

//supported since Linux 3.9.
DEFINE_bool(reuse_port, false, "Enable SO_REUSEPORT for all listened sockets");

DEFINE_bool(reuse_addr, true, "Enable SO_REUSEADDR for all listened sockets");

DEFINE_bool(reuse_uds_path, false, "remove unix domain socket file before listen to it");

__BEGIN_DECLS
int FLARE_WEAK fiber_connect(
        int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen) {
    return connect(sockfd, serv_addr, addrlen);
}
__END_DECLS

namespace flare {

    using detail::extended_end_point;

    static void set_endpoint(end_point *ep, ip_t ip, int port) {
        ep->ip = ip;
        ep->port = port;
        if (extended_end_point::is_extended(*ep)) {
            extended_end_point *eep = extended_end_point::address(*ep);
            if (eep) {
                eep->inc_ref();
            } else {
                ep->ip = IP_ANY;
                ep->port = 0;
            }
        }
    }

    void end_point::reset(void) {
        if (extended_end_point::is_extended(*this)) {
            extended_end_point *eep = extended_end_point::address(*this);
            if (eep) {
                eep->dec_ref();
            }
        }
        ip = IP_ANY;
        port = 0;
    }


    end_point::end_point(ip_t ip2, int port2) : ip(ip2), port(port2) {
        // Should never construct an extended endpoint by this way
        if (extended_end_point::is_extended(*this)) {
            FLARE_CHECK(0) << "EndPoint construct with value that points to an extended EndPoint";
            ip = IP_ANY;
            port = 0;
        }
    }

    end_point::end_point(const end_point &rhs) {
        set_endpoint(this, rhs.ip, rhs.port);
    }

    end_point::~end_point() {
        reset();
    }

    void end_point::operator=(const end_point &rhs) {
        reset();
        set_endpoint(this, rhs.ip, rhs.port);
    }

    int str2ip(const char *ip_str, ip_t *ip) {
        // ip_str can be nullptr when called by end_point(0, ...)
        if (ip_str != nullptr) {
            for (; isspace(*ip_str); ++ip_str);
            int rc = inet_pton(AF_INET, ip_str, ip);
            if (rc > 0) {
                return 0;
            }
        }
        return -1;
    }

    IPStr ip2str(ip_t ip) {
        IPStr str;
        if (inet_ntop(AF_INET, &ip, str._buf, INET_ADDRSTRLEN) == nullptr) {
            return ip2str(IP_NONE);
        }
        return str;
    }

    int ip2hostname(ip_t ip, char *host, size_t host_len) {
        if (host == nullptr || host_len == 0) {
            errno = EINVAL;
            return -1;
        }
        sockaddr_in sa;
        bzero((char *) &sa, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port = 0;    // useless since we don't need server_name
        sa.sin_addr = ip;
        if (getnameinfo((const sockaddr *) &sa, sizeof(sa),
                        host, host_len, nullptr, 0, NI_NAMEREQD) != 0) {
            return -1;
        }
        // remove baidu-specific domain name (that every name has)
        std::string_view str(host);
        if (flare::ends_with(host, ".baidu.com")) {
            host[str.size() - 10] = '\0';
        }
        return 0;
    }

    int ip2hostname(ip_t ip, std::string *host) {
        char buf[128];
        if (ip2hostname(ip, buf, sizeof(buf)) == 0) {
            host->assign(buf);
            return 0;
        }
        return -1;
    }

    end_point_str endpoint2str(const end_point &point) {
        end_point_str str;
        if (extended_end_point::is_extended(point)) {
            extended_end_point *eep = extended_end_point::address(point);
            if (eep) {
                eep->to(&str);
            } else {
                str._buf[0] = '\0';
            }
            return str;
        }
        if (inet_ntop(AF_INET, &point.ip, str._buf, INET_ADDRSTRLEN) == nullptr) {
            return endpoint2str(end_point(IP_NONE, 0));
        }
        char *buf = str._buf + strlen(str._buf);
        *buf++ = ':';
        snprintf(buf, 16, "%d", point.port);
        return str;
    }

    int hostname2ip(const char *hostname, ip_t *ip) {
        char buf[256];
        if (nullptr == hostname) {
            if (gethostname(buf, sizeof(buf)) < 0) {
                return -1;
            }
            hostname = buf;
        } else {
            // skip heading space
            for (; isspace(*hostname); ++hostname);
        }

#if defined(FLARE_PLATFORM_APPLE)
        // gethostbyname on MAC is thread-safe (with current usage) since the
        // returned hostent is TLS. Check following link for the ref:
        // https://lists.apple.com/archives/darwin-dev/2006/May/msg00008.html
        struct hostent *result = gethostbyname(hostname);
        if (result == nullptr) {
            return -1;
        }
#else
        char aux_buf[1024];
        int error = 0;
        struct hostent ent;
        struct hostent* result = nullptr;
        if (gethostbyname_r(hostname, &ent, aux_buf, sizeof(aux_buf),
                            &result, &error) != 0 || result == nullptr) {
            return -1;
        }
#endif // defined(FLARE_PLATFORM_APPLE)
        // Only fetch the first address here
        bcopy((char *) result->h_addr, (char *) ip, result->h_length);
        return 0;
    }

    struct MyAddressInfo {
        char my_hostname[256];
        ip_t my_ip;
        IPStr my_ip_str;

        MyAddressInfo() {
            my_ip = IP_ANY;
            if (gethostname(my_hostname, sizeof(my_hostname)) < 0) {
                my_hostname[0] = '\0';
            } else if (hostname2ip(my_hostname, &my_ip) != 0) {
                my_ip = IP_ANY;
            }
            my_ip_str = ip2str(my_ip);
        }
    };

    ip_t my_ip() {
        return get_leaky_singleton<MyAddressInfo>()->my_ip;
    }

    const char *my_ip_cstr() {
        return get_leaky_singleton<MyAddressInfo>()->my_ip_str.c_str();
    }

    const char *my_hostname() {
        return get_leaky_singleton<MyAddressInfo>()->my_hostname;
    }

    int str2endpoint(const char *str, end_point *point) {
        if (extended_end_point::create(str, point)) {
            return 0;
        }
        // Should be enough to hold ip address
        char buf[64];
        size_t i = 0;
        for (; i < sizeof(buf) && str[i] != '\0' && str[i] != ':'; ++i) {
            buf[i] = str[i];
        }
        if (i >= sizeof(buf) || str[i] != ':') {
            return -1;
        }
        buf[i] = '\0';
        if (str2ip(buf, &point->ip) != 0) {
            return -1;
        }
        ++i;
        char *end = nullptr;
        point->port = strtol(str + i, &end, 10);
        if (end == str + i) {
            return -1;
        } else if (*end) {
            for (++end; isspace(*end); ++end);
            if (*end) {
                return -1;
            }
        }
        if (point->port < 0 || point->port > 65535) {
            return -1;
        }
        return 0;
    }

    int str2endpoint(const char *ip_str, int port, end_point *point) {
        if (extended_end_point::create(ip_str, port, point)) {
            return 0;
        }
        if (str2ip(ip_str, &point->ip) != 0) {
            return -1;
        }
        if (port < 0 || port > 65535) {
            return -1;
        }
        point->port = port;
        return 0;
    }

    int hostname2endpoint(const char *str, end_point *point) {
        // Should be enough to hold ip address
        char buf[64];
        size_t i = 0;
        for (; i < sizeof(buf) - 1 && str[i] != '\0' && str[i] != ':'; ++i) {
            buf[i] = str[i];
        }
        if (i == sizeof(buf) - 1) {
            return -1;
        }

        buf[i] = '\0';
        if (hostname2ip(buf, &point->ip) != 0) {
            return -1;
        }
        if (str[i] == ':') {
            ++i;
        }
        char *end = nullptr;
        point->port = strtol(str + i, &end, 10);
        if (end == str + i) {
            return -1;
        } else if (*end) {
            for (; isspace(*end); ++end);
            if (*end) {
                return -1;
            }
        }
        if (point->port < 0 || point->port > 65535) {
            return -1;
        }
        return 0;
    }

    int hostname2endpoint(const char *name_str, int port, end_point *point) {
        if (hostname2ip(name_str, &point->ip) != 0) {
            return -1;
        }
        if (port < 0 || port > 65535) {
            return -1;
        }
        point->port = port;
        return 0;
    }

    int endpoint2hostname(const end_point &point, char *host, size_t host_len) {
        if (extended_end_point::is_extended(point)) {
            extended_end_point* eep = extended_end_point::address(point);
            if (eep) {
                return eep->to_hostname(host, host_len);
            }
            return -1;
        }
        if (ip2hostname(point.ip, host, host_len) == 0) {
            size_t len = strlen(host);
            if (len + 1 < host_len) {
                snprintf(host + len, host_len - len, ":%d", point.port);
            }
            return 0;
        }
        return -1;
    }

    int endpoint2hostname(const end_point &point, std::string *host) {
        char buf[256];
        if (endpoint2hostname(point, buf, sizeof(buf)) == 0) {
            host->assign(buf);
            return 0;
        }
        return -1;
    }

    int tcp_connect(end_point point, int *self_port) {
        struct sockaddr_storage serv_addr;
        socklen_t serv_addr_size = 0;
        if (endpoint2sockaddr(point, &serv_addr, &serv_addr_size) != 0) {
            return -1;
        }
        flare::base::fd_guard sockfd(socket(serv_addr.ss_family, SOCK_STREAM, 0));
        if (sockfd < 0) {
            return -1;
        }

        int rc = 0;
        if (fiber_connect != nullptr) {
            rc = fiber_connect(sockfd, (struct sockaddr*) &serv_addr, serv_addr_size);
        } else {
            rc = ::connect(sockfd, (struct sockaddr*) &serv_addr, serv_addr_size);
        }
        if (rc < 0) {
            return -1;
        }
        if (self_port != nullptr) {
            end_point pt;
            if (get_local_side(sockfd, &pt) == 0) {
                *self_port = pt.port;
            } else {
                FLARE_CHECK(false) << "Fail to get the local port of sockfd=" << sockfd;
            }
        }
        return sockfd.release();
    }

    int tcp_listen(end_point point) {
        struct sockaddr_storage serv_addr;
        socklen_t serv_addr_size = 0;
        if (endpoint2sockaddr(point, &serv_addr, &serv_addr_size) != 0) {
            return -1;
        }
        flare::base::fd_guard sockfd(socket(serv_addr.ss_family, SOCK_STREAM, 0));
        if (sockfd < 0) {
            return -1;
        }

        if (FLAGS_reuse_addr) {
#if defined(SO_REUSEADDR)
            const int on = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                           &on, sizeof(on)) != 0) {
                return -1;
            }
#else
            FLARE_LOG(ERROR) << "Missing def of SO_REUSEADDR while -reuse_addr is on";
            return -1;
#endif
        }

        if (FLAGS_reuse_port) {
#if defined(SO_REUSEPORT)
            const int on = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT,
                           &on, sizeof(on)) != 0) {
                FLARE_LOG(WARNING) << "Fail to setsockopt SO_REUSEPORT of sockfd=" << sockfd;
            }
#else
            FLARE_LOG(ERROR) << "Missing def of SO_REUSEPORT while -reuse_port is on";
            return -1;
#endif
        }

        if (FLAGS_reuse_uds_path && serv_addr.ss_family == AF_UNIX) {
            ::unlink(((sockaddr_un*) &serv_addr)->sun_path);
        }

        if (bind(sockfd, (struct sockaddr*)& serv_addr, serv_addr_size) != 0) {
            return -1;
        }
        if (listen(sockfd, 65535) != 0) {
            //             ^^^ kernel would silently truncate backlog to the value
            //             defined in /proc/sys/net/core/somaxconn if it is less
            //             than 65535
            return -1;
        }
        return sockfd.release();
    }

    int get_local_side(int fd, end_point *out) {
        struct sockaddr_storage addr;
        socklen_t socklen = sizeof(addr);
        const int rc = getsockname(fd, (struct sockaddr*)&addr, &socklen);
        if (rc != 0) {
            return rc;
        }
        if (out) {
            return sockaddr2endpoint(&addr, socklen, out);
        }
        return 0;
    }

    int get_remote_side(int fd, end_point *out) {
        struct sockaddr_storage addr;
        bzero(&addr, sizeof(addr));
        socklen_t socklen = sizeof(addr);
        const int rc = getpeername(fd, (struct sockaddr*)&addr, &socklen);
        if (rc != 0) {
            return rc;
        }
        if (out) {
            return sockaddr2endpoint(&addr, socklen, out);
        }
        return 0;
    }

    int endpoint2sockaddr(const end_point& point, struct sockaddr_storage* ss, socklen_t* size) {
        bzero(ss, sizeof(*ss));
        if (extended_end_point::is_extended(point)) {
            extended_end_point* eep = extended_end_point::address(point);
            if (!eep) {
                return -1;
            }
            int ret = eep->to(ss);
            if (ret < 0) {
                return -1;
            }
            if (size) {
                *size = static_cast<socklen_t>(ret);
            }
            return 0;
        }
        struct sockaddr_in* in4 = (struct sockaddr_in*) ss;
        in4->sin_family = AF_INET;
        in4->sin_addr = point.ip;
        in4->sin_port = htons(point.port);
        if (size) {
            *size = sizeof(*in4);
        }
        return 0;
    }


    int sockaddr2endpoint(struct sockaddr_storage* ss, socklen_t size, end_point* point) {
        if (ss->ss_family == AF_INET) {
            *point = end_point(*(sockaddr_in*)ss);
            return 0;
        }
        if (extended_end_point::create(ss, size, point)) {
            return 0;
        }
        return -1;
    }

    sa_family_t get_endpoint_type(const end_point& point) {
        if (extended_end_point::is_extended(point)) {
            extended_end_point* eep = extended_end_point::address(point);
            if (eep) {
                return eep->family();
            }
            return AF_UNSPEC;
        }
        return AF_INET;
    }

    bool is_endpoint_extended(const end_point& point) {
        return extended_end_point::is_extended(point);
    }


}  // namespace flare
