
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_BASE_ENDPOINT_H_
#define TURBO_BASE_ENDPOINT_H_

#include <netinet/in.h>                          // in_addr
#include <sys/un.h>                              // sockaddr_un
#include <iostream>                              // std::ostream
#include "turbo/container/hash_tables.h"         // hashing functions

namespace turbo {

    // Type of an IP address
    typedef struct in_addr ip_t;

    static const ip_t IP_ANY = {INADDR_ANY};
    static const ip_t IP_NONE = {INADDR_NONE};

    // Convert |ip| to an integral
    inline in_addr_t ip2int(ip_t ip) { return ip.s_addr; }

    // Convert integral |ip_value| to an IP
    inline ip_t int2ip(in_addr_t ip_value) {
        const ip_t ip = {ip_value};
        return ip;
    }

    // Convert string `ip_str' to ip_t *ip.
    // `ip_str' is in IPv4 dotted-quad format: `127.0.0.1', `10.23.249.73' ...
    // Returns 0 on success, -1 otherwise.
    int str2ip(const char *ip_str, ip_t *ip);

    struct IPStr {
        const char *c_str() const { return _buf; }

        char _buf[INET_ADDRSTRLEN];
    };

    // Convert IP to c-style string. Notice that you can serialize ip_t to
    // std::ostream directly. Use this function when you don't have streaming log.
    // Example: printf("ip=%s\n", ip2str(some_ip).c_str());
    IPStr ip2str(ip_t ip);

    // Convert `hostname' to ip_t *ip. If `hostname' is nullptr, use hostname
    // of this machine.
    // `hostname' is typically in this form: `tc-cm-et21.tc' `db-cos-dev.db01' ...
    // Returns 0 on success, -1 otherwise.
    int hostname2ip(const char *hostname, ip_t *ip);

    // Convert `ip' to `hostname'.
    // Returns 0 on success, -1 otherwise and errno is set.
    int ip2hostname(ip_t ip, char *hostname, size_t hostname_len);

    int ip2hostname(ip_t ip, std::string *hostname);

    // Hostname of this machine, "" on error.
    // NOTE: This function caches result on first call.
    const char *my_hostname();

    // IP of this machine, IP_ANY on error.
    // NOTE: This function caches result on first call.
    ip_t my_ip();

    // String form.
    const char *my_ip_cstr();

    // For IPv4 endpoint, ip and port are real things.
    // For UDS/IPv6 endpoint, to keep ABI compatibility, ip is ResourceId, and port is a special flag.
    // See str2endpoint implementation for details.
    struct end_point {
        end_point() : ip(IP_ANY), port(0) {}

        end_point(ip_t ip2, int port2);

        explicit end_point(const sockaddr_in &in)
                : ip(in.sin_addr), port(ntohs(in.sin_port)) {}

        end_point(const end_point &);

        ~end_point();

        void operator=(const end_point &);

        void reset();

        ip_t ip;
        int port;
    };

    struct end_point_str {
        const char *c_str() const { return _buf; }

        char _buf[sizeof("unix:") + sizeof(sockaddr_un::sun_path)];
    };

    // Convert end_point to c-style string. Notice that you can serialize
    // end_point to std::ostream directly. Use this function when you don't
    // have streaming log.
    // Example: printf("point=%s\n", endpoint2str(point).c_str());
    end_point_str endpoint2str(const end_point &);

    // Convert string `ip_and_port_str' to a end_point *point.
    // Returns 0 on success, -1 otherwise.
    int str2endpoint(const char *ip_and_port_str, end_point *point);

    int str2endpoint(const char *ip_str, int port, end_point *point);

    // Convert `hostname_and_port_str' to a end_point *point.
    // Returns 0 on success, -1 otherwise.
    int hostname2endpoint(const char *ip_and_port_str, end_point *point);

    int hostname2endpoint(const char *name_str, int port, end_point *point);

    // Convert `endpoint' to `hostname'.
    // Returns 0 on success, -1 otherwise and errno is set.
    int endpoint2hostname(const end_point &point, char *hostname, size_t hostname_len);

    int endpoint2hostname(const end_point &point, std::string *host);

    // Create a TCP socket and connect it to `server'. Write port of this side
    // into `self_port' if it's not nullptr.
    // Returns the socket descriptor, -1 otherwise and errno is set.
    int tcp_connect(end_point server, int *self_port);

