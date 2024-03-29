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

#ifndef TURBO_SYSTEM_INTERNAL_ENDPOINT_H_
#define TURBO_SYSTEM_INTERNAL_ENDPOINT_H_

#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <mutex>
#include <unordered_set>
#include "turbo/system/endpoint.h"
#include "turbo/log/logging.h"
#include "turbo/memory/resource_pool.h"
#include "turbo/strings/str_trim.h"

namespace turbo::system_internal {

    // For ipv6/unix socket address.
    //
    // We have to keep turbo::EndPoint ABI compatible because it is used so widely, and the size of turbo::EndPoint is
    // too small to store more information such as ipv6 address.
    // We store enough information about endpoint in such tiny struct by putting real things in another big object
    // holding by ResourcePool. The EndPoint::ip saves ResourceId, while EndPoint::port denotes if the EndPoint object
    // is an old style ipv4 endpoint.

    // If EndPoint.port equals to this value, we should get the extended endpoint in resource pool.
    const static int EXTENDED_ENDPOINT_PORT = 123456789;

    class ExtendedEndPoint;

    // A global unordered set to dedup ExtendedEndPoint
    // ExtendedEndPoints which have same ipv6/unix socket address must have same id,
    // so that user can simply use the value of EndPoint for comparision.
    class GlobalEndPointSet {
    public:
        ExtendedEndPoint* insert(ExtendedEndPoint* p);

        void erase(ExtendedEndPoint* p);

        static GlobalEndPointSet* instance() {
            static GlobalEndPointSet global_set;
            return &global_set;
        }

    private:
        struct Hash {
            size_t operator()(ExtendedEndPoint* const& p) const;
        };

        struct Equals {
            bool operator()(ExtendedEndPoint* const& p1, ExtendedEndPoint* const& p2) const;
        };

        typedef std::unordered_set<ExtendedEndPoint*, Hash, Equals> SetType;
        SetType _set;
        std::mutex _mutex;
    };

    class ExtendedEndPoint {
    public:
        // Construct ExtendedEndPoint.
        // User should use create() functions to get ExtendedEndPoint instance.
        ExtendedEndPoint(void) {
            _ref_count.store(0, std::memory_order_relaxed);
            _u.sa.sa_family = AF_UNSPEC;
        }

    public:
        // Create ExtendedEndPoint.
        // If creation is successful, create()s will embed the ExtendedEndPoint instance in the given EndPoint*,
        // and return it as well. Or else, the given EndPoint* won't be touched.
        //
        // The format of the parameter is inspired by nginx.
        // Valid forms are:
        // - ipv6
        // without port: [2400:da00::3b0b]
        // with    port: [2400:da00::3b0b]:8080
        // - unix domain socket
        // abslute path : unix:/path/to/file.sock
        // relative path: unix:path/to/file.sock

        static ExtendedEndPoint* create(std::string_view sp, EndPoint* ep) {
            sp = turbo::trim_all(sp);
            if (sp.empty()) {
                return nullptr;
            }
            if (sp[0] == '[') {
                size_t colon_pos = sp.find(']');
                if (colon_pos == std::string_view::npos || colon_pos == 1 /* [] is invalid */ || ++colon_pos >= sp.size()) {
                    return nullptr;
                }
                std::string_view port_sp = sp.substr(colon_pos);
                if (port_sp.size() < 2 /* colon and at least one integer */ || port_sp[0] != ':') {
                    return nullptr;
                }
                port_sp.remove_prefix(1); // remove `:'
                if (port_sp.size() > 5) { // max 65535
                    return nullptr;
                }
                char buf[6];
                buf[port_sp.copy(buf, port_sp.size())] = '\0';
                char* end = nullptr;
                int port = ::strtol(buf, &end, 10 /* base */);
                if (end != buf + port_sp.size()) {
                    return nullptr;
                }
                return create(sp.substr(0, colon_pos), port, ep);
            } else if (starts_with(sp, "unix:")) {
                return create(sp, EXTENDED_ENDPOINT_PORT, ep);
            }
            return nullptr;
        }

