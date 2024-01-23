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


#ifndef  TURBO_VAR_REDUCER_H_
#define  TURBO_VAR_REDUCER_H_

#include <limits>
#include "turbo/base/internal/raw_logging.h"
#include "turbo/var/variable.h"
#include "turbo/var/internal/combiner.h"
#include "turbo/var/internal/sampler.h"
#include "turbo/var/internal/series.h"
#include "turbo/var/window.h"
#include "turbo/flags/flag.h"
#include "turbo/meta/reflect.h"

namespace turbo {

    // Reduce multiple values into one with `Op': e1 Op e2 Op e3 ...
    // `Op' shall satisfy:
    //   - associative:     a Op (b Op c) == (a Op b) Op c
    //   - commutative:     a Op b == b Op a;
    //   - no side effects: a Op b never changes if a and b are fixed.
    // otherwise the result is undefined.
    //
    // For performance issues, we don't let Op return value, instead it shall
    // set the result to the first parameter in-place. Namely to add two values,
    // "+=" should be implemented rather than "+".
    //
    // Reducer works for non-primitive T which satisfies:
    //   - T() should be the identity of Op.
    //   - stream << v should compile and put description of v into the stream
    // Example:
    // class MyType {
    // friend std::ostream& operator<<(std::ostream& os, const MyType&);
    // public:
    //     MyType() : _x(0) {}
    //     explicit MyType(int x) : _x(x) {}
    //     void operator+=(const MyType& rhs) const {
    //         _x += rhs._x;
    //     }
    // private:
    //     int _x;
    // };
    // std::ostream& operator<<(std::ostream& os, const MyType& value) {
    //     return os << "MyType{" << value._x << "}";
    // }
    // turbo::Adder<MyType> my_type_sum;
    // my_type_sum << MyType(1) << MyType(2) << MyType(3);
    // LOG(INFO) << my_type_sum;  // "MyType{6}"

    template<typename T, typename Op, typename InvOp = var_internal::VoidOp>
    class Reducer : public Variable {
    public:
        typedef typename var_internal::AgentCombiner<T, T, Op> combiner_type;
        typedef typename combiner_type::Agent agent_type;
        typedef var_internal::ReducerSampler<Reducer, T, Op, InvOp> sampler_type;

        class SeriesSampler : public var_internal::Sampler {
        public:
            SeriesSampler(Reducer *owner, const Op &op)
                    : _owner(owner), _series(op) {}

            ~SeriesSampler() {}

            void take_sample() override { _series.append(_owner->get_value()); }

            void describe(std::ostream &os) { _series.describe(os, nullptr); }

        private:
            Reducer *_owner;
            var_internal::Series<T, Op> _series;
        };

    public:
        // The `identify' must satisfy: identity Op a == a
        Reducer(typename add_cr_non_integral<T>::type identity = T(),
                const Op &op = Op(),
                const InvOp &inv_op = InvOp())
                : _combiner(identity, identity, op), _sampler(nullptr), _series_sampler(nullptr), _inv_op(inv_op) {
        }

        ~Reducer() {
            // Calling hide() manually is a MUST required by Variable.
            hide();
            if (_sampler) {
                _sampler->destroy();
                _sampler = nullptr;
            }
            if (_series_sampler) {
                _series_sampler->destroy();
                _series_sampler = nullptr;
            }
        }

        // Add a value.
        // Returns self reference for chaining.
        Reducer &operator<<(typename add_cr_non_integral<T>::type value);

        // Get reduced value.
        // Notice that this function walks through threads that ever add values
        // into this reducer. You should avoid calling it frequently.
        T get_value() const {
            if(!(std::is_same<InvOp, var_internal::VoidOp>::value) || _sampler == nullptr) {
                /*
                TURBO_RAW_LOG(FATAL, " You should not call Reducer<%s, %s>::get_value() when a"
                                     " Window<> is used because the operator does not have inverse.",
                              std::string(REFLECT_SHORT_TYPE_RTTI(T)).c_str(),
                              std::string(turbo::nameof_short_type<Op>()).c_str());
                              */
            }
            return _combiner.combine_agents();
        }


        // Reset the reduced value to T().
        // Returns the reduced value before reset.
        T reset() { return _combiner.reset_all_agents(); }

        void describe(std::ostream &os, bool quote_string) const override {
            if (std::is_same<T, std::string>::value && quote_string) {
                os << '"' << get_value() << '"';
            } else {
                os << get_value();
            }
        }

        // True if this reducer is constructed successfully.
        bool valid() const { return _combiner.valid(); }

        // Get instance of Op.
        const Op &op() const { return _combiner.op(); }

        const InvOp &inv_op() const { return _inv_op; }

        sampler_type *get_sampler() {
            if (nullptr == _sampler) {
                _sampler = new sampler_type(this);
                _sampler->schedule();
            }
            return _sampler;
        }

        int describe_series(std::ostream &os, const SeriesOptions &options) const override {
            if (_series_sampler == nullptr) {
                return 1;
            }
            if (!options.test_only) {
                _series_sampler->describe(os);
            }
            return 0;
        }

