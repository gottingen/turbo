
#ifndef  FLARE_VARIABLE_DETAIL_SERIES_H_
#define  FLARE_VARIABLE_DETAIL_SERIES_H_

#include <cmath>                       // round
#include <ostream>
#include <mutex>
#include "flare/metrics/vector.h"
#include "flare/metrics/detail/call_op_returning_void.h"
#include "flare/strings/str_split.h"

namespace flare::metrics_detail {

    template<typename T, typename Op, typename Enabler = void>
    struct divide_on_addition {
        static void inplace_divide(T & /*obj*/, const Op &, int /*number*/) {
            // do nothing
        }
    };

    template<typename T, typename Op>
    struct probably_addtition {
        explicit probably_addtition(const Op &op) {
            T res(32);
            call_op_returning_void(op, res, T(64));
            _ok = (res == T(96));  // works for integral/floating point.
        }

        explicit operator bool() const { return _ok; }

    private:
        bool _ok;
    };

    template<typename T, typename Op>
    struct divide_on_addition<T, Op, typename std::enable_if<
            std::is_integral<T>::value>::type> {
        static void inplace_divide(T &obj, const Op &op, int number) {
            static probably_addtition<T, Op> probably_add(op);
            if (probably_add) {
                obj = (T) round(obj / (double) number);
            }
        }
    };

    template<typename T, typename Op>
    struct divide_on_addition<T, Op, typename std::enable_if<
            std::is_floating_point<T>::value>::type> {
        static void inplace_divide(T &obj, const Op &op, int number) {
            static probably_addtition<T, Op> probably_add(op);
            if (probably_add) {
                obj /= number;
            }
        }
    };

    template<typename T, size_t N, typename Op>
    struct divide_on_addition<Vector<T, N>, Op, typename std::enable_if<
            std::is_integral<T>::value>::type> {
        static void inplace_divide(Vector<T, N> &obj, const Op &op, int number) {
            static probably_addtition<Vector<T, N>, Op> probably_add(op);
            if (probably_add) {
                for (size_t i = 0; i < N; ++i) {
                    obj[i] = (T) round(obj[i] / (double) number);
                }
            }
        }
    };

    template<typename T, size_t N, typename Op>
    struct divide_on_addition<Vector<T, N>, Op, typename std::enable_if<
            std::is_floating_point<T>::value>::type> {
        static void inplace_divide(Vector<T, N> &obj, const Op &op, int number) {
            static probably_addtition<Vector<T, N>, Op> probably_add(op);
            if (probably_add) {
                obj /= number;
            }
        }
    };

    template<typename T, typename Op>
    class series_base {
    public:
        explicit series_base(const Op &op)
                : _op(op), _mutex(), _nsecond(0), _nminute(0), _nhour(0), _nday(0) {
        }

        ~series_base() = default;

        void append(const T &value) {
            std::unique_lock l(_mutex);
            return append_second(value, _op);
        }

    private:
        void append_second(const T &value, const Op &op);

        void append_minute(const T &value, const Op &op);

        void append_hour(const T &value, const Op &op);

        void append_day(const T &value);

        struct inner_data {
        public:
            inner_data() {
                // is_pod does not work for gcc 3.4
                if (std::is_integral<T>::value ||
                    std::is_floating_point<T>::value) {
                    memset(static_cast<void *>(_array), 0, sizeof(_array));
                }
            }

            T &second(int index) { return _array[index]; }

            const T &second(int index) const { return _array[index]; }

            T &minute(int index) { return _array[60 + index]; }

            const T &minute(int index) const { return _array[60 + index]; }

            T &hour(int index) { return _array[120 + index]; }

            const T &hour(int index) const { return _array[120 + index]; }

            T &day(int index) { return _array[144 + index]; }

            const T &day(int index) const { return _array[144 + index]; }

        private:
            T _array[60 + 60 + 24 + 30];
        };

    protected:
        Op _op;
        mutable std::mutex _mutex;
        char _nsecond;
        char _nminute;
        char _nhour;
        char _nday;
        inner_data _data;
    };

    template<typename T, typename Op>
    void series_base<T, Op>::append_second(const T &value, const Op &op) {
        _data.second(_nsecond) = value;
        ++_nsecond;
        if (_nsecond >= 60) {
            _nsecond = 0;
            T tmp = _data.second(0);
            for (int i = 1; i < 60; ++i) {
                call_op_returning_void(op, tmp, _data.second(i));
            }
            divide_on_addition<T, Op>::inplace_divide(tmp, op, 60);
            append_minute(tmp, op);
        }
    }

    template<typename T, typename Op>
    void series_base<T, Op>::append_minute(const T &value, const Op &op) {
        _data.minute(_nminute) = value;
        ++_nminute;
        if (_nminute >= 60) {
            _nminute = 0;
            T tmp = _data.minute(0);
            for (int i = 1; i < 60; ++i) {
                call_op_returning_void(op, tmp, _data.minute(i));
            }
            divide_on_addition<T, Op>::inplace_divide(tmp, op, 60);
            append_hour(tmp, op);
        }
    }