        static ExtendedEndPoint* create(std::string_view sp, int port, EndPoint* ep) {
            sp = trim_all(sp);
            if (sp.empty()) {
                return nullptr;
            }
            ExtendedEndPoint* eep = nullptr;
            if (sp[0] == '[' && port >= 0 && port <= 65535) {
                if (sp.back() != ']' || sp.size() == 2 || sp.size() - 2 >= INET6_ADDRSTRLEN) {
                    return nullptr;
                }
                char buf[INET6_ADDRSTRLEN];
                buf[sp.copy(buf, sp.size() - 2 /* skip `[' and `]' */, 1 /* skip `[' */)] = '\0';

                in6_addr addr;
                if (inet_pton(AF_INET6, buf, &addr) != 1 /* succ */) {
                    return nullptr;
                }

                eep = new_extended_endpoint(AF_INET6);
                if (eep) {
                    eep->_u.in6.sin6_addr = addr;
                    eep->_u.in6.sin6_port = htons(port);
                    eep->_u.in6.sin6_flowinfo = 0u;
                    eep->_u.in6.sin6_scope_id = 0u;
                    eep->_socklen = sizeof(_u.in6);
#if defined(TURBO_PLATFORM_OSX)
                    eep->_u.in6.sin6_len = eep->_socklen;
#endif
                }
            } else if (starts_with(sp, "unix:")) { // ignore port
                sp.remove_prefix(5); // remove `unix:'
                if (sp.empty() || sp.size() >= UDS_PATH_SIZE) {
                    return nullptr;
                }
                eep = new_extended_endpoint(AF_UNIX);
                if (eep) {
                    int size = sp.copy(eep->_u.un.sun_path, sp.size());
                    eep->_u.un.sun_path[size] = '\0';
                    eep->_socklen = offsetof(sockaddr_un, sun_path) + size + 1;
#if defined(TURBO_PLATFORM_OSX)
                    eep->_u.un.sun_len = eep->_socklen;
#endif
                }
            }
            if (eep) {
                eep = dedup(eep);
                eep->embed_to(ep);
            }
            return eep;
        }

        static ExtendedEndPoint* create(sockaddr_storage* ss, socklen_t size, EndPoint* ep) {
            ExtendedEndPoint* eep = nullptr;
            if (ss->ss_family == AF_INET6 || ss->ss_family == AF_UNIX) {
                eep = new_extended_endpoint(ss->ss_family);
            }
            if (eep) {
                memcpy(&eep->_u.ss, ss, size);
                eep->_socklen = size;
                if (ss->ss_family == AF_UNIX && size == offsetof(sockaddr_un, sun_path)) {
                    // See unix(7): When the address of an unnamed socket is returned,
                    // its length is sizeof(sa_family_t), and sun_path should not be inspected.
                    eep->_u.un.sun_path[0] = '\0';
                }
                eep = dedup(eep);
                eep->embed_to(ep);
            }
            return eep;
        }

        // Get ExtendedEndPoint instance from EndPoint
        static ExtendedEndPoint* address(const EndPoint& ep) {
            if (!is_extended(ep)) {
                return nullptr;
            }
            turbo::ResourceId<ExtendedEndPoint> id;
            id.value = ep._ip.num();
            ExtendedEndPoint* eep = turbo::address_resource<ExtendedEndPoint>(id);
            TLOG_CHECK(eep, "fail to address ExtendedEndPoint from EndPoint");
            return eep;
        }

        // Check if an EndPoint has embedded ExtendedEndPoint
        static constexpr bool is_extended(const turbo::EndPoint& ep) {
            return ep._port == EXTENDED_ENDPOINT_PORT;
        }

    private:
        friend class GlobalEndPointSet;

        static GlobalEndPointSet* global_set() {
            return GlobalEndPointSet::instance();
        }

        static ExtendedEndPoint* new_extended_endpoint(sa_family_t family) {
            turbo::ResourceId<ExtendedEndPoint> id;
            ExtendedEndPoint* eep = ::turbo::get_resource(&id);
            if (eep) {
                int64_t old_ref = eep->_ref_count.load(std::memory_order_relaxed);
                TLOG_CHECK(old_ref == 0, "new ExtendedEndPoint has reference {}", old_ref);
                TLOG_CHECK(eep->_u.sa.sa_family == AF_UNSPEC, "new ExtendedEndPoint has family {} set", eep->_u.sa.sa_family);
                eep->_ref_count.store(1, std::memory_order_relaxed);
                eep->_id = id;
                eep->_u.sa.sa_family = family;
            }
            return eep;
        }

        void embed_to(EndPoint* ep) const {
            TLOG_CHECK(0 == _id.value >> 32,  "ResourceId beyond index");
            ep->reset();
            ep->_ip = ip_t{static_cast<uint32_t>(_id.value)};
            ep->_port = EXTENDED_ENDPOINT_PORT;
        }

        static ExtendedEndPoint* dedup(ExtendedEndPoint* eep) {
            eep->_hash = std::hash<std::string>()(std::string((const char*)&eep->_u, eep->_socklen));

            ExtendedEndPoint* first_eep = global_set()->insert(eep);
            if (first_eep != eep) {
                eep->_ref_count.store(0, std::memory_order_relaxed);
                eep->_u.sa.sa_family = AF_UNSPEC;
                ::turbo::return_resource(eep->_id);
            }
            return first_eep;
        }

