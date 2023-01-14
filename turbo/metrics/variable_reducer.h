
#ifndef  TURBO_VARIABLE_REDUCER_H_
#define  TURBO_VARIABLE_REDUCER_H_

#include <limits>                                 // std::numeric_limits
#include "turbo/log/logging.h"                         // TURBO_LOG()
#include "turbo/base/type_traits.h"                     // turbo::base::add_cr_non_integral
#include "turbo/base/class_name.h"                      // class_name_str
#include "turbo/metrics/variable_base.h"                        // variable_base
#include "turbo/metrics/detail/combiner.h"                 // metrics_detail::agent_combiner
#include "turbo/metrics/detail/sampler.h"                  // reducer_sampler
#include "turbo/metrics/detail/series.h"
#include "turbo/metrics/window.h"

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
    // variable_reducer works for non-primitive T which satisfies:
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
    // TURBO_LOG(INFO) << my_type_sum;  // "MyType{6}"

    template<typename T, typename Op, typename InvOp = metrics_detail::void_op>
    class variable_reducer : public variable_base {
    public:
        typedef typename metrics_detail::agent_combiner<T, T, Op> combiner_type;
        typedef typename combiner_type::Agent agent_type;
        typedef metrics_detail::reducer_sampler<variable_reducer, T, Op, InvOp> sampler_type;

        class series_sampler : public metrics_detail::variable_sampler {
        public:
            series_sampler(variable_reducer *owner, const Op &op)
                    : _owner(owner), _series(op) {}

            ~series_sampler() {}

            void take_sample() override { _series.append(_owner->get_value()); }

            void describe(std::ostream &os) { _series.describe(os, nullptr); }

        private:
            variable_reducer *_owner;
            metrics_detail::series<T, Op> _series;
        };

    public:
        // The `identify' must satisfy: identity Op a == a
        variable_reducer(typename turbo::add_cr_non_integral<T>::type identity = T(),
                const Op &op = Op(),
                const InvOp &inv_op = InvOp())
                : _combiner(identity, identity, op), _sampler(nullptr), _series_sampler(nullptr), _inv_op(inv_op) {
        }

        ~variable_reducer() {
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
        variable_reducer &operator<<(typename turbo::add_cr_non_integral<T>::type value);

        // Get reduced value.
        // Notice that this function walks through threads that ever add values
        // into this reducer. You should avoid calling it frequently.
        T get_value() const {
            TURBO_CHECK(!(std::is_same<InvOp, metrics_detail::void_op>::value) || _sampler == nullptr)
                            << "You should not call variable_reducer<" << turbo::base::class_name_str<T>()
                            << ", " << turbo::base::class_name_str<Op>() << ">::get_value() when a"
                            << " window<> is used because the operator does not have inverse.";
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

        int describe_series(std::ostream &os, const variable_series_options &options) const override {
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
                        const std::string_view &help,
                        const std::unordered_map<std::string, std::string> &tags,
                        display_filter filter) override {
            const int rc = variable_base::expose_impl(prefix, name, help, tags, filter);
            if (rc == 0 &&
                _series_sampler == nullptr &&
                !std::is_same<InvOp, metrics_detail::void_op>::value &&
                !std::is_same<T, std::string>::value &&
                FLAGS_save_series) {
                _series_sampler = new series_sampler(this, _combiner.op());
                _series_sampler->schedule();
            }
            return rc;
        }

    private:
        combiner_type _combiner;
        sampler_type *_sampler;
        series_sampler *_series_sampler;
        InvOp _inv_op;
    };

    template<typename T, typename Op, typename InvOp>
    inline variable_reducer<T, Op, InvOp> &variable_reducer<T, Op, InvOp>::operator<<(
            typename turbo::add_cr_non_integral<T>::type value) {
        // It's wait-free for most time
        agent_type *agent = _combiner.get_or_create_tls_agent();
        if (__builtin_expect(!agent, 0)) {
            TURBO_LOG(FATAL) << "Fail to create agent";
            return *this;
        }
        agent->element.modify(_combiner.op(), value);
        return *this;
    }

    namespace metrics_detail {
        template<typename Tp>
        struct add_to {
            void operator()(Tp &lhs,
                            typename turbo::add_cr_non_integral<Tp>::type rhs) const { lhs += rhs; }
        };

        template<typename Tp>
        struct minus_from {
            void operator()(Tp &lhs,
                            typename turbo::add_cr_non_integral<Tp>::type rhs) const { lhs -= rhs; }
        };
    }

    namespace metrics_detail {
        template<typename Tp>
        struct max_to {
            void operator()(Tp &lhs,
                            typename turbo::add_cr_non_integral<Tp>::type rhs) const {
                // Use operator< as well.
                if (lhs < rhs) {
                    lhs = rhs;
                }
            }
        };

        class LatencyRecorderBase;

        template<typename Tp>
        struct min_to {
            void operator()(Tp &lhs,
                            typename turbo::add_cr_non_integral<Tp>::type rhs) const {
                if (rhs < lhs) {
                    lhs = rhs;
                }
            }
        };

    }

}  // namespace turbo

#endif  // TURBO_VARIABLE_REDUCER_H_
