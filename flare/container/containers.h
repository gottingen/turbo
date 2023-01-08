
#ifndef FLARE_CONTAINER_CONTAINERS_H_
#define FLARE_CONTAINER_CONTAINERS_H_

#include "flare/memory/allocator.h"

#include <algorithm>  // std::max
#include <cstddef>    // size_t
#include <utility>    // std::move

#include <deque>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace flare {
    namespace containers {

        template<typename T>
        using deque = std::deque<T, stl_allocator<T>>;

        template<typename K, typename V, typename C = std::less<K>>
        using map = std::map<K, V, C, stl_allocator<std::pair<const K, V>>>;

        template<typename K, typename C = std::less<K>>
        using set = std::set<K, C, stl_allocator<K>>;

        template<typename K,
                typename V,
                typename H = std::hash<K>,
                typename E = std::equal_to<K>>
        using unordered_map =
        std::unordered_map<K, V, H, E, stl_allocator<std::pair<const K, V>>>;

        template<typename K, typename H = std::hash<K>, typename E = std::equal_to<K>>
        using unordered_set = std::unordered_set<K, H, E, stl_allocator<K>>;

        // take() takes and returns the front value from the deque.
        template<typename T>
        inline T take(deque<T> &queue) {
            auto out = std::move(queue.front());
            queue.pop_front();
            return out;
        }

        // take() takes and returns the first value from the unordered_set.
        template<typename T, typename H, typename E>
        inline T take(unordered_set<T, H, E> &set) {
            auto it = set.begin();
            auto out = std::move(*it);
            set.erase(it);
            return out;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // vector<T, BASE_CAPACITY>
        ////////////////////////////////////////////////////////////////////////////////

        // vector is a container of contiguously stored elements.
        // Unlike std::vector, flare::containers::vector keeps the first
        // BASE_CAPACITY elements internally, which will avoid dynamic heap
        // allocations. Once the vector exceeds BASE_CAPACITY elements, vector will
        // allocate storage from the heap.
        template<typename T, int BASE_CAPACITY>
        class vector {
        public:
            inline vector(allocator *allocator = allocator::Default);

            template<int BASE_CAPACITY_2>
            inline vector(const vector<T, BASE_CAPACITY_2> &other,
                          allocator *allocator = allocator::Default);

            template<int BASE_CAPACITY_2>
            inline vector(vector<T, BASE_CAPACITY_2> &&other,
                          allocator *allocator = allocator::Default);

            inline ~vector();

            inline vector &operator=(const vector &);

            template<int BASE_CAPACITY_2>
            inline vector<T, BASE_CAPACITY> &operator=(
                    const vector<T, BASE_CAPACITY_2> &);

            template<int BASE_CAPACITY_2>
            inline vector<T, BASE_CAPACITY> &operator=(
                    vector<T, BASE_CAPACITY_2> &&);

            inline void push_back(const T &el);

            inline void emplace_back(T &&el);

            inline void pop_back();

            inline T &front();

            inline T &back();

            inline const T &front() const;

            inline const T &back() const;

            inline T *begin();

            inline T *end();

            inline const T *begin() const;

            inline const T *end() const;

            inline T &operator[](size_t i);

            inline const T &operator[](size_t i) const;

            inline size_t size() const;

            inline size_t cap() const;

            inline void resize(size_t n);

            inline void reserve(size_t n);

            inline T *data();

            inline const T *data() const;

            allocator *const _allocator;

        private:
            using TStorage = typename flare::aligned_storage<sizeof(T), alignof(T)>::type;

            vector(const vector &) = delete;

            inline void free();

            size_t count = 0;
            size_t capacity = BASE_CAPACITY;
            TStorage buffer[BASE_CAPACITY];
            TStorage *elements = buffer;
            allocation _allocation;
        };

        template<typename T, int BASE_CAPACITY>
        vector<T, BASE_CAPACITY>::vector(
                flare::allocator *allocator /* = allocator::Default */)
                : _allocator(allocator) {}

        template<typename T, int BASE_CAPACITY>
        template<int BASE_CAPACITY_2>
        vector<T, BASE_CAPACITY>::vector(
                const vector<T, BASE_CAPACITY_2> &other, flare::allocator *alloc /* = allocator::Default */)
                : _allocator(alloc) {
            *this = other;
        }

        template<typename T, int BASE_CAPACITY>
        template<int BASE_CAPACITY_2>
        vector<T, BASE_CAPACITY>::vector(
                vector<T, BASE_CAPACITY_2> &&other,
                flare::allocator *allocator /* = allocator::Default */)
                : _allocator(allocator) {
            *this = std::move(other);
        }

        template<typename T, int BASE_CAPACITY>
        vector<T, BASE_CAPACITY>::~vector() {
            free();
        }

        template<typename T, int BASE_CAPACITY>
        vector<T, BASE_CAPACITY> &vector<T, BASE_CAPACITY>::operator=(
                const vector &other) {
            free();
            reserve(other.size());
            count = other.size();
            for (size_t i = 0; i < count; i++) {
                new(&reinterpret_cast<T *>(elements)[i]) T(other[i]);
            }
            return *this;
        }

        template<typename T, int BASE_CAPACITY>
        template<int BASE_CAPACITY_2>
        vector<T, BASE_CAPACITY> &vector<T, BASE_CAPACITY>::operator=(
                const vector<T, BASE_CAPACITY_2> &other) {
            free();
            reserve(other.size());
            count = other.size();
            for (size_t i = 0; i < count; i++) {
                new(&reinterpret_cast<T *>(elements)[i]) T(other[i]);
            }
            return *this;
        }

        template<typename T, int BASE_CAPACITY>
        template<int BASE_CAPACITY_2>
        vector<T, BASE_CAPACITY> &vector<T, BASE_CAPACITY>::operator=(
                vector<T, BASE_CAPACITY_2> &&other) {
            free();
            reserve(other.size());
            count = other.size();
            for (size_t i = 0; i < count; i++) {
                new(&reinterpret_cast<T *>(elements)[i]) T(std::move(other[i]));
            }
            other.resize(0);
            return *this;
        }

        template<typename T, int BASE_CAPACITY>
        void vector<T, BASE_CAPACITY>::push_back(const T &el) {
            reserve(count + 1);
            new(&reinterpret_cast<T *>(elements)[count]) T(el);
            count++;
        }

        template<typename T, int BASE_CAPACITY>
        void vector<T, BASE_CAPACITY>::emplace_back(T &&el) {
            reserve(count + 1);
            new(&reinterpret_cast<T *>(elements)[count]) T(std::move(el));
            count++;
        }

        template<typename T, int BASE_CAPACITY>
        void vector<T, BASE_CAPACITY>::pop_back() {
            FLARE_CHECK(count > 0) << "pop_back() called on empty vector";
            count--;
            reinterpret_cast<T *>(elements)[count].~T();
        }

        template<typename T, int BASE_CAPACITY>
        T &vector<T, BASE_CAPACITY>::front() {
            FLARE_CHECK(count > 0) << "front() called on empty vector";
            return reinterpret_cast<T *>(elements)[0];
        }

        template<typename T, int BASE_CAPACITY>
        T &vector<T, BASE_CAPACITY>::back() {
            FLARE_CHECK(count > 0) << "back() called on empty vector";
            return reinterpret_cast<T *>(elements)[count - 1];
        }

        template<typename T, int BASE_CAPACITY>
        const T &vector<T, BASE_CAPACITY>::front() const {
            FLARE_CHECK(count > 0) << "front() called on empty vector";
            return reinterpret_cast<T *>(elements)[0];
        }

        template<typename T, int BASE_CAPACITY>
        const T &vector<T, BASE_CAPACITY>::back() const {
            FLARE_CHECK(count > 0) << "back() called on empty vector";
            return reinterpret_cast<T *>(elements)[count - 1];
        }

        template<typename T, int BASE_CAPACITY>
        T *vector<T, BASE_CAPACITY>::begin() {
            return reinterpret_cast<T *>(elements);
        }

        template<typename T, int BASE_CAPACITY>
        T *vector<T, BASE_CAPACITY>::end() {
            return reinterpret_cast<T *>(elements) + count;
        }

        template<typename T, int BASE_CAPACITY>
        const T *vector<T, BASE_CAPACITY>::begin() const {
            return reinterpret_cast<T *>(elements);
        }

        template<typename T, int BASE_CAPACITY>
        const T *vector<T, BASE_CAPACITY>::end() const {
            return reinterpret_cast<T *>(elements) + count;
        }

        template<typename T, int BASE_CAPACITY>
        T &vector<T, BASE_CAPACITY>::operator[](size_t i) {
            FLARE_CHECK(i < count);
            return reinterpret_cast<T *>(elements)[i];
        }

        template<typename T, int BASE_CAPACITY>
        const T &vector<T, BASE_CAPACITY>::operator[](size_t i) const {
            FLARE_CHECK(i < count);
            return reinterpret_cast<T *>(elements)[i];
        }

        template<typename T, int BASE_CAPACITY>
        size_t vector<T, BASE_CAPACITY>::size() const {
            return count;
        }

        template<typename T, int BASE_CAPACITY>
        void vector<T, BASE_CAPACITY>::resize(size_t n) {
            reserve(n);
            while (count < n) {
                new(&reinterpret_cast<T *>(elements)[count++]) T();
            }
            while (n < count) {
                reinterpret_cast<T *>(elements)[--count].~T();
            }
        }

        template<typename T, int BASE_CAPACITY>
        void vector<T, BASE_CAPACITY>::reserve(size_t n) {
            if (n > capacity) {
                capacity = std::max<size_t>(n * 2, 8);

                allocation::required_info request;
                request.size = sizeof(T) * capacity;
                request.alignment = alignof(T);
                request.usage = allocation::Usage::Vector;

                auto a = _allocator->allocate(request);
                auto grown = reinterpret_cast<TStorage *>(a.ptr);
                for (size_t i = 0; i < count; i++) {
                    new(&reinterpret_cast<T *>(grown)[i])
                            T(std::move(reinterpret_cast<T *>(elements)[i]));
                }
                free();
                elements = grown;
                _allocation = a;
            }
        }

        template<typename T, int BASE_CAPACITY>
        T *vector<T, BASE_CAPACITY>::data() {
            return elements;
        }

        template<typename T, int BASE_CAPACITY>
        const T *vector<T, BASE_CAPACITY>::data() const {
            return elements;
        }

        template<typename T, int BASE_CAPACITY>
        void vector<T, BASE_CAPACITY>::free() {
            for (size_t i = 0; i < count; i++) {
                reinterpret_cast<T *>(elements)[i].~T();
            }

            if (_allocation.ptr != nullptr) {
                _allocator->free(_allocation);
                _allocation = {};
                elements = nullptr;
            }
        }

        ////////////////////////////////////////////////////////////////////////////////
        // list<T, BASE_CAPACITY>
        ////////////////////////////////////////////////////////////////////////////////

        // list is a minimal std::list like container that supports constant time
        // insertion and removal of elements.
        // list keeps hold of allocations (it only releases allocations on destruction),
        // to avoid repeated heap allocations and frees when frequently inserting and
        // removing elements.
        template<typename T>
        class list {
            struct Entry {
                T data;
                Entry *next;
                Entry *prev;
            };

        public:
            class iterator {
            public:
                inline iterator(Entry *);

                inline T *operator->();

                inline T &operator*();

                inline iterator &operator++();

                inline bool operator==(const iterator &) const;

                inline bool operator!=(const iterator &) const;

            private:
                friend list;
                Entry *entry;
            };

            inline list(allocator *allocator = allocator::Default);

            inline ~list();

            inline iterator begin();

            inline iterator end();

            inline size_t size() const;

            template<typename... Args>
            inline iterator emplace_front(Args &&... args);

            inline void erase(iterator);

        private:

            // copy / move is currently unsupported.
            list(const list &) = delete;

            list(list &&) = delete;

            list &operator=(const list &) = delete;

            list &operator=(list &&) = delete;

            struct AllocationChain {
                allocation _allocation;
                AllocationChain *next;
            };

            inline void grow(size_t count);

            static inline void unlink(Entry *entry, Entry *&list);

            static inline void link(Entry *entry, Entry *&list);

            allocator *const alloc;
            size_t size_ = 0;
            size_t capacity = 0;
            AllocationChain *allocations = nullptr;
            Entry *free = nullptr;
            Entry *head = nullptr;
        };

        template<typename T>
        list<T>::iterator::iterator(Entry *entry) : entry(entry) {}

        template<typename T>
        T *list<T>::iterator::operator->() {
            return &entry->data;
        }

        template<typename T>
        T &list<T>::iterator::operator*() {
            return entry->data;
        }

        template<typename T>
        typename list<T>::iterator &list<T>::iterator::operator++() {
            entry = entry->next;
            return *this;
        }

        template<typename T>
        bool list<T>::iterator::operator==(const iterator &rhs) const {
            return entry == rhs.entry;
        }

        template<typename T>
        bool list<T>::iterator::operator!=(const iterator &rhs) const {
            return entry != rhs.entry;
        }

        template<typename T>
        list<T>::list(allocator *allocator /* = allocator::Default */)
                : alloc(allocator) {}

        template<typename T>
        list<T>::~list() {
            for (auto el = head; el != nullptr; el = el->next) {
                el->data.~T();
            }

            auto curr = allocations;
            while (curr != nullptr) {
                auto next = curr->next;
                alloc->free(curr->_allocation);
                curr = next;
            }
        }

        template<typename T>
        typename list<T>::iterator list<T>::begin() {
            return {head};
        }

        template<typename T>
        typename list<T>::iterator list<T>::end() {
            return {nullptr};
        }

        template<typename T>
        size_t list<T>::size() const {
            return size_;
        }

        template<typename T>
        template<typename... Args>
        typename list<T>::iterator list<T>::emplace_front(Args &&... args) {
            if (free == nullptr) {
                grow(std::max<size_t>(capacity, 8));
            }

            auto entry = free;

            unlink(entry, free);
            link(entry, head);

            new(&entry->data) T(std::forward<T>(args)...);
            size_++;

            return entry;
        }

        template<typename T>
        void list<T>::erase(iterator it) {
            auto entry = it.entry;
            unlink(entry, head);
            link(entry, free);

            entry->data.~T();
            size_--;
        }

        template<typename T>
        void list<T>::grow(size_t count) {
            auto const entriesSize = sizeof(Entry) * count;
            auto const allocChainOffset = flare::base::align_up(entriesSize, alignof(AllocationChain));
            auto const allocSize = allocChainOffset + sizeof(AllocationChain);

            allocation::required_info request;
            request.size = allocSize;
            request.alignment = std::max(alignof(Entry), alignof(AllocationChain));
            request.usage = allocation::Usage::List;
            auto alloca = alloc->allocate(request);

            auto entries = reinterpret_cast<Entry *>(alloca.ptr);
            for (size_t i = 0; i < count; i++) {
                auto entry = &entries[i];
                entry->prev = nullptr;
                entry->next = free;
                if (free) {
                    free->prev = entry;
                }
                free = entry;
            }

            auto allocChain = reinterpret_cast<AllocationChain *>(
                    reinterpret_cast<uint8_t *>(alloca.ptr) + allocChainOffset);

            allocChain->_allocation = alloca;
            allocChain->next = allocations;
            allocations = allocChain;

            capacity += count;
        }

        template<typename T>
        void list<T>::unlink(Entry *entry, Entry *&list) {
            if (list == entry) {
                list = list->next;
            }
            if (entry->prev) {
                entry->prev->next = entry->next;
            }
            if (entry->next) {
                entry->next->prev = entry->prev;
            }
            entry->prev = nullptr;
            entry->next = nullptr;
        }

        template<typename T>
        void list<T>::link(Entry *entry, Entry *&list) {
            FLARE_CHECK(entry->next == nullptr);
            FLARE_CHECK(entry->prev == nullptr);
            if (list) {
                entry->next = list;
                list->prev = entry;
            }
            list = entry;
        }

    }  // namespace containers
}  // namespace flare

#endif  // FLARE_CONTAINER_CONTAINERS_H_