    public:

        void dec_ref(void) {
            int64_t old_ref = _ref_count.fetch_sub(1, std::memory_order_relaxed);
            TLOG_CHECK(old_ref >= 1, "ExtendedEndPoint has unexpected reference {}", old_ref);
            if (old_ref == 1) {
                global_set()->erase(this);
                _u.sa.sa_family = AF_UNSPEC;
                ::turbo::return_resource(_id);
            }
        }

        void inc_ref(void) {
            int64_t old_ref = _ref_count.fetch_add(1, std::memory_order_relaxed);
            TLOG_CHECK(old_ref >= 1, "ExtendedEndPoint has unexpected reference {}", old_ref);
        }

        sa_family_t family(void) const {
            return _u.sa.sa_family;
        }

        int to(sockaddr_storage* ss) const {
            memcpy(ss, &_u.ss, _socklen);
            return _socklen;
        }

        void to(EndPointStr* ep_str) const {
            if (_u.sa.sa_family == AF_UNIX) {
                snprintf(ep_str->_buf, sizeof(ep_str->_buf), "unix:%s", _u.un.sun_path);
            } else if (_u.sa.sa_family == AF_INET6) {
                char buf[INET6_ADDRSTRLEN] = {0};
                const char* ret = inet_ntop(_u.sa.sa_family, &_u.in6.sin6_addr, buf, sizeof(buf));
                TLOG_CHECK(ret, "fail to do inet_ntop");
                snprintf(ep_str->_buf, sizeof(ep_str->_buf), "[%s]:%d", buf, ntohs(_u.in6.sin6_port));
            } else {
                TLOG_CHECK(0, "family {}  not supported", _u.sa.sa_family);
            }
        }

        int to_hostname(char* host, size_t host_len) const {
            if (_u.sa.sa_family == AF_UNIX) {
                snprintf(host, host_len, "unix:%s", _u.un.sun_path);
                return 0;
            } else if (_u.sa.sa_family == AF_INET6) {
                sockaddr_in6 sa = _u.in6;
                if (getnameinfo((const sockaddr*) &sa, sizeof(sa), host, host_len, nullptr, 0, NI_NAMEREQD) != 0) {
                    return -1;
                }
                size_t len = ::strlen(host);
                if (len + 1 < host_len) {
                    snprintf(host + len, host_len - len, ":%d", _u.in6.sin6_port);
                }
                return 0;
            } else {
                TLOG_CHECK(0, "family {}  not supported", _u.sa.sa_family);
                return -1;
            }
        }

    private:
        static const size_t UDS_PATH_SIZE = sizeof(sockaddr_un::sun_path);

        std::atomic<int64_t> _ref_count;
        turbo::ResourceId<ExtendedEndPoint> _id;
        size_t _hash;  // pre-compute hash code of sockaddr for saving unordered_set query time
        socklen_t _socklen; // valid data length of sockaddr
        union {
            sockaddr sa;
            sockaddr_in6 in6;
            sockaddr_un un;
            sockaddr_storage ss;
        } _u;
    };

    inline ExtendedEndPoint* GlobalEndPointSet::insert(ExtendedEndPoint* p) {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _set.find(p);
        if (it != _set.end()) {
            if ((*it)->_ref_count.fetch_add(1, std::memory_order_relaxed) == 0) {
                // another thread is calling dec_ref(), do not reuse it
                (*it)->_ref_count.fetch_sub(1, std::memory_order_relaxed);
                _set.erase(it);
                _set.insert(p);
                return p;
            } else {
                // the ExtendedEndPoint is valid, reuse it
                return *it;
            }
        }
        _set.insert(p);
        return p;
    }

    inline void GlobalEndPointSet::erase(ExtendedEndPoint* p) {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _set.find(p);
        if (it == _set.end() || *it != p) {
            // another thread has been erase it
            return;
        }
        _set.erase(it);
    }

    inline size_t GlobalEndPointSet::Hash::operator()(ExtendedEndPoint* const& p) const {
        return p->_hash;
    }

    inline bool GlobalEndPointSet::Equals::operator()(ExtendedEndPoint* const& p1, ExtendedEndPoint* const& p2) const {
        return p1->_socklen == p2->_socklen
               && memcmp(&p1->_u, &p2->_u, p1->_socklen) == 0;
    }

}  // namespace turbo::system_internal

#endif  // TURBO_SYSTEM_INTERNAL_ENDPOINT_H_
