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

#include <turbo/flags/reflection.h>

#include <assert.h>

#include <atomic>
#include <string>

#include <turbo/base/config.h>
#include <turbo/base/no_destructor.h>
#include <turbo/base/thread_annotations.h>
#include <turbo/container/flat_hash_map.h>
#include <turbo/flags/commandlineflag.h>
#include <turbo/flags/internal/private_handle_accessor.h>
#include <turbo/flags/internal/registry.h>
#include <turbo/flags/usage_config.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/string_view.h>
#include <turbo/synchronization/mutex.h>

namespace turbo::flags_internal {

    // --------------------------------------------------------------------
    // FlagRegistry
    //    A FlagRegistry singleton object holds all flag objects indexed by their
    //    names so that if you know a flag's name, you can access or set it. If the
    //    function is named FooLocked(), you must own the registry lock before
    //    calling the function; otherwise, you should *not* hold the lock, and the
    //    function will acquire it itself if needed.
    // --------------------------------------------------------------------

    class FlagRegistry {
    public:
        FlagRegistry() = default;

        ~FlagRegistry() = default;

        // Store a flag in this registry. Takes ownership of *flag.
        void RegisterFlag(CommandLineFlag &flag, const char *filename);

        void Lock()

        TURBO_EXCLUSIVE_LOCK_FUNCTION(lock_) {lock_.Lock();}

        void Unlock()

        TURBO_UNLOCK_FUNCTION(lock_) {lock_.Unlock();}

        // Returns the flag object for the specified name, or nullptr if not found.
        // Will emit a warning if a 'retired' flag is specified.
        CommandLineFlag *FindFlag(turbo::string_view name);

        static FlagRegistry &GlobalRegistry();  // returns a singleton registry

    private:
        friend class flags_internal::FlagSaverImpl;  // reads all the flags in order
        // to copy them
        friend void ForEachFlag(std::function<void(CommandLineFlag &)> visitor);

        friend void FinalizeRegistry();

        // The map from name to flag, for FindFlag().
        using FlagMap = turbo::flat_hash_map<turbo::string_view, CommandLineFlag *>;
        using FlagIterator = FlagMap::iterator;
        using FlagConstIterator = FlagMap::const_iterator;
        FlagMap flags_;
        std::vector<CommandLineFlag *> flat_flags_;
        std::atomic<bool> finalized_flags_{false};

        turbo::Mutex lock_;

        // Disallow
        FlagRegistry(const FlagRegistry &);

        FlagRegistry &operator=(const FlagRegistry &);
    };

    namespace {

        class FlagRegistryLock {
        public:
            explicit FlagRegistryLock(FlagRegistry &fr) : fr_(fr) { fr_.Lock(); }

            ~FlagRegistryLock() { fr_.Unlock(); }

        private:
            FlagRegistry &fr_;
        };

    }  // namespace

    CommandLineFlag *FlagRegistry::FindFlag(turbo::string_view name) {
        if (finalized_flags_.load(std::memory_order_acquire)) {
            // We could save some gcus here if we make `name()` be non-virtual.
            // We could move the `const char*` name to the base class.
            auto it = std::partition_point(
                    flat_flags_.begin(), flat_flags_.end(),
                    [=](CommandLineFlag *f) { return f->name() < name; });
            if (it != flat_flags_.end() && (*it)->name() == name) return *it;
        }

        FlagRegistryLock frl(*this);
        auto it = flags_.find(name);
        return it != flags_.end() ? it->second : nullptr;
    }