    template<typename T, typename Op>
    void series_base<T, Op>::append_hour(const T &value, const Op &op) {
        _data.hour(_nhour) = value;
        ++_nhour;
        if (_nhour >= 24) {
            _nhour = 0;
            T tmp = _data.hour(0);
            for (int i = 1; i < 24; ++i) {
                call_op_returning_void(op, tmp, _data.hour(i));
            }
            divide_on_addition<T, Op>::inplace_divide(tmp, op, 24);
            append_day(tmp);
        }
    }

    template<typename T, typename Op>
    void series_base<T, Op>::append_day(const T &value) {
        _data.day(_nday) = value;
        ++_nday;
        if (_nday >= 30) {
            _nday = 0;
        }
    }

    template<typename T, typename Op>
    class series : public series_base<T, Op> {
        typedef series_base<T, Op> Base;
    public:
        explicit series(const Op &op) : Base(op) {}

        void describe(std::ostream &os, const std::string *vector_names) const;
    };

    template<typename T, size_t N, typename Op>
    class series<Vector<T, N>, Op> : public series_base<Vector<T, N>, Op> {
        typedef series_base<Vector<T, N>, Op> Base;
    public:
        explicit series(const Op &op) : Base(op) {}

        void describe(std::ostream &os, const std::string *vector_names) const;
    };

    template<typename T, typename Op>
    void series<T, Op>::describe(std::ostream &os,
                                 const std::string *vector_names) const {
        FLARE_CHECK(vector_names == nullptr);
        this->_mutex.lock();
        const int second_begin = this->_nsecond;
        const int minute_begin = this->_nminute;
        const int hour_begin = this->_nhour;
        const int day_begin = this->_nday;
        this->_mutex.unlock();
        // NOTE: we don't save _data which may be inconsistent sometimes, but
        // this output is generally for "peeking the trend" and does not need
        // to exactly accurate.

        int c = 0;
        os << "{\"label\":\"trend\",\"data\":[";
        for (int i = 0; i < 30; ++i, ++c) {
            if (c) {
                os << ',';
            }
            os << '[' << c << ',' << this->_data.day((i + day_begin) % 30) << ']';
        }
        for (int i = 0; i < 24; ++i, ++c) {
            if (c) {
                os << ',';
            }
            os << '[' << c << ',' << this->_data.hour((i + hour_begin) % 24) << ']';
        }
        for (int i = 0; i < 60; ++i, ++c) {
            if (c) {
                os << ',';
            }
            os << '[' << c << ',' << this->_data.minute((i + minute_begin) % 60) << ']';
        }
        for (int i = 0; i < 60; ++i, ++c) {
            if (c) {
                os << ',';
            }
            os << '[' << c << ',' << this->_data.second((i + second_begin) % 60) << ']';
        }
        os << "]}";
    }

    template<typename T, size_t N, typename Op>
    void series<Vector<T, N>, Op>::describe(std::ostream &os,
                                            const std::string *vector_names) const {
        this->_mutex.lock();
        const int second_begin = this->_nsecond;
        const int minute_begin = this->_nminute;
        const int hour_begin = this->_nhour;
        const int day_begin = this->_nday;
        // NOTE: we don't save _data which may be inconsistent sometimes, but
        // this output is generally for "peeking the trend" and does not need
        // to exactly accurate.
        this->_mutex.unlock();

        std::vector<std::string_view> vnames = flare::string_split(vector_names ? vector_names->c_str() : "", ',');
        auto it = vnames.begin();
        os << '[';
        for (size_t j = 0; j < N; ++j) {
            if (j) {
                os << ',';
            }
            int c = 0;
            os << "{\"label\":\"";
            if (it != vnames.end()) {
                os << *it;
                ++it;
            } else {
                os << "Vector[" << j << ']';
            }
            os << "\",\"data\":[";
            for (int i = 0; i < 30; ++i, ++c) {
                if (c) {
                    os << ',';
                }
                os << '[' << c << ',' << this->_data.day((i + day_begin) % 30)[j] << ']';
            }
            for (int i = 0; i < 24; ++i, ++c) {
                if (c) {
                    os << ',';
                }
                os << '[' << c << ',' << this->_data.hour((i + hour_begin) % 24)[j] << ']';
            }
            for (int i = 0; i < 60; ++i, ++c) {
                if (c) {
                    os << ',';
                }
                os << '[' << c << ',' << this->_data.minute((i + minute_begin) % 60)[j] << ']';
            }
            for (int i = 0; i < 60; ++i, ++c) {
                if (c) {
                    os << ',';
                }
                os << '[' << c << ',' << this->_data.second((i + second_begin) % 60)[j] << ']';
            }
            os << "]}";
        }
        os << ']';
    }

}  // namespace flare::metrics_detail

#endif  // FLARE_VARIABLE_DETAIL_SERIES_H_
