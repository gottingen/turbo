//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include <turbo/flags/internal/flag.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <array>
#include <atomic>
#include <memory>
#include <new>
#include <string>
#include <typeinfo>

#include <turbo/base/call_once.h>
#include <turbo/base/casts.h>
#include <turbo/base/config.h>
#include <turbo/base/dynamic_annotations.h>
#include <turbo/base/optimization.h>
#include <turbo/flags/config.h>
#include <turbo/flags/internal/commandlineflag.h>
#include <turbo/flags/usage_config.h>
#include <turbo/memory/memory.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/string_view.h>
#include <turbo/synchronization/mutex.h>

namespace turbo {
    namespace flags_internal {

        // The help message indicating that the commandline flag has been
        // 'stripped'. It will not show up when doing "-help" and its
        // variants. The flag is stripped if TURBO_FLAGS_STRIP_HELP is set to 1
        // before including turbo/flags/flag.h
        const char kStrippedFlagHelp[] = "\001\002\003\004 (unknown) \004\003\002\001";

        namespace {

            // Currently we only validate flag values for user-defined flag types.
            bool ShouldValidateFlagValue(FlagFastTypeId flag_type_id) {
#define DONT_VALIDATE(T, _) \
  if (flag_type_id == base_internal::FastTypeId<T>()) return false;
                TURBO_FLAGS_INTERNAL_SUPPORTED_TYPES(DONT_VALIDATE)
#undef DONT_VALIDATE

                return true;
            }

            // RAII helper used to temporarily unlock and relock `turbo::Mutex`.
            // This is used when we need to ensure that locks are released while
            // invoking user supplied callbacks and then reacquired, since callbacks may
            // need to acquire these locks themselves.
            class MutexRelock {
            public:
                explicit MutexRelock(turbo::Mutex &mu) : mu_(mu) { mu_.Unlock(); }

                ~MutexRelock() { mu_.Lock(); }

                MutexRelock(const MutexRelock &) = delete;

                MutexRelock &operator=(const MutexRelock &) = delete;

            private:
                turbo::Mutex &mu_;
            };

        }  // namespace

        ///////////////////////////////////////////////////////////////////////////////
        // Persistent state of the flag data.

        class FlagImpl;

        class FlagState : public flags_internal::FlagStateInterface {
        public:
            template<typename V>
            FlagState(FlagImpl &flag_impl, const V &v, bool modified,
                      bool on_command_line, int64_t counter)
                    : flag_impl_(flag_impl),
                      value_(v),
                      modified_(modified),
                      on_command_line_(on_command_line),
                      counter_(counter) {}

            ~FlagState() override {
                if (flag_impl_.value_storage_kind() != FlagValueStorageKind::kAlignedBuffer &&
                    flag_impl_.value_storage_kind() != FlagValueStorageKind::kSequenceLocked)
                    return;
                flags_internal::Delete(flag_impl_.op_, value_.heap_allocated);
            }

        private:
            friend class FlagImpl;

            // Restores the flag to the saved state.
            void Restore() const override {
                if (!flag_impl_.RestoreState(*this)) return;

                TURBO_INTERNAL_LOG(INFO,
                                   turbo::str_cat("Restore saved value of ", flag_impl_.name(),
                                                  " to: ", flag_impl_.current_value()));
            }

            // Flag and saved flag data.
            FlagImpl &flag_impl_;

            union SavedValue {
                explicit SavedValue(void *v) : heap_allocated(v) {}

                explicit SavedValue(int64_t v) : one_word(v) {}

                void *heap_allocated;
                int64_t one_word;
            } value_;

            bool modified_;
            bool on_command_line_;
            int64_t counter_;
        };

        ///////////////////////////////////////////////////////////////////////////////
        // Flag implementation, which does not depend on flag value type.

        DynValueDeleter::DynValueDeleter(FlagOpFn op_arg) : op(op_arg) {}

        void DynValueDeleter::operator()(void *ptr) const {
            if (op == nullptr) return;

            Delete(op, ptr);
        }

