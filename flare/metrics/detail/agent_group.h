
#ifndef  FLARE_VARIABLE_DETAIL__AGENT_GROUP_H_
#define  FLARE_VARIABLE_DETAIL__AGENT_GROUP_H_

#include <pthread.h>                        // pthread_mutex_*
#include <cstdlib>                         // abort

#include <new>                              // std::nothrow
#include <deque>                            // std::deque
#include <vector>                           // std::vector

#include "flare/base/errno.h"                      // errno
#include "flare/thread/thread.h"              // thread_atexit
#include "flare/base/scoped_lock.h"
#include "flare/log/logging.h"

namespace flare {
    namespace metrics_detail {

        typedef int agent_id;

        // General NOTES:
        // * Don't use bound-checking vector::at.
        // * static functions in template class are not guaranteed to be inlined,
        //   add inline keyword explicitly.
        // * Put fast path in "if" branch, which is more cpu-wise.
        // * don't use __builtin_expect excessively because CPU may predict the branch
        //   better than you. Only hint branches that are definitely unusual.

        template<typename Agent>
        class agent_group {
        public:
            typedef Agent agent_type;

            // TODO: We should remove the template parameter and unify agent_group
            // of all variable with a same one, to reuse the memory between different
            // type of variable. The unified agent_group allocates small structs in-place
            // and large structs on heap, thus keeping batch efficiencies on small
            // structs and improving memory usage on large structs.
            const static size_t RAW_BLOCK_SIZE = 4096;
            const static size_t ELEMENTS_PER_BLOCK =
                    (RAW_BLOCK_SIZE + sizeof(Agent) - 1) / sizeof(Agent);

            // The most generic method to allocate agents is to call ctor when
            // agent is needed, however we construct all agents when initializing
            // ThreadBlock, which has side effects:
            //  * calling ctor ELEMENTS_PER_BLOCK times is slower.
            //  * calling ctor of non-pod types may be unpredictably slow.
            //  * non-pod types may allocate space inside ctor excessively.
            //  * may return non-null for unexist id.
            //  * lifetime of agent is more complex. User has to reset the agent before
            //    destroying id otherwise when the agent is (implicitly) reused by
            //    another one who gets the reused id, things are screwed.
            // TODO(chenzhangyi01): To fix these problems, a method is to keep a bitmap
            // along with ThreadBlock* in _s_tls_blocks, each bit in the bitmap
            // represents that the agent is constructed or not. Drawback of this method
            // is that the bitmap may take 32bytes (for 256 agents, which is common) so
            // that addressing on _s_tls_blocks may be slower if identifiers distribute
            // sparsely. Another method is to put the bitmap in ThreadBlock. But this
            // makes alignment of ThreadBlock harder and to address the agent we have
            // to touch an additional cacheline: the bitmap. Whereas in the first
            // method, bitmap and ThreadBlock* are in one cacheline.
            struct FLARE_CACHELINE_ALIGNMENT ThreadBlock {
                inline Agent *at(size_t offset) { return _agents + offset; };

            private:
                Agent _agents[ELEMENTS_PER_BLOCK];
            };

            inline static agent_id create_new_agent() {
                FLARE_SCOPED_LOCK(_s_mutex);
                agent_id agent_id = 0;
                if (!_get_free_ids().empty()) {
                    agent_id = _get_free_ids().back();
                    _get_free_ids().pop_back();
                } else {
                    agent_id = _s_agent_kinds++;
                }
                return agent_id;
            }

            inline static int destroy_agent(agent_id id) {
                // TODO: How to avoid double free?
                FLARE_SCOPED_LOCK(_s_mutex);
                if (id < 0 || id >= _s_agent_kinds) {
                    errno = EINVAL;
                    return -1;
                }
                _get_free_ids().push_back(id);
                return 0;
            }

            // Note: May return non-null for unexist id, see notes on ThreadBlock
            // We need this function to be as fast as possible.
            inline static Agent *get_tls_agent(agent_id id) {
                if (__builtin_expect(id >= 0, 1)) {
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
            inline static Agent *get_or_create_tls_agent(agent_id id) {
                if (__builtin_expect(id < 0, 0)) {
                    FLARE_CHECK(false) << "Invalid id=" << id;
                    return nullptr;
                }
                if (_s_tls_blocks == nullptr) {
                    _s_tls_blocks = new(std::nothrow) std::vector<ThreadBlock *>;
                    if (__builtin_expect(_s_tls_blocks == nullptr, 0)) {
                        FLARE_LOG(FATAL) << "Fail to create vector, " << flare_error();
                        return nullptr;
                    }
                    flare::thread::atexit(_destroy_tls_blocks);
                }
                const size_t block_id = (size_t) id / ELEMENTS_PER_BLOCK;
                if (block_id >= _s_tls_blocks->size()) {
                    // The 32ul avoid pointless small resizes.
                    _s_tls_blocks->resize(std::max(block_id + 1, 32ul));
                }
                ThreadBlock *tb = (*_s_tls_blocks)[block_id];
                if (tb == nullptr) {
                    ThreadBlock *new_block = new(std::nothrow) ThreadBlock;
                    if (__builtin_expect(new_block == nullptr, 0)) {
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

            inline static std::deque<agent_id> &_get_free_ids() {
                if (__builtin_expect(!_s_free_ids, 0)) {
                    _s_free_ids = new(std::nothrow) std::deque<agent_id>();
                    if (!_s_free_ids) {
                        abort();
                    }
                }
                return *_s_free_ids;
            }

            static pthread_mutex_t _s_mutex;
            static agent_id _s_agent_kinds;
            static std::deque<agent_id> *_s_free_ids;
            static __thread std::vector<ThreadBlock *> *_s_tls_blocks;
        };

        template<typename Agent>
        pthread_mutex_t agent_group<Agent>::_s_mutex = PTHREAD_MUTEX_INITIALIZER;

        template<typename Agent>
        std::deque<agent_id> *agent_group<Agent>::_s_free_ids = nullptr;

        template<typename Agent>
        agent_id agent_group<Agent>::_s_agent_kinds = 0;

        template<typename Agent>
        __thread std::vector<typename agent_group<Agent>::ThreadBlock *>
                *agent_group<Agent>::_s_tls_blocks = nullptr;

    }  // namespace metrics_detail
}  // namespace flare

#endif  // FLARE_VARIABLE_DETAIL__AGENT_GROUP_H_