    void FlagRegistry::RegisterFlag(CommandLineFlag &flag, const char *filename) {
        if (filename != nullptr &&
            flag.filename() != GetUsageConfig().normalize_filename(filename)) {
            flags_internal::ReportUsageError(
                    turbo::str_cat(
                            "Inconsistency between flag object and registration for flag '",
                            flag.name(),
                            "', likely due to duplicate flags or an ODR violation. Relevant "
                            "files: ",
                            flag.filename(), " and ", filename),
                    true);
            std::exit(1);
        }

        FlagRegistryLock registry_lock(*this);

        std::pair<FlagIterator, bool> ins =
                flags_.insert(FlagMap::value_type(flag.name(), &flag));
        if (ins.second == false) {  // means the name was already in the map
            CommandLineFlag &old_flag = *ins.first->second;
            if (flag.is_retired() != old_flag.is_retired()) {
                // All registrations must agree on the 'retired' flag.
                flags_internal::ReportUsageError(
                        turbo::str_cat(
                                "Retired flag '", flag.name(), "' was defined normally in file '",
                                (flag.is_retired() ? old_flag.filename() : flag.filename()), "'."),
                        true);
            } else if (flags_internal::PrivateHandleAccessor::TypeId(flag) !=
                       flags_internal::PrivateHandleAccessor::TypeId(old_flag)) {
                flags_internal::ReportUsageError(
                        turbo::str_cat("Flag '", flag.name(),
                                       "' was defined more than once but with "
                                       "differing types. Defined in files '",
                                       old_flag.filename(), "' and '", flag.filename(), "'."),
                        true);
            } else if (old_flag.is_retired()) {
                return;
            } else if (old_flag.filename() != flag.filename()) {
                flags_internal::ReportUsageError(
                        turbo::str_cat("Flag '", flag.name(),
                                       "' was defined more than once (in files '",
                                       old_flag.filename(), "' and '", flag.filename(), "')."),
                        true);
            } else {
                flags_internal::ReportUsageError(
                        turbo::str_cat(
                                "Something is wrong with flag '", flag.name(), "' in file '",
                                flag.filename(), "'. One possibility: file '", flag.filename(),
                                "' is being linked both statically and dynamically into this "
                                "executable. e.g. some files listed as srcs to a test and also "
                                "listed as srcs of some shared lib deps of the same test."),
                        true);
            }
            // All cases above are fatal, except for the retired flags.
            std::exit(1);
        }
    }

    FlagRegistry &FlagRegistry::GlobalRegistry() {
        static turbo::NoDestructor<FlagRegistry> global_registry;
        return *global_registry;
    }

    // --------------------------------------------------------------------

    void ForEachFlag(std::function<void(CommandLineFlag &)> visitor) {
        FlagRegistry &registry = FlagRegistry::GlobalRegistry();

        if (registry.finalized_flags_.load(std::memory_order_acquire)) {
            for (const auto &i: registry.flat_flags_) visitor(*i);
        }

        FlagRegistryLock frl(registry);
        for (const auto &i: registry.flags_) visitor(*i.second);
    }

    // --------------------------------------------------------------------

    bool RegisterCommandLineFlag(CommandLineFlag &flag, const char *filename) {
        FlagRegistry::GlobalRegistry().RegisterFlag(flag, filename);
        return true;
    }

    void FinalizeRegistry() {
        auto &registry = FlagRegistry::GlobalRegistry();
        FlagRegistryLock frl(registry);
        if (registry.finalized_flags_.load(std::memory_order_relaxed)) {
            // Was already finalized. Ignore the second time.
            return;
        }
        registry.flat_flags_.reserve(registry.flags_.size());
        for (const auto &f: registry.flags_) {
            registry.flat_flags_.push_back(f.second);
        }
        std::sort(std::begin(registry.flat_flags_), std::end(registry.flat_flags_),
                  [](const CommandLineFlag *lhs, const CommandLineFlag *rhs) {
                      return lhs->name() < rhs->name();
                  });
        registry.flags_.clear();
        registry.finalized_flags_.store(true, std::memory_order_release);
    }

// --------------------------------------------------------------------

    namespace {

// These are only used as constexpr global objects.
// They do not use a virtual destructor to simplify their implementation.
// They are not destroyed except at program exit, so leaks do not matter.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

        class RetiredFlagObj final : public CommandLineFlag {
        public:
            constexpr RetiredFlagObj(const char *name, FlagFastTypeId type_id)
                    : name_(name), type_id_(type_id) {}