        void FlagImpl::Init() {
            new(&data_guard_) turbo::Mutex;

            auto def_kind = static_cast<FlagDefaultKind>(def_kind_);

            switch (value_storage_kind()) {
                case FlagValueStorageKind::kValueAndInitBit:
                case FlagValueStorageKind::kOneWordAtomic: {
                    alignas(int64_t) std::array<char, sizeof(int64_t)> buf{};
                    if (def_kind == FlagDefaultKind::kGenFunc) {
                        (*default_value_.gen_func)(buf.data());
                    } else {
                        assert(def_kind != FlagDefaultKind::kDynamicValue);
                        std::memcpy(buf.data(), &default_value_, Sizeof(op_));
                    }
                    if (value_storage_kind() == FlagValueStorageKind::kValueAndInitBit) {
                        // We presume here the memory layout of FlagValueAndInitBit struct.
                        uint8_t initialized = 1;
                        std::memcpy(buf.data() + Sizeof(op_), &initialized,
                                    sizeof(initialized));
                    }
                    // Type can contain valid uninitialized bits, e.g. padding.
                    TURBO_ANNOTATE_MEMORY_IS_INITIALIZED(buf.data(), buf.size());
                    OneWordValue().store(turbo::bit_cast<int64_t>(buf),
                                         std::memory_order_release);
                    break;
                }
                case FlagValueStorageKind::kSequenceLocked: {
                    // For this storage kind the default_value_ always points to gen_func
                    // during initialization.
                    assert(def_kind == FlagDefaultKind::kGenFunc);
                    (*default_value_.gen_func)(AtomicBufferValue());
                    break;
                }
                case FlagValueStorageKind::kAlignedBuffer:
                    // For this storage kind the default_value_ always points to gen_func
                    // during initialization.
                    assert(def_kind == FlagDefaultKind::kGenFunc);
                    (*default_value_.gen_func)(AlignedBufferValue());
                    break;
            }
            seq_lock_.MarkInitialized();
        }

        turbo::Mutex *FlagImpl::DataGuard() const {
            turbo::call_once(const_cast<FlagImpl *>(this)->init_control_, &FlagImpl::Init,
                             const_cast<FlagImpl *>(this));

            // data_guard_ is initialized inside Init.
            return reinterpret_cast<turbo::Mutex *>(&data_guard_);
        }

        void FlagImpl::assert_valid_type(FlagFastTypeId rhs_type_id,
                                       const std::type_info *(*gen_rtti)()) const {
            FlagFastTypeId lhs_type_id = flags_internal::FastTypeId(op_);

            // `rhs_type_id` is the fast type id corresponding to the declaration
            // visible at the call site. `lhs_type_id` is the fast type id
            // corresponding to the type specified in flag definition. They must match
            //  for this operation to be well-defined.
            if (TURBO_LIKELY(lhs_type_id == rhs_type_id)) return;

            const std::type_info *lhs_runtime_type_id =
                    flags_internal::RuntimeTypeId(op_);
            const std::type_info *rhs_runtime_type_id = (*gen_rtti)();

            if (lhs_runtime_type_id == rhs_runtime_type_id) return;

#ifdef TURBO_INTERNAL_HAS_RTTI
            if (*lhs_runtime_type_id == *rhs_runtime_type_id) return;
#endif

            TURBO_INTERNAL_LOG(
                    FATAL, turbo::str_cat("Flag '", name(),
                                          "' is defined as one type and declared as another"));
        }

        std::unique_ptr<void, DynValueDeleter> FlagImpl::make_init_value() const {
            void *res = nullptr;
            switch (DefaultKind()) {
                case FlagDefaultKind::kDynamicValue:
                    res = flags_internal::Clone(op_, default_value_.dynamic_value);
                    break;
                case FlagDefaultKind::kGenFunc:
                    res = flags_internal::Alloc(op_);
                    (*default_value_.gen_func)(res);
                    break;
                default:
                    res = flags_internal::Clone(op_, &default_value_);
                    break;
            }
            return {res, DynValueDeleter{op_}};
        }

