/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include <flare/container/dynamic_bitset.h>
#include <flare/log/logging.h>
#include "flare/base/natural_number_set.h"

namespace flare {
    template<typename T>
    natural_number_set<T>::natural_number_set()
            : _values_bitset(std::make_unique<flare::dynamic_bitset<>>()) {
    }

    template<typename T>
    bool natural_number_set<T>::is_empty() const {
        return _values_vector.empty();
    }

    template<typename T>
    void natural_number_set<T>::reserve(size_t count) {
        _values_vector.reserve(count);
        _values_bitset->reserve(count);
    }

    template<typename T>
    void natural_number_set<T>::insert(T id) {
        _values_vector.emplace_back(id);

        if (id >= _values_bitset->size())
            _values_bitset->resize(static_cast<size_t>(id) + 1);

        _values_bitset->set(id, true);
    }

    template<typename T>
    bool natural_number_set<T>::pop_any(T &id) {
        if (_values_vector.empty()) {
            return false;
        }

        id = _values_vector.back();
        _values_vector.pop_back();

        _values_bitset->set(id, false);

        return true;
    }

    template<typename T>
    void natural_number_set<T>::clear() {
        _values_vector.clear();
        _values_bitset->clear();
    }

    template<typename T>
    size_t natural_number_set<T>::size() const {
        return _values_vector.size();
    }

    template<typename T>
    bool natural_number_set<T>::is_in_set(T id) const {
        return _values_bitset->test(id);
    }

    // Instantiate used templates.
    template
    class natural_number_set<unsigned>;
}  // namespace flare