    // Create and listen to a TCP socket bound with `ip_and_port'.
    // To enable SO_REUSEADDR for the whole program, enable gflag -reuse_addr
    // To enable SO_REUSEPORT for the whole program, enable gflag -reuse_port
    // Returns the socket descriptor, -1 otherwise and errno is set.
    int tcp_listen(end_point ip_and_port);

    // Get the local end of a socket connection
    int get_local_side(int fd, end_point *out);

    // Get the other end of a socket connection
    int get_remote_side(int fd, end_point *out);

    // Get sockaddr from endpoint, return -1 on failed
    int endpoint2sockaddr(const end_point& point, struct sockaddr_storage* ss, socklen_t* size = NULL);

    // Create endpoint from sockaddr, return -1 on failed
    int sockaddr2endpoint(struct sockaddr_storage* ss, socklen_t size, end_point* point);

    // Get end_point type (AF_INET/AF_INET6/AF_UNIX)
    sa_family_t get_endpoint_type(const end_point& point);

    // Check if endpoint is extended.
    bool is_endpoint_extended(const end_point& point);

}  // namespace turbo

// Since ip_t is defined from in_addr which is globally defined, due to ADL
// we have to put overloaded operators globally as well.
inline bool operator<(turbo::ip_t lhs, turbo::ip_t rhs) {
    return turbo::ip2int(lhs) < turbo::ip2int(rhs);
}

inline bool operator>(turbo::ip_t lhs, turbo::ip_t rhs) {
    return rhs < lhs;
}

inline bool operator>=(turbo::ip_t lhs, turbo::ip_t rhs) {
    return !(lhs < rhs);
}

inline bool operator<=(turbo::ip_t lhs, turbo::ip_t rhs) {
    return !(rhs < lhs);
}

inline bool operator==(turbo::ip_t lhs, turbo::ip_t rhs) {
    return turbo::ip2int(lhs) == turbo::ip2int(rhs);
}

inline bool operator!=(turbo::ip_t lhs, turbo::ip_t rhs) {
    return !(lhs == rhs);
}

inline std::ostream &operator<<(std::ostream &os, const turbo::IPStr &ip_str) {
    return os << ip_str.c_str();
}

inline std::ostream &operator<<(std::ostream &os, turbo::ip_t ip) {
    return os << turbo::ip2str(ip);
}

namespace turbo {
    // Overload operators for end_point in the same namespace due to ADL.
    inline bool operator<(end_point p1, end_point p2) {
        return (p1.ip != p2.ip) ? (p1.ip < p2.ip) : (p1.port < p2.port);
    }

    inline bool operator>(end_point p1, end_point p2) {
        return p2 < p1;
    }

    inline bool operator<=(end_point p1, end_point p2) {
        return !(p2 < p1);
    }

    inline bool operator>=(end_point p1, end_point p2) {
        return !(p1 < p2);
    }

    inline bool operator==(end_point p1, end_point p2) {
        return p1.ip == p2.ip && p1.port == p2.port;
    }

    inline bool operator!=(end_point p1, end_point p2) {
        return !(p1 == p2);
    }

    inline std::ostream &operator<<(std::ostream &os, const end_point &ep) {
        return os << endpoint2str(ep).c_str();
    }

    inline std::ostream &operator<<(std::ostream &os, const end_point_str &ep_str) {
        return os << ep_str.c_str();
    }

}  // namespace turbo


namespace TURBO_HASH_NAMESPACE {

// Implement methods for hashing a pair of integers, so they can be used as
// keys in STL containers.

#if defined(TURBO_COMPILER_MSVC)

    inline std::size_t hash_value(const turbo::end_point& ep) {
        return turbo::container::hash_pair(turbo::ip2int(ep.ip), ep.port);
    }

#elif defined(TURBO_COMPILER_GNUC) || defined(TURBO_COMPILER_CLANG)

    template<>
    struct hash<turbo::end_point> {
        std::size_t operator()(const turbo::end_point &ep) const {
            return turbo::container::hash_pair(turbo::ip2int(ep.ip), ep.port);
        }
    };

#else
#error define hash<end_point> for your compiler
#endif  // COMPILER

}

#endif  // TURBO_BASE_ENDPOINT_H_