        void FlagImpl::StoreValue(const void *src) {
            switch (value_storage_kind()) {
                case FlagValueStorageKind::kValueAndInitBit:
                case FlagValueStorageKind::kOneWordAtomic: {
                    // Load the current value to avoid setting 'init' bit manually.
                    int64_t one_word_val = OneWordValue().load(std::memory_order_acquire);
                    std::memcpy(&one_word_val, src, Sizeof(op_));
                    OneWordValue().store(one_word_val, std::memory_order_release);
                    seq_lock_.IncrementModificationCount();
                    break;
                }
                case FlagValueStorageKind::kSequenceLocked: {
                    seq_lock_.Write(AtomicBufferValue(), src, Sizeof(op_));
                    break;
                }
                case FlagValueStorageKind::kAlignedBuffer:
                    Copy(op_, src, AlignedBufferValue());
                    seq_lock_.IncrementModificationCount();
                    break;
            }
            modified_ = true;
            invoke_callback();
        }

        turbo::string_view FlagImpl::name() const { return name_; }

        std::string FlagImpl::filename() const {
            return flags_internal::GetUsageConfig().normalize_filename(filename_);
        }

        std::string FlagImpl::help() const {
            return HelpSourceKind() == FlagHelpKind::kLiteral ? help_.literal
                                                              : help_.gen_func();
        }

        FlagFastTypeId FlagImpl::type_id() const {
            return flags_internal::FastTypeId(op_);
        }

        int64_t FlagImpl::modification_count() const {
            return seq_lock_.modification_count();
        }

        bool FlagImpl::is_specified_on_commandLine() const {
            turbo::MutexLock l(DataGuard());
            return on_command_line_;
        }

        std::string FlagImpl::default_value() const {
            turbo::MutexLock l(DataGuard());

            auto obj = make_init_value();
            return flags_internal::Unparse(op_, obj.get());
        }

        std::string FlagImpl::current_value() const {
            auto *guard = DataGuard();  // Make sure flag initialized
            switch (value_storage_kind()) {
                case FlagValueStorageKind::kValueAndInitBit:
                case FlagValueStorageKind::kOneWordAtomic: {
                    const auto one_word_val =
                            turbo::bit_cast<std::array<char, sizeof(int64_t)>>(
                                    OneWordValue().load(std::memory_order_acquire));
                    return flags_internal::Unparse(op_, one_word_val.data());
                }
                case FlagValueStorageKind::kSequenceLocked: {
                    std::unique_ptr<void, DynValueDeleter> cloned(flags_internal::Alloc(op_),
                                                                  DynValueDeleter{op_});
                    read_sequence_locked_data(cloned.get());
                    return flags_internal::Unparse(op_, cloned.get());
                }
                case FlagValueStorageKind::kAlignedBuffer: {
                    turbo::MutexLock l(guard);
                    return flags_internal::Unparse(op_, AlignedBufferValue());
                }
            }

            return "";
        }

        void FlagImpl::set_flag_callback(const FlagCallbackFunc mutation_callback) {
            turbo::MutexLock l(DataGuard());

            if (callback_ == nullptr) {
                callback_ = new FlagCallback;
            }
            callback_->func = mutation_callback;

            invoke_callback();
        }

        void FlagImpl::set_validator(const FlagValidatorFunc mutation_callback) {
            turbo::MutexLock l(DataGuard());

            if (validator_ == nullptr) {
                validator_ = new FlagValidator;
            }
            validator_->func = mutation_callback;
        }

