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
//
// Created by jeff on 24-1-5.
//

#ifndef TURBO_SYSTEM_ENDPOINT_H_
#define TURBO_SYSTEM_ENDPOINT_H_

#include <netinet/in.h>                          // in_addr
#include <sys/un.h>                              // sockaddr_un
#include <iostream>                              // std::ostream

namespace turbo {


    struct ListenOption {
        bool reuse_addr{true};
        bool reuse_port{false};
        bool reuse_uds{false};
    };
    // Type of an IP address
    typedef struct in_addr ip_t;

    static constexpr ip_t IP_ANY = { INADDR_ANY };
    static constexpr ip_t IP_NONE = { INADDR_NONE };
    static constexpr int MAX_DOMAIN_LENGTH = 253;

    // Convert |ip| to an integral
    inline constexpr in_addr_t ip2int(ip_t ip) { return ip.s_addr; }

    // Convert integral |ip_value| to an IP
    inline constexpr ip_t int2ip(in_addr_t ip_value) {
        return ip_t{ ip_value };
    }

    // Convert string `ip_str' to ip_t *ip.
    // `ip_str' is in IPv4 dotted-quad format: `127.0.0.1', `10.23.249.73' ...
    // Returns 0 on success, -1 otherwise.
    int str2ip(const char* ip_str, ip_t* ip);

    struct IPStr {
        const char* c_str() const { return _buf; }
        char _buf[INET_ADDRSTRLEN];
    };

    // Convert IP to c-style string. Notice that you can serialize ip_t to
    // std::ostream directly. Use this function when you don't have streaming log.
    // Example: printf("ip=%s\n", ip2str(some_ip).c_str());
    IPStr ip2str(ip_t ip);

    // Convert `hostname' to ip_t *ip. If `hostname' is NULL, use hostname
    // of this machine.
    // `hostname' is typically in this form: `tc-cm-et21.tc' `db-cos-dev.db01' ...
    // Returns 0 on success, -1 otherwise.
    int hostname2ip(const char* hostname, ip_t* ip);

    // Convert `ip' to `hostname'.
    // Returns 0 on success, -1 otherwise and errno is set.
    int ip2hostname(ip_t ip, char* hostname, size_t hostname_len);
    int ip2hostname(ip_t ip, std::string* hostname);

    // Hostname of this machine, "" on error.
    // NOTE: This function caches result on first call.
    const char* my_hostname();

    // IP of this machine, IP_ANY on error.
    // NOTE: This function caches result on first call.
    ip_t my_ip();
    // String form.
    const char* my_ip_cstr();

    // For IPv4 endpoint, ip and port are real things.
    // For UDS/IPv6 endpoint, to keep ABI compatibility, ip is ResourceId, and port is a special flag.
    // See str2endpoint implementation for details.
    struct EndPoint {
        EndPoint() : ip(IP_ANY), port(0) {}
        EndPoint(ip_t ip2, int port2);
        explicit EndPoint(const sockaddr_in& in)
                : ip(in.sin_addr), port(ntohs(in.sin_port)) {}

        EndPoint(const EndPoint&);
        ~EndPoint();
        void operator=(const EndPoint&);

        void reset(void);

        ip_t ip;
        int port;
    };

    struct EndPointStr {
        const char* c_str() const { return _buf; }
        char _buf[sizeof("unix:") + sizeof(sockaddr_un::sun_path)];
    };

    // Convert EndPoint to c-style string. Notice that you can serialize
    // EndPoint to std::ostream directly. Use this function when you don't
    // have streaming log.
    // Example: printf("point=%s\n", endpoint2str(point).c_str());
    EndPointStr endpoint2str(const EndPoint&);

    // Convert string `ip_and_port_str' to a EndPoint *point.
    // Returns 0 on success, -1 otherwise.
    int str2endpoint(const char* ip_and_port_str, EndPoint* point);
    int str2endpoint(const char* ip_str, int port, EndPoint* point);

    // Convert `hostname_and_port_str' to a EndPoint *point.
    // Returns 0 on success, -1 otherwise.
    int hostname2endpoint(const char* ip_and_port_str, EndPoint* point);
    int hostname2endpoint(const char* name_str, int port, EndPoint* point);

    // Convert `endpoint' to `hostname'.
    // Returns 0 on success, -1 otherwise and errno is set.
    int endpoint2hostname(const EndPoint& point, char* hostname, size_t hostname_len);
    int endpoint2hostname(const EndPoint& point, std::string* host);

    // Create a TCP socket and connect it to `server'. Write port of this side
    // into `self_port' if it's not NULL.
    // Returns the socket descriptor, -1 otherwise and errno is set.
    int tcp_connect(EndPoint server, int* self_port);

    // Create and listen to a TCP socket bound with `ip_and_port'.
    // To enable SO_REUSEADDR for the whole program, enable gflag -reuse_addr
    // To enable SO_REUSEPORT for the whole program, enable gflag -reuse_port
    // Returns the socket descriptor, -1 otherwise and errno is set.
    int tcp_listen(EndPoint ip_and_port, ListenOption option = ListenOption());

    // Get the local end of a socket connection
    int get_local_side(int fd, EndPoint *out);

    // Get the other end of a socket connection
    int get_remote_side(int fd, EndPoint *out);

    // Get sockaddr from endpoint, return -1 on failed
    int endpoint2sockaddr(const EndPoint& point, struct sockaddr_storage* ss, socklen_t* size = NULL);

    // Create endpoint from sockaddr, return -1 on failed
    int sockaddr2endpoint(struct sockaddr_storage* ss, socklen_t size, EndPoint* point);

    // Get EndPoint type (AF_INET/AF_INET6/AF_UNIX)
    sa_family_t get_endpoint_type(const EndPoint& point);

    // Check if endpoint is extended.
    bool is_endpoint_extended(const EndPoint& point);

    template <typename H>
    inline H hash_value(H h, const EndPoint& point) {
        return H::combine(std::move(h), point.ip.s_addr, point.port);
    }
}  // namespace turbo

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

inline std::ostream& operator<<(std::ostream& os, const turbo::IPStr& ip_str) {
    return os << ip_str.c_str();
}
inline std::ostream& operator<<(std::ostream& os, turbo::ip_t ip) {
    return os << turbo::ip2str(ip);
}

namespace turbo {
    inline bool operator<(EndPoint p1, EndPoint p2) {
        return (p1.ip != p2.ip) ? (p1.ip < p2.ip) : (p1.port < p2.port);
    }
    inline bool operator>(EndPoint p1, EndPoint p2) {
        return p2 < p1;
    }
    inline bool operator<=(EndPoint p1, EndPoint p2) {
        return !(p2 < p1);
    }
    inline bool operator>=(EndPoint p1, EndPoint p2) {
        return !(p1 < p2);
    }
    inline bool operator==(EndPoint p1, EndPoint p2) {
        return p1.ip == p2.ip && p1.port == p2.port;
    }
    inline bool operator!=(EndPoint p1, EndPoint p2) {
        return !(p1 == p2);
    }

    inline std::ostream& operator<<(std::ostream& os, const EndPoint& ep) {
        return os << endpoint2str(ep).c_str();
    }
    inline std::ostream& operator<<(std::ostream& os, const EndPointStr& ep_str) {
        return os << ep_str.c_str();
    }
}  // namespace turbo

#endif  // TURBO_SYSTEM_ENDPOINT_H_
