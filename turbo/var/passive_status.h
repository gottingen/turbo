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


#ifndef  TURBO_VAR_PASSIVE_STATUS_H_
#define  TURBO_VAR_PASSIVE_STATUS_H_

#include "turbo/var/variable.h"
#include "turbo/var/reducer.h"

namespace turbo {

    template<typename Tp>
    class PassiveStatus : public Variable {
    public:
        typedef Tp value_type;
        typedef var_internal::ReducerSampler<PassiveStatus, Tp, var_internal::AddTo<Tp>,
                var_internal::MinusFrom<Tp> > sampler_type;

        struct PlaceHolderOp {
            void operator()(Tp &, const Tp &) const {}
        };

        static const bool ADDITIVE = (std::is_integral<Tp>::value ||
                                      std::is_floating_point<Tp>::value ||
                                      is_batch<Tp>::value);

        class SeriesSampler : public var_internal::Sampler {
        public:
            typedef typename std::conditional<
                    ADDITIVE, var_internal::AddTo<Tp>, PlaceHolderOp>::type Op;

            explicit SeriesSampler(PassiveStatus *owner)
                    : _owner(owner), _vector_names(NULL), _series(Op()) {}

            ~SeriesSampler() {
                delete _vector_names;
            }

            void take_sample() override { _series.append(_owner->get_value()); }

            void describe(std::ostream &os) { _series.describe(os, _vector_names); }

            void set_vector_names(const std::string &names) {
                if (_vector_names == NULL) {
                    _vector_names = new std::string;
                }
                *_vector_names = names;
            }

        private:
            PassiveStatus *_owner;
            std::string *_vector_names;
            var_internal::Series<Tp, Op> _series;
        };

    public:
        // NOTE: You must be very careful about lifetime of `arg' which should be
        // valid during lifetime of PassiveStatus.
        PassiveStatus(const std::string &name,
                      Tp (*getfn)(void *), void *arg)
                : _getfn(getfn), _arg(arg), _sampler(NULL), _series_sampler(NULL) {
            expose(name);
        }

        PassiveStatus(const std::string &prefix,
                      const std::string &name,
                      Tp (*getfn)(void *), void *arg)
                : _getfn(getfn), _arg(arg), _sampler(NULL), _series_sampler(NULL) {
            expose_as(prefix, name);
        }

        PassiveStatus(Tp (*getfn)(void *), void *arg)
                : _getfn(getfn), _arg(arg), _sampler(NULL), _series_sampler(NULL) {
        }

        ~PassiveStatus() {
            hide();
            if (_sampler) {
                _sampler->destroy();
                _sampler = NULL;
            }
            if (_series_sampler) {
                _series_sampler->destroy();
                _series_sampler = NULL;
            }
        }

        int set_vector_names(const std::string &names) {
            if (_series_sampler) {
                _series_sampler->set_vector_names(names);
                return 0;
            }
            return -1;
        }

        void describe(std::ostream &os, bool /*quote_string*/) const override {
            os << get_value();
        }


        Tp get_value() const {
            return (_getfn ? _getfn(_arg) : Tp());
        }

        sampler_type *get_sampler() {
            if (NULL == _sampler) {
                _sampler = new sampler_type(this);
                _sampler->schedule();
            }
            return _sampler;
        }

        var_internal::AddTo<Tp> op() const { return var_internal::AddTo<Tp>(); }

        var_internal::MinusFrom<Tp> inv_op() const { return var_internal::MinusFrom<Tp>(); }

        int describe_series(std::ostream &os, const SeriesOptions &options) const override {
            if (_series_sampler == NULL) {
                return 1;
            }
            if (!options.test_only) {
                _series_sampler->describe(os);
            }
            return 0;
        }

        Tp reset() {
            TURBO_RAW_LOG(FATAL, "PassiveStatus::reset() should never be called, abort");
            abort();
        }

    protected:
        int expose_impl(const std::string_view &prefix,
                        const std::string_view &name,
                        DisplayFilter display_filter) override {
            const int rc = Variable::expose_impl(prefix, name, display_filter);
            if (ADDITIVE &&
                rc == 0 &&
                _series_sampler == NULL &&
                get_flag(FLAGS_var_save_series)) {
                _series_sampler = new SeriesSampler(this);
                _series_sampler->schedule();
            }
            return rc;
        }

    private:
        Tp (*_getfn)(void *);

        void *_arg;
        sampler_type *_sampler;
        SeriesSampler *_series_sampler;
    };

// ccover g++ may complain about ADDITIVE is undefined unless it's 
// explicitly declared here.
    template<typename Tp> const bool PassiveStatus<Tp>::ADDITIVE;

// Specialize std::string for using std::ostream& as a more friendly
// interface for user's callback.
    template<>
    class PassiveStatus<std::string> : public Variable {
    public:
        // NOTE: You must be very careful about lifetime of `arg' which should be
        // valid during lifetime of PassiveStatus.
        PassiveStatus(const std::string &name,
                      void (*print)(std::ostream &, void *), void *arg)
                : _print(print), _arg(arg) {
            expose(name);
        }

        PassiveStatus(const std::string &prefix,
                      const std::string &name,
                      void (*print)(std::ostream &, void *), void *arg)
                : _print(print), _arg(arg) {
            expose_as(prefix, name);
        }

        PassiveStatus(void (*print)(std::ostream &, void *), void *arg)
                : _print(print), _arg(arg) {}

        ~PassiveStatus() {
            hide();
        }

        void describe(std::ostream &os, bool quote_string) const override {
            if (quote_string) {
                if (_print) {
                    os << '"';
                    _print(os, _arg);
                    os << '"';
                } else {
                    os << "\"null\"";
                }
            } else {
                if (_print) {
                    _print(os, _arg);
                } else {
                    os << "null";
                }
            }
        }

    private:
        void (*_print)(std::ostream &, void *);

        void *_arg;
    };

    template<typename Tp>
    class BasicPassiveStatus : public PassiveStatus<Tp> {
    public:
        BasicPassiveStatus(const std::string &name,
                           Tp (*getfn)(void *), void *arg)
                : PassiveStatus<Tp>(name, getfn, arg) {}

        BasicPassiveStatus(const std::string &prefix,
                           const std::string &name,
                           Tp (*getfn)(void *), void *arg)
                : PassiveStatus<Tp>(prefix, name, getfn, arg) {}

        BasicPassiveStatus(Tp (*getfn)(void *), void *arg)
                : PassiveStatus<Tp>(getfn, arg) {}
    };

    template<>
    class BasicPassiveStatus<std::string> : public PassiveStatus<std::string> {
    public:
        BasicPassiveStatus(const std::string &name,
                           void (*print)(std::ostream &, void *), void *arg)
                : PassiveStatus<std::string>(name, print, arg) {}

        BasicPassiveStatus(const std::string &prefix,
                           const std::string &name,
                           void (*print)(std::ostream &, void *), void *arg)
                : PassiveStatus<std::string>(prefix, name, print, arg) {}

        BasicPassiveStatus(void (*print)(std::ostream &, void *), void *arg)
                : PassiveStatus<std::string>(print, arg) {}
    };


}  // namespace turbo

#endif  // TURBO_VAR_PASSIVE_STATUS_H