        void FlagImpl::invoke_callback() const {
            if (!callback_) return;

            // Make a copy of the C-style function pointer that we are about to invoke
            // before we release the lock guarding it.
            FlagCallbackFunc cb = callback_->func;

            // If the flag has a mutation callback this function invokes it. While the
            // callback is being invoked the primary flag's mutex is unlocked and it is
            // re-locked back after call to callback is completed. Callback invocation is
            // guarded by flag's secondary mutex instead which prevents concurrent
            // callback invocation. Note that it is possible for other thread to grab the
            // primary lock and update flag's value at any time during the callback
            // invocation. This is by design. Callback can get a value of the flag if
            // necessary, but it might be different from the value initiated the callback
            // and it also can be different by the time the callback invocation is
            // completed. Requires that *primary_lock be held in exclusive mode; it may be
            // released and reacquired by the implementation.
            MutexRelock relock(*DataGuard());
            turbo::MutexLock lock(&callback_->guard);
            cb();
        }

        std::unique_ptr<FlagStateInterface> FlagImpl::SaveState() {
            turbo::MutexLock l(DataGuard());

            bool modified = modified_;
            bool on_command_line = on_command_line_;
            switch (value_storage_kind()) {
                case FlagValueStorageKind::kValueAndInitBit:
                case FlagValueStorageKind::kOneWordAtomic: {
                    return turbo::make_unique<FlagState>(
                            *this, OneWordValue().load(std::memory_order_acquire), modified,
                            on_command_line, modification_count());
                }
                case FlagValueStorageKind::kSequenceLocked: {
                    void *cloned = flags_internal::Alloc(op_);
                    // Read is guaranteed to be successful because we hold the lock.
                    bool success =
                            seq_lock_.TryRead(cloned, AtomicBufferValue(), Sizeof(op_));
                    assert(success);
                    static_cast<void>(success);
                    return turbo::make_unique<FlagState>(*this, cloned, modified,
                                                         on_command_line, modification_count());
                }
                case FlagValueStorageKind::kAlignedBuffer: {
                    return turbo::make_unique<FlagState>(
                            *this, flags_internal::Clone(op_, AlignedBufferValue()), modified,
                            on_command_line, modification_count());
                }
            }
            return nullptr;
        }

        bool FlagImpl::RestoreState(const FlagState &flag_state) {
            turbo::MutexLock l(DataGuard());
            if (flag_state.counter_ == modification_count()) {
                return false;
            }

            switch (value_storage_kind()) {
                case FlagValueStorageKind::kValueAndInitBit:
                case FlagValueStorageKind::kOneWordAtomic:
                    StoreValue(&flag_state.value_.one_word);
                    break;
                case FlagValueStorageKind::kSequenceLocked:
                case FlagValueStorageKind::kAlignedBuffer:
                    StoreValue(flag_state.value_.heap_allocated);
                    break;
            }

            modified_ = flag_state.modified_;
            on_command_line_ = flag_state.on_command_line_;

            return true;
        }

        template<typename StorageT>
        StorageT *FlagImpl::OffsetValue() const {
            char *p = reinterpret_cast<char *>(const_cast<FlagImpl *>(this));
            // The offset is deduced via Flag value type specific op_.
            ptrdiff_t offset = flags_internal::ValueOffset(op_);

            return reinterpret_cast<StorageT *>(p + offset);
        }

        void *FlagImpl::AlignedBufferValue() const {
            assert(value_storage_kind() == FlagValueStorageKind::kAlignedBuffer);
            return OffsetValue<void>();
        }

        std::atomic<uint64_t> *FlagImpl::AtomicBufferValue() const {
            assert(value_storage_kind() == FlagValueStorageKind::kSequenceLocked);
            return OffsetValue<std::atomic<uint64_t>>();
        }

        std::atomic<int64_t> &FlagImpl::OneWordValue() const {
            assert(value_storage_kind() == FlagValueStorageKind::kOneWordAtomic ||
                   value_storage_kind() == FlagValueStorageKind::kValueAndInitBit);
            return OffsetValue<FlagOneWordValue>()->value;
        }