    protected:
        int expose_impl(const std::string_view &prefix,
                        const std::string_view &name,
                        DisplayFilter display_filter) override {
            const int rc = Variable::expose_impl(prefix, name, display_filter);
            if (rc == 0 &&
                _series_sampler == nullptr &&
                !std::is_same<InvOp, var_internal::VoidOp>::value &&
                !std::is_same<T, std::string>::value &&
                get_flag(FLAGS_var_save_series)) {
                _series_sampler = new SeriesSampler(this, _combiner.op());
                _series_sampler->schedule();
            }
            return rc;
        }

    private:
        combiner_type _combiner;
        sampler_type *_sampler;
        SeriesSampler *_series_sampler;
        InvOp _inv_op;
    };

    template<typename T, typename Op, typename InvOp>
    inline Reducer<T, Op, InvOp> &Reducer<T, Op, InvOp>::operator<<(
            typename add_cr_non_integral<T>::type value) {
        // It's wait-free for most time
        agent_type *agent = _combiner.get_or_create_tls_agent();
        if (__builtin_expect(!agent, 0)) {
            TURBO_RAW_LOG(FATAL, "Fail to create agent");
            return *this;
        }
        agent->element.modify(_combiner.op(), value);
        return *this;
    }

    // =================== Common reducers ===================

    // var::Adder<int> sum;
    // sum << 1 << 2 << 3 << 4;
    // LOG(INFO) << sum.get_value(); // 10
    // Commonly used functors
    namespace var_internal {
        template<typename Tp>
        struct AddTo {
            void operator()(Tp &lhs,
                            typename add_cr_non_integral<Tp>::type rhs) const { lhs += rhs; }
        };

        template<typename Tp>
        struct MinusFrom {
            void operator()(Tp &lhs,
                            typename add_cr_non_integral<Tp>::type rhs) const { lhs -= rhs; }
        };
    }
    template<typename T>
    class Adder : public Reducer<T, var_internal::AddTo<T>, var_internal::MinusFrom<T> > {
    public:
        typedef Reducer<T, var_internal::AddTo<T>, var_internal::MinusFrom<T> > Base;
        typedef T value_type;
        typedef typename Base::sampler_type sampler_type;
    public:
        Adder() : Base() {}

        explicit Adder(const std::string &name) : Base() {
            this->expose(name);
        }

        Adder(const std::string &prefix,
              const std::string &name) : Base() {
            this->expose_as(prefix, name);
        }

        ~Adder() { Variable::hide(); }
    };

    // turbo::Maxer<int> max_value;
    // max_value << 1 << 2 << 3 << 4;
    // LOG(INFO) << max_value.get_value(); // 4
    namespace var_internal {
        template<typename Tp>
        struct MaxTo {
            void operator()(Tp &lhs,
                            typename add_cr_non_integral<Tp>::type rhs) const {
                // Use operator< as well.
                if (lhs < rhs) {
                    lhs = rhs;
                }
            }
        };

        class LatencyRecorderBase;
    }
    template<typename T>
    class Maxer : public Reducer<T, var_internal::MaxTo<T> > {
    public:
        typedef Reducer<T, var_internal::MaxTo<T> > Base;
        typedef T value_type;
        typedef typename Base::sampler_type sampler_type;
    public:
        Maxer() : Base(std::numeric_limits<T>::min()) {}

        explicit Maxer(const std::string &name)
                : Base(std::numeric_limits<T>::min()) {
            this->expose(name);
        }

        Maxer(const std::string &prefix, const std::string &name)
                : Base(std::numeric_limits<T>::min()) {
            this->expose_as(prefix, name);
        }

        ~Maxer() { Variable::hide(); }

    private:
        friend class var_internal::LatencyRecorderBase;

        // The following private funcition a now used in LatencyRecorder,
        // it's dangerous so we don't make them public
        explicit Maxer(T default_value) : Base(default_value) {
        }

        Maxer(T default_value, const std::string &prefix,
              const std::string &name)
                : Base(default_value) {
            this->expose_as(prefix, name);
        }

        Maxer(T default_value, const std::string &name) : Base(default_value) {
            this->expose(name);
        }
    };

    // turbo::Miner<int> min_value;
    // min_value << 1 << 2 << 3 << 4;
    // LOG(INFO) << min_value.get_value(); // 1
    namespace var_internal {

        template<typename Tp>
        struct MinTo {
            void operator()(Tp &lhs,
                            typename add_cr_non_integral<Tp>::type rhs) const {
                if (rhs < lhs) {
                    lhs = rhs;
                }
            }
        };

    }  // namespace detail

    template<typename T>
    class Miner : public Reducer<T, var_internal::MinTo<T> > {
    public:
        typedef Reducer<T, var_internal::MinTo<T> > Base;
        typedef T value_type;
        typedef typename Base::sampler_type sampler_type;
    public:
        Miner() : Base(std::numeric_limits<T>::max()) {}

        explicit Miner(const std::string &name)
                : Base(std::numeric_limits<T>::max()) {
            this->expose(name);
        }

        Miner(const std::string &prefix, const std::string &name)
                : Base(std::numeric_limits<T>::max()) {
            this->expose_as(prefix, name);
        }

        ~Miner() { Variable::hide(); }
    };

}  // namespace turbo

#endif  // TURBO_VAR_REDUCER_H_
