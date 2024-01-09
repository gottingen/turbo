// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "turbo/base/endpoint.h"
#include "turbo/files/io.h"
#include "turbo/base/internal/endpoint_internal.h"

namespace turbo {
    int TURBO_WEAK fiber_connect(
            int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen) {
        return connect(sockfd, serv_addr, addrlen);
    }
        
    using turbo::base_internal::ExtendedEndPoint;

    static void set_endpoint(EndPoint *ep, ip_t ip, int port) {
        ep->ip = ip;
        ep->port = port;
        if (ExtendedEndPoint::is_extended(*ep)) {
            ExtendedEndPoint *eep = ExtendedEndPoint::address(*ep);
            if (eep) {
                eep->inc_ref();
            } else {
                ep->ip = IP_ANY;
                ep->port = 0;
            }
        }
    }

    void EndPoint::reset(void) {
        if (ExtendedEndPoint::is_extended(*this)) {
            ExtendedEndPoint *eep = ExtendedEndPoint::address(*this);
            if (eep) {
                eep->dec_ref();
            }
        }
        ip = IP_ANY;
        port = 0;
    }

    EndPoint::EndPoint(ip_t ip2, int port2) : ip(ip2), port(port2) {
        // Should never construct an extended endpoint by this way
        if (ExtendedEndPoint::is_extended(*this)) {
            TLOG_CHECK(0, "EndPoint construct with value that points to an extended EndPoint");
            ip = IP_ANY;
            port = 0;
        }
    }

    EndPoint::EndPoint(const EndPoint &rhs) {
        set_endpoint(this, rhs.ip, rhs.port);
    }

    EndPoint::~EndPoint() {
        reset();
    }

    void EndPoint::operator=(const EndPoint &rhs) {
        reset();
        set_endpoint(this, rhs.ip, rhs.port);
    }