        // Attempts to parse supplied `value` string using parsing routine in the `flag`
        // argument. If parsing successful, this function replaces the dst with newly
        // parsed value. In case if any error is encountered in either step, the error
        // message is stored in 'err'
        std::unique_ptr<void, DynValueDeleter> FlagImpl::TryParse(
                turbo::string_view value, std::string &err) const {
            std::unique_ptr<void, DynValueDeleter> tentative_value = make_init_value();

            std::string parse_err;
            if (!flags_internal::Parse(op_, value, tentative_value.get(), &parse_err)) {
                turbo::string_view err_sep = parse_err.empty() ? "" : "; ";
                err = turbo::str_cat("Illegal value '", value, "' specified for flag '",
                                     name(), "'", err_sep, parse_err);
                return nullptr;
            }

            return tentative_value;
        }

        void FlagImpl::Read(void *dst) const {
            auto *guard = DataGuard();  // Make sure flag initialized
            switch (value_storage_kind()) {
                case FlagValueStorageKind::kValueAndInitBit:
                case FlagValueStorageKind::kOneWordAtomic: {
                    const int64_t one_word_val =
                            OneWordValue().load(std::memory_order_acquire);
                    std::memcpy(dst, &one_word_val, Sizeof(op_));
                    break;
                }
                case FlagValueStorageKind::kSequenceLocked: {
                    read_sequence_locked_data(dst);
                    break;
                }
                case FlagValueStorageKind::kAlignedBuffer: {
                    turbo::MutexLock l(guard);
                    flags_internal::CopyConstruct(op_, AlignedBufferValue(), dst);
                    break;
                }
            }
        }

        int64_t FlagImpl::ReadOneWord() const {
            assert(value_storage_kind() == FlagValueStorageKind::kOneWordAtomic ||
                   value_storage_kind() == FlagValueStorageKind::kValueAndInitBit);
            auto *guard = DataGuard();  // Make sure flag initialized
            (void) guard;
            return OneWordValue().load(std::memory_order_acquire);
        }

        bool FlagImpl::ReadOneBool() const {
            assert(value_storage_kind() == FlagValueStorageKind::kValueAndInitBit);
            auto *guard = DataGuard();  // Make sure flag initialized
            (void) guard;
            return turbo::bit_cast<FlagValueAndInitBit<bool>>(
                    OneWordValue().load(std::memory_order_acquire))
                    .value;
        }

        void FlagImpl::read_sequence_locked_data(void *dst) const {
            size_t size = Sizeof(op_);
            // Attempt to read using the sequence lock.
            if (TURBO_LIKELY(seq_lock_.TryRead(dst, AtomicBufferValue(), size))) {
                return;
            }
            // We failed due to contention. Acquire the lock to prevent contention
            // and try again.
            turbo::ReaderMutexLock l(DataGuard());
            bool success = seq_lock_.TryRead(dst, AtomicBufferValue(), size);
            assert(success);
            static_cast<void>(success);
        }

        void FlagImpl::Write(const void *src) {
            turbo::MutexLock l(DataGuard());

            if (ShouldValidateFlagValue(flags_internal::FastTypeId(op_))) {
                std::unique_ptr<void, DynValueDeleter> obj{flags_internal::Clone(op_, src),
                                                           DynValueDeleter{op_}};
                std::string ignored_error;
                std::string src_as_str = flags_internal::Unparse(op_, src);
                if (!flags_internal::Parse(op_, src_as_str, obj.get(), &ignored_error)) {
                    TURBO_INTERNAL_LOG(ERROR, turbo::str_cat("Attempt to set flag '", name(),
                                                             "' to invalid value ", src_as_str));
                }
            }

            StoreValue(src);
        }
        bool FlagImpl::user_validate(turbo::string_view value, std::string *err) const {
            if (validator_ == nullptr) return true;

            // Make a copy of the C-style function pointer that we are about to invoke
            FlagValidatorFunc cb = validator_->func;

            // If the flag has a mutation callback this function invokes it. While the
            // callback is being invoked the primary flag's mutex is unlocked and it is
            // re-locked back after call to callback is completed. Callback invocation is
            // guarded by flag's secondary mutex instead which prevents concurrent
            // callback invocation. Note that it is possible for other thread to grab the
            // primary lock and update flag's value at any time during the callback
            // invocation. This is by design. Callback can get a value of the flag if
            // necessary, but it might be different from the value initiated the callback
            // and it also can be different by the time the callback invocation is
            // completed. Requires that *primary_lock be held in exclusive mode; it may be
            // released and reacquired by the implementation.
            ReaderMutexLock dl(DataGuard());
            MutexLock ulock(&validator_->guard);
            auto r = cb(value, err);
            return r;
        }