        private:
            turbo::string_view name() const override { return name_; }

            std::string filename() const override {
                OnAccess();
                return "RETIRED";
            }

            FlagFastTypeId type_id() const override { return type_id_; }

            std::string help() const override {
                OnAccess();
                return "";
            }

            bool is_retired() const override { return true; }

            bool is_specified_on_commandLine() const override {
                OnAccess();
                return false;
            }

            std::string default_value() const override {
                OnAccess();
                return "";
            }

            std::string current_value() const override {
                OnAccess();
                return "";
            }

            // Any input is valid
            bool validate_input_value(turbo::string_view) const override {
                OnAccess();
                return true;
            }

            std::unique_ptr <flags_internal::FlagStateInterface> SaveState() override {
                return nullptr;
            }

            bool parse_from(turbo::string_view, flags_internal::FlagSettingMode,
                           flags_internal::ValueSource, std::string &) override {
                OnAccess();
                return false;
            }

            void check_default_value_parsing_roundtrip() const override { OnAccess(); }

            void Read(void *) const override { OnAccess(); }

            void OnAccess() const {
                flags_internal::ReportUsageError(
                        turbo::str_cat("Accessing retired flag '", name_, "'"), false);
            }

            // Data members
            const char *const name_;
            const FlagFastTypeId type_id_;
        };

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

    }  // namespace

    void Retire(const char *name, FlagFastTypeId type_id, char *buf) {
        static_assert(sizeof(RetiredFlagObj) == kRetiredFlagObjSize, "");
        static_assert(alignof(RetiredFlagObj) == kRetiredFlagObjAlignment, "");
        auto *flag = ::new(static_cast<void *>(buf))
                flags_internal::RetiredFlagObj(name, type_id);
        FlagRegistry::GlobalRegistry().RegisterFlag(*flag, nullptr);
    }

    // --------------------------------------------------------------------

    class FlagSaverImpl {
    public:
        FlagSaverImpl() = default;

        FlagSaverImpl(const FlagSaverImpl &) = delete;

        void operator=(const FlagSaverImpl &) = delete;

        // Saves the flag states from the flag registry into this object.
        // It's an error to call this more than once.
        void SaveFromRegistry() {
            assert(backup_registry_.empty());  // call only once!
            flags_internal::ForEachFlag([&](CommandLineFlag &flag) {
                if (auto flag_state =
                        flags_internal::PrivateHandleAccessor::SaveState(flag)) {
                    backup_registry_.emplace_back(std::move(flag_state));
                }
            });
        }

        // Restores the saved flag states into the flag registry.
        void RestoreToRegistry() {
            for (const auto &flag_state: backup_registry_) {
                flag_state->Restore();
            }
        }

    private:
        std::vector <std::unique_ptr<flags_internal::FlagStateInterface>>
                backup_registry_;
    };

}  // namespace turbo::flags_internal
namespace turbo {
    FlagSaver::FlagSaver() : impl_(new flags_internal::FlagSaverImpl) {
        impl_->SaveFromRegistry();
    }

    FlagSaver::~FlagSaver() {
        if (!impl_) return;

        impl_->RestoreToRegistry();
        delete impl_;
    }

    // --------------------------------------------------------------------

    CommandLineFlag *find_command_line_flag(turbo::string_view name) {
        if (name.empty()) return nullptr;
        flags_internal::FlagRegistry &registry =
                flags_internal::FlagRegistry::GlobalRegistry();
        return registry.FindFlag(name);
    }

    // --------------------------------------------------------------------

    turbo::flat_hash_map<turbo::string_view, turbo::CommandLineFlag *> get_all_flags() {
        turbo::flat_hash_map<turbo::string_view, turbo::CommandLineFlag *> res;
        flags_internal::ForEachFlag([&](CommandLineFlag &flag) {
            if (!flag.is_retired()) res.insert({flag.name(), &flag});
        });
        return res;
    }
}  // namespace turbo
