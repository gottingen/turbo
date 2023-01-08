
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_CONTAINER_PTR_CONTAINER_H_
#define FLARE_CONTAINER_PTR_CONTAINER_H_

namespace flare::container {

    // Manage lifetime of a pointer. The key difference between ptr_container and
    // unique_ptr is that ptr_container can be copied and the pointer inside is
    // deeply copied or constructed on-demand.
    template<typename T>
    class ptr_container {
    public:
        ptr_container() : _ptr(nullptr) {}

        explicit ptr_container(T *obj) : _ptr(obj) {}

        ~ptr_container() {
            delete _ptr;
        }

        ptr_container(const ptr_container &rhs)
                : _ptr(rhs._ptr ? new T(*rhs._ptr) : nullptr) {}

        void operator=(const ptr_container &rhs) {
            if (rhs._ptr) {
                if (_ptr) {
                    *_ptr = *rhs._ptr;
                } else {
                    _ptr = new T(*rhs._ptr);
                }
            } else {
                delete _ptr;
                _ptr = nullptr;
            }
        }

        T *get() const { return _ptr; }

        void reset(T *ptr) {
            delete _ptr;
            _ptr = ptr;
        }

        operator void *() const { return _ptr; }

    private:
        T *_ptr;
    };

}  // namespace flare::container

#endif  // FLARE_CONTAINER_PTR_CONTAINER_H_
