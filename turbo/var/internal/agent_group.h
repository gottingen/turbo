// Copyright 2023 The Turbo Authors.
//
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

#ifndef  TURBO_VAR_INTERNAL_AGENT_GROUP_H_
#define  TURBO_VAR_INTERNAL_AGENT_GROUP_H_

#include <thread>
#include <stdlib.h>                         // abort
#include <new>                              // std::nothrow
#include <deque>                            // std::deque
#include <vector>                           // std::vector
#include "turbo/status/error.h"
#include "turbo/system/threading.h"
#include "turbo/system/atexit.h"
#include "turbo/platform/port.h"
#include "turbo/base/internal/raw_logging.h"

namespace turbo::var_internal {

    typedef int AgentId;

    template<typename Agent>
    class AgentGroup {
    public:
        typedef Agent agent_type;

        const static size_t RAW_BLOCK_SIZE = 4096;
        const static size_t ELEMENTS_PER_BLOCK =
                (RAW_BLOCK_SIZE + sizeof(Agent) - 1) / sizeof(Agent);

        struct TURBO_CACHE_LINE_ALIGNED ThreadBlock {
            inline Agent *at(size_t offset) { return _agents + offset; };

        private:
            Agent _agents[ELEMENTS_PER_BLOCK];
        };

        inline static AgentId create_new_agent() {
            std::unique_lock l(_s_mutex);
            AgentId agent_id = 0;
            if (!_get_free_ids().empty()) {
                agent_id = _get_free_ids().back();
                _get_free_ids().pop_back();
            } else {
                agent_id = _s_agent_kinds++;
            }
            return agent_id;
        }

        inline static int destroy_agent(AgentId id) {
            std::unique_lock l(_s_mutex);
            if (id < 0 || id >= _s_agent_kinds) {
                errno = EINVAL;
                return -1;
            }
            _get_free_ids().push_back(id);
            return 0;
        }

        // Note: May return non-null for unexist id, see notes on ThreadBlock
        // We need this function to be as fast as possible.
        inline static Agent *get_tls_agent(AgentId id) {
            if (TURBO_LIKELY(id >= 0)) {
                if (_s_tls_blocks) {
                    const size_t block_id = (size_t) id / ELEMENTS_PER_BLOCK;
                    if (block_id < _s_tls_blocks->size()) {
                        ThreadBlock *const tb = (*_s_tls_blocks)[block_id];
                        if (tb) {
                            return tb->at(id - block_id * ELEMENTS_PER_BLOCK);
                        }
                    }
                }
            }
            return nullptr;
        }

        // Note: May return non-null for unexist id, see notes on ThreadBlock
        inline static Agent *get_or_create_tls_agent(AgentId id) {
            if (TURBO_UNLIKELY(id < 0)) {
                TURBO_RAW_LOG(FATAL, "Invalid id=%d", id);
                return nullptr;
            }
            if (_s_tls_blocks == nullptr) {
                _s_tls_blocks = new(std::nothrow) std::vector<ThreadBlock *>;
                if (TURBO_UNLIKELY(_s_tls_blocks == nullptr)) {
                    TURBO_RAW_LOG(FATAL, "Fail to create vector, %s", terror());
                    return nullptr;
                }
                turbo::thread_atexit(_destroy_tls_blocks);
            }
            const size_t block_id = (size_t) id / ELEMENTS_PER_BLOCK;
            if (block_id >= _s_tls_blocks->size()) {
                // The 32ul avoid pointless small resizes.
                _s_tls_blocks->resize(std::max(block_id + 1, 32ul));
            }
            ThreadBlock *tb = (*_s_tls_blocks)[block_id];
            if (tb == nullptr) {
                ThreadBlock *new_block = new(std::nothrow) ThreadBlock;
                if (TURBO_UNLIKELY(new_block == nullptr)) {
                    return nullptr;
                }
                tb = new_block;
                (*_s_tls_blocks)[block_id] = new_block;
            }
            return tb->at(id - block_id * ELEMENTS_PER_BLOCK);
        }

    private:
        static void _destroy_tls_blocks() {
            if (!_s_tls_blocks) {
                return;
            }
            for (size_t i = 0; i < _s_tls_blocks->size(); ++i) {
                delete (*_s_tls_blocks)[i];
            }
            delete _s_tls_blocks;
            _s_tls_blocks = nullptr;
        }

        inline static std::deque<AgentId> &_get_free_ids() {
            if (TURBO_UNLIKELY(!_s_free_ids)) {
                _s_free_ids = new(std::nothrow) std::deque<AgentId>();
                if (_s_free_ids == nullptr) {
                    TURBO_RAW_LOG(FATAL, "Fail to create deque, %s", terror());
                }
            }
            return *_s_free_ids;
        }

        static std::mutex _s_mutex;
        static AgentId _s_agent_kinds;
        static std::deque<AgentId> *_s_free_ids;
        static __thread std::vector<ThreadBlock *> *_s_tls_blocks;
    };

    template<typename Agent>
    std::mutex AgentGroup<Agent>::_s_mutex;

    template<typename Agent>
    std::deque<AgentId> *AgentGroup<Agent>::_s_free_ids = nullptr;

    template<typename Agent>
    AgentId AgentGroup<Agent>::_s_agent_kinds = 0;

    template<typename Agent>
    __thread std::vector<typename AgentGroup<Agent>::ThreadBlock *>
            *AgentGroup<Agent>::_s_tls_blocks = nullptr;

}  // namespace turbo::var_internal

#endif  // TURBO_VAR_INTERNAL_AGENT_GROUP_H_