    int str2ip(const char *ip_str, ip_t *ip) {
        // ip_str can be nullptr when called by EndPoint(0, ...)
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

    EndPointStr endpoint2str(const EndPoint &point) {
        EndPointStr str;
        if (ExtendedEndPoint::is_extended(point)) {
            ExtendedEndPoint *eep = ExtendedEndPoint::address(point);
            if (eep) {
                eep->to(&str);
            } else {
                str._buf[0] = '\0';
            }
            return str;
        }
        if (inet_ntop(AF_INET, &point.ip, str._buf, INET_ADDRSTRLEN) == nullptr) {
            return endpoint2str(EndPoint(IP_NONE, 0));
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

#if defined(OS_MACOSX)
        // gethostbyname on MAC is thread-safe (with current usage) since the
    // returned hostent is TLS. Check following link for the ref:
    // https://lists.apple.com/archives/darwin-dev/2006/May/msg00008.html
    struct hostent* result = gethostbyname(hostname);
    if (result == nullptr) {
        return -1;
    }
#else
        int aux_buf_len = 1024;
        std::unique_ptr<char[]> aux_buf(new char[aux_buf_len]);
        int ret = 0;
        int error = 0;
        struct hostent ent;
        struct hostent *result = nullptr;
        do {
            result = nullptr;
            error = 0;
            ret = gethostbyname_r(hostname,
                                  &ent,
                                  aux_buf.get(),
                                  aux_buf_len,
                                  &result,
                                  &error);
            if (ret != ERANGE) { // aux_buf is not long enough
                break;
            }
            aux_buf_len *= 2;
            aux_buf.reset(new char[aux_buf_len]);
        } while (1);
        if (ret != 0 || result == nullptr) {
            return -1;
        }
#endif // defined(OS_MACOSX)
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

        static MyAddressInfo *get_instance() {
            static MyAddressInfo my_addr;
            return &my_addr;
        }

    };


    ip_t my_ip() {
        return MyAddressInfo::get_instance()->my_ip;
    }

    const char *my_ip_cstr() {
        return MyAddressInfo::get_instance()->my_ip_str.c_str();
    }

    const char *my_hostname() {
        return MyAddressInfo::get_instance()->my_hostname;
    }

    int str2endpoint(const char *str, EndPoint *point) {
        if (ExtendedEndPoint::create(str, point)) {
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

    int str2endpoint(const char *ip_str, int port, EndPoint *point) {
        if (ExtendedEndPoint::create(ip_str, port, point)) {
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

    int hostname2endpoint(const char *str, EndPoint *point) {
        // Should be enough to hold ip address
        // The definitive descriptions of the rules for forming domain names appear in RFC 1035, RFC 1123, RFC 2181,
        // and RFC 5892. The full domain name may not exceed the length of 253 characters in its textual representation
        // (Domain Names - Domain Concepts and Facilities. IETF. doi:10.17487/RFC1034. RFC 1034.).
        // For cacheline optimize, use buf size as 256;
        char buf[256];
        size_t i = 0;
        for (; i < MAX_DOMAIN_LENGTH && str[i] != '\0' && str[i] != ':'; ++i) {
            buf[i] = str[i];
        }

        if (i >= MAX_DOMAIN_LENGTH || str[i] != ':') {
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

    int hostname2endpoint(const char *name_str, int port, EndPoint *point) {
        if (hostname2ip(name_str, &point->ip) != 0) {
            return -1;
        }
        if (port < 0 || port > 65535) {
            return -1;
        }
        point->port = port;
        return 0;
    }

    int endpoint2hostname(const EndPoint &point, char *host, size_t host_len) {
        if (ExtendedEndPoint::is_extended(point)) {
            ExtendedEndPoint *eep = ExtendedEndPoint::address(point);
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

    int endpoint2hostname(const EndPoint &point, std::string *host) {
        char buf[256];
        if (endpoint2hostname(point, buf, sizeof(buf)) == 0) {
            host->assign(buf);
            return 0;
        }
        return -1;
    }

    int tcp_connect(EndPoint point, int *self_port) {
        struct sockaddr_storage serv_addr;
        socklen_t serv_addr_size = 0;
        if (endpoint2sockaddr(point, &serv_addr, &serv_addr_size) != 0) {
            return -1;
        }
        FDGuard sockfd(socket(serv_addr.ss_family, SOCK_STREAM, 0));
        if (sockfd < 0) {
            return -1;
        }
        int rc = 0;
        if (fiber_connect != nullptr) {
            rc = fiber_connect(sockfd, (struct sockaddr *) &serv_addr, serv_addr_size);
        } else {
            rc = ::connect(sockfd, (struct sockaddr *) &serv_addr, serv_addr_size);
        }
        if (rc < 0) {
            return -1;
        }
        if (self_port != nullptr) {
            EndPoint pt;
            if (get_local_side(sockfd, &pt) == 0) {
                *self_port = pt.port;
            } else {
                TLOG_CHECK(false, "Fail to get the local port of sockfd={}", (int)sockfd);
            }
        }
        return sockfd.release();
    }

    int tcp_listen(EndPoint point,  ListenOption option) {
        struct sockaddr_storage serv_addr;
        socklen_t serv_addr_size = 0;
        if (endpoint2sockaddr(point, &serv_addr, &serv_addr_size) != 0) {
            return -1;
        }
        FDGuard sockfd(socket(serv_addr.ss_family, SOCK_STREAM, 0));
        if (sockfd < 0) {
            return -1;
        }

        if (option.reuse_addr) {
#if defined(SO_REUSEADDR)
            const int on = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                           &on, sizeof(on)) != 0) {
                return -1;
            }
#else
            LOG(ERROR) << "Missing def of SO_REUSEADDR while -reuse_addr is on";
        return -1;
#endif
        }

        if (option.reuse_port) {
#if defined(SO_REUSEPORT)
            const int on = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT,
                           &on, sizeof(on)) != 0) {
                TLOG_WARN("Fail to setsockopt SO_REUSEPORT of sockfd={}", (int)sockfd);
            }
#else
            LOG(ERROR) << "Missing def of SO_REUSEPORT while -reuse_port is on";
        return -1;
#endif
        }

        if (option.reuse_uds && serv_addr.ss_family == AF_UNIX) {
            ::unlink(((sockaddr_un *) &serv_addr)->sun_path);
        }

        if (bind(sockfd, (struct sockaddr *) &serv_addr, serv_addr_size) != 0) {
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

    int get_local_side(int fd, EndPoint *out) {
        struct sockaddr_storage addr;
        socklen_t socklen = sizeof(addr);
        const int rc = getsockname(fd, (struct sockaddr *) &addr, &socklen);
        if (rc != 0) {
            return rc;
        }
        if (out) {
            return sockaddr2endpoint(&addr, socklen, out);
        }
        return 0;
    }

    int get_remote_side(int fd, EndPoint *out) {
        struct sockaddr_storage addr;
        bzero(&addr, sizeof(addr));
        socklen_t socklen = sizeof(addr);
        const int rc = getpeername(fd, (struct sockaddr *) &addr, &socklen);
        if (rc != 0) {
            return rc;
        }
        if (out) {
            return sockaddr2endpoint(&addr, socklen, out);
        }
        return 0;
    }

    int endpoint2sockaddr(const EndPoint &point, struct sockaddr_storage *ss, socklen_t *size) {
        bzero(ss, sizeof(*ss));
        if (ExtendedEndPoint::is_extended(point)) {
            ExtendedEndPoint *eep = ExtendedEndPoint::address(point);
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
        struct sockaddr_in *in4 = (struct sockaddr_in *) ss;
        in4->sin_family = AF_INET;
        in4->sin_addr = point.ip;
        in4->sin_port = htons(point.port);
        if (size) {
            *size = sizeof(*in4);
        }
        return 0;
    }

    int sockaddr2endpoint(struct sockaddr_storage *ss, socklen_t size, EndPoint *point) {
        if (ss->ss_family == AF_INET) {
            *point = EndPoint(*(sockaddr_in *) ss);
            return 0;
        }
        if (ExtendedEndPoint::create(ss, size, point)) {
            return 0;
        }
        return -1;
    }

    sa_family_t get_endpoint_type(const EndPoint &point) {
        if (ExtendedEndPoint::is_extended(point)) {
            ExtendedEndPoint *eep = ExtendedEndPoint::address(point);
            if (eep) {
                return eep->family();
            }
            return AF_UNSPEC;
        }
        return AF_INET;
    }

    bool is_endpoint_extended(const EndPoint &point) {
        return ExtendedEndPoint::is_extended(point);
    }


}  // namespace turbo