        bool FlagImpl::has_user_validator() const  {
            return validator_ != nullptr;
        }

        // Sets the value of the flag based on specified string `value`. If the flag
        // was successfully set to new value, it returns true. Otherwise, sets `err`
        // to indicate the error, leaves the flag unchanged, and returns false. There
        // are three ways to set the flag's value:
        //  * Update the current flag value
        //  * Update the flag's default value
        //  * Update the current flag value if it was never set before
        // The mode is selected based on 'set_mode' parameter.
        bool FlagImpl::parse_from(turbo::string_view value, FlagSettingMode set_mode,
                                 ValueSource source, std::string &err) {
            turbo::MutexLock l(DataGuard());

            switch (set_mode) {
                case SET_FLAGS_VALUE: {
                    // set or modify the flag's value
                    auto tentative_value = TryParse(value, err);
                    if (!tentative_value) return false;

                    StoreValue(tentative_value.get());

                    if (source == kCommandLine) {
                        on_command_line_ = true;
                    }
                    break;
                }
                case SET_FLAG_IF_DEFAULT: {
                    // set the flag's value, but only if it hasn't been set by someone else
                    if (modified_) {
                        // TODO(rogeeff): review and fix this semantic. Currently we do not fail
                        // in this case if flag is modified. This is misleading since the flag's
                        // value is not updated even though we return true.
                        // *err = turbo::str_cat(Name(), " is already set to ",
                        //                     current_value(), "\n");
                        // return false;
                        return true;
                    }
                    auto tentative_value = TryParse(value, err);
                    if (!tentative_value) return false;

                    StoreValue(tentative_value.get());
                    break;
                }
                case SET_FLAGS_DEFAULT: {
                    auto tentative_value = TryParse(value, err);
                    if (!tentative_value) return false;

                    if (DefaultKind() == FlagDefaultKind::kDynamicValue) {
                        void *old_value = default_value_.dynamic_value;
                        default_value_.dynamic_value = tentative_value.release();
                        tentative_value.reset(old_value);
                    } else {
                        default_value_.dynamic_value = tentative_value.release();
                        def_kind_ = static_cast<uint8_t>(FlagDefaultKind::kDynamicValue);
                    }

                    if (!modified_) {
                        // Need to set both default value *and* current, in this case.
                        StoreValue(default_value_.dynamic_value);
                        modified_ = false;
                    }
                    break;
                }
            }

            return true;
        }

        void FlagImpl::check_default_value_parsing_roundtrip() const {
            std::string v = default_value();

            turbo::MutexLock lock(DataGuard());

            auto dst = make_init_value();
            std::string error;
            if (!flags_internal::Parse(op_, v, dst.get(), &error)) {
                TURBO_INTERNAL_LOG(
                        FATAL,
                        turbo::str_cat("Flag ", name(), " (from ", filename(),
                                       "): string form of default value '", v,
                                       "' could not be parsed; error=", error));
            }

            // We do not compare dst to def since parsing/unparsing may make
            // small changes, e.g., precision loss for floating point types.
        }

        bool FlagImpl::validate_input_value(turbo::string_view value) const {
            turbo::MutexLock l(DataGuard());

            auto obj = make_init_value();
            std::string ignored_error;
            return flags_internal::Parse(op_, value, obj.get(), &ignored_error);
        }

    }  // namespace flags_internal
    TURBO_NAMESPACE_END
}  // namespace turbo
