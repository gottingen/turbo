#ifndef TURBO_CONTAINER_FLAT_HASH_MAP_DUMP_H_
#define TURBO_CONTAINER_FLAT_HASH_MAP_DUMP_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include "turbo/container/flat_hash_map.h"

namespace turbo {

    namespace type_traits_internal {

#if defined(__GLIBCXX__) && __GLIBCXX__ < 20150801
        template<typename T> struct IsTriviallyCopyable : public std::integral_constant<bool, __has_trivial_copy(T)> {};
#else
        template<typename T>
        struct IsTriviallyCopyable : public std::is_trivially_copyable<T> {
        };
#endif

        template<class T1, class T2>
        struct IsTriviallyCopyable<std::pair<T1, T2>> {
            static constexpr bool value = IsTriviallyCopyable<T1>::value && IsTriviallyCopyable<T2>::value;
        };
    }

    namespace priv {

#if !defined(TURBO_MAP_NON_DETERMINISTIC) && !defined(TURBO_MAP_DISABLE_DUMP)

        // ------------------------------------------------------------------------
        // dump/load for raw_hash_set
        // ------------------------------------------------------------------------
        template<class Policy, class Hash, class Eq, class Alloc>
        template<typename OutputArchive>
        bool raw_hash_set<Policy, Hash, Eq, Alloc>::turbo_map_dump(OutputArchive &ar) const {
            static_assert(type_traits_internal::IsTriviallyCopyable<value_type>::value,
                          "value_type should be trivially copyable");

            ar.saveBinary(&size_, sizeof(size_t));
            if (size_ == 0)
                return true;
            ar.saveBinary(&capacity_, sizeof(size_t));
            ar.saveBinary(ctrl_, sizeof(ctrl_t) * (capacity_ + Group::kWidth + 1));
            ar.saveBinary(slots_, sizeof(slot_type) * capacity_);
            return true;
        }

        template<class Policy, class Hash, class Eq, class Alloc>
        template<typename InputArchive>
        bool raw_hash_set<Policy, Hash, Eq, Alloc>::turbo_map_load(InputArchive &ar) {
            static_assert(type_traits_internal::IsTriviallyCopyable<value_type>::value,
                          "value_type should be trivially copyable");
            raw_hash_set<Policy, Hash, Eq, Alloc>().swap(*this); // clear any existing content
            ar.loadBinary(&size_, sizeof(size_t));
            if (size_ == 0)
                return true;
            ar.loadBinary(&capacity_, sizeof(size_t));

            // allocate memory for ctrl_ and slots_
            initialize_slots(capacity_);
            ar.loadBinary(ctrl_, sizeof(ctrl_t) * (capacity_ + Group::kWidth + 1));
            ar.loadBinary(slots_, sizeof(slot_type) * capacity_);
            return true;
        }

        // ------------------------------------------------------------------------
        // dump/load for parallel_hash_set
        // ------------------------------------------------------------------------
        template<size_t N,
                template<class, class, class, class> class RefSet,
                class Mtx_,
                class Policy, class Hash, class Eq, class Alloc>
        template<typename OutputArchive>
        bool parallel_hash_set<N, RefSet, Mtx_, Policy, Hash, Eq, Alloc>::turbo_map_dump(OutputArchive &ar) const {
            static_assert(type_traits_internal::IsTriviallyCopyable<value_type>::value,
                          "value_type should be trivially copyable");

            size_t submap_count = subcnt();
            ar.saveBinary(&submap_count, sizeof(size_t));
            for (size_t i = 0; i < sets_.size(); ++i) {
                auto &inner = sets_[i];
                typename Lockable::UniqueLock m(const_cast<Inner &>(inner));
                if (!inner.set_.turbo_map_dump(ar)) {
                    std::cerr << "Failed to dump submap " << i << std::endl;
                    return false;
                }
            }
            return true;
        }

        template<size_t N,
                template<class, class, class, class> class RefSet,
                class Mtx_,
                class Policy, class Hash, class Eq, class Alloc>
        template<typename InputArchive>
        bool parallel_hash_set<N, RefSet, Mtx_, Policy, Hash, Eq, Alloc>::turbo_map_load(InputArchive &ar) {
            static_assert(type_traits_internal::IsTriviallyCopyable<value_type>::value,
                          "value_type should be trivially copyable");

            size_t submap_count = 0;
            ar.loadBinary(&submap_count, sizeof(size_t));
            if (submap_count != subcnt()) {
                std::cerr << "submap count(" << submap_count << ") != N(" << N << ")" << std::endl;
                return false;
            }

            for (size_t i = 0; i < submap_count; ++i) {
                auto &inner = sets_[i];
                typename Lockable::UniqueLock m(const_cast<Inner &>(inner));
                if (!inner.set_.turbo_map_load(ar)) {
                    std::cerr << "Failed to load submap " << i << std::endl;
                    return false;
                }
            }
            return true;
        }

#endif // !defined(TURBO_MAP_NON_DETERMINISTIC) && !defined(TURBO_MAP_DISABLE_DUMP)

    } // namespace priv



    // ------------------------------------------------------------------------
    // BinaryArchive
    //       File is closed when archive object is destroyed
    // ------------------------------------------------------------------------

    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------
    class BinaryOutputArchive {
    public:
        BinaryOutputArchive(const char *file_path) {
            ofs_.open(file_path, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
        }

        bool saveBinary(const void *p, size_t sz) {
            ofs_.write(reinterpret_cast<const char *>(p), sz);
            return true;
        }

    private:
        std::ofstream ofs_;
    };


    class BinaryInputArchive {
    public:
        BinaryInputArchive(const char *file_path) {
            ifs_.open(file_path, std::ofstream::in | std::ofstream::binary);
        }

        bool loadBinary(void *p, size_t sz) {
            ifs_.read(reinterpret_cast<char *>(p), sz);
            return true;
        }

    private:
        std::ifstream ifs_;
    };

} // namespace turbo


#ifdef CEREAL_SIZE_TYPE

template <class T>
using turbo_triv_copyable = typename turbo::type_traits_internal::IsTriviallyCopyable<T>;
    
namespace cereal
{
    // Overload Cereal serialization code for turbo::flat_hash_map
    // -----------------------------------------------------------
    template <class K, class V, class Hash, class Eq, class A>
    void save(typename std::enable_if<turbo_triv_copyable<K>::value && turbo_triv_copyable<V>::value, typename cereal::BinaryOutputArchive>::type &ar,
              turbo::flat_hash_map<K, V, Hash, Eq, A> const &hmap)
    {
        hmap.turbo_map_dump(ar);
    }

    template <class K, class V, class Hash, class Eq, class A>
    void load(typename std::enable_if<turbo_triv_copyable<K>::value && turbo_triv_copyable<V>::value, typename cereal::BinaryInputArchive>::type &ar,
              turbo::flat_hash_map<K, V, Hash, Eq, A>  &hmap)
    {
        hmap.turbo_map_load(ar);
    }


    // Overload Cereal serialization code for turbo::parallel_flat_hash_map
    // --------------------------------------------------------------------
    template <class K, class V, class Hash, class Eq, class A, size_t N, class Mtx_>
    void save(typename std::enable_if<turbo_triv_copyable<K>::value && turbo_triv_copyable<V>::value, typename cereal::BinaryOutputArchive>::type &ar,
              turbo::parallel_flat_hash_map<K, V, Hash, Eq, A, N, Mtx_> const &hmap)
    {
        hmap.turbo_map_dump(ar);
    }

    template <class K, class V, class Hash, class Eq, class A, size_t N, class Mtx_>
    void load(typename std::enable_if<turbo_triv_copyable<K>::value && turbo_triv_copyable<V>::value, typename cereal::BinaryInputArchive>::type &ar,
              turbo::parallel_flat_hash_map<K, V, Hash, Eq, A, N, Mtx_>  &hmap)
    {
        hmap.turbo_map_load(ar);
    }

    // Overload Cereal serialization code for turbo::flat_hash_set
    // -----------------------------------------------------------
    template <class K, class Hash, class Eq, class A>
    void save(typename std::enable_if<turbo_triv_copyable<K>::value, typename cereal::BinaryOutputArchive>::type &ar,
              turbo::flat_hash_set<K, Hash, Eq, A> const &hset)
    {
        hset.turbo_map_dump(ar);
    }

    template <class K, class Hash, class Eq, class A>
    void load(typename std::enable_if<turbo_triv_copyable<K>::value, typename cereal::BinaryInputArchive>::type &ar,
              turbo::flat_hash_set<K, Hash, Eq, A>  &hset)
    {
        hset.turbo_map_load(ar);
    }

    // Overload Cereal serialization code for turbo::parallel_flat_hash_set
    // --------------------------------------------------------------------
    template <class K, class Hash, class Eq, class A, size_t N, class Mtx_>
    void save(typename std::enable_if<turbo_triv_copyable<K>::value, typename cereal::BinaryOutputArchive>::type &ar,
              turbo::parallel_flat_hash_set<K, Hash, Eq, A, N, Mtx_> const &hset)
    {
        hset.turbo_map_dump(ar);
    }

    template <class K, class Hash, class Eq, class A, size_t N, class Mtx_>
    void load(typename std::enable_if<turbo_triv_copyable<K>::value, typename cereal::BinaryInputArchive>::type &ar,
              turbo::parallel_flat_hash_set<K, Hash, Eq, A, N, Mtx_>  &hset)
    {
        hset.turbo_map_load(ar);
    }
}

#endif


#endif  // TURBO_CONTAINER_FLAT_HASH_MAP_DUMP_H_
