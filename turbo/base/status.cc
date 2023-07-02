// Copyright 2022 The Turbo Authors.
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
#include "turbo/base/status.h"

#include <errno.h>

#include <cassert>
#include <utility>

#include "status_payload_printer.h"
#include "turbo/base/internal/raw_logging.h"
#include "turbo/base/internal/strerror.h"
#include "turbo/base/turbo_error.h"
#include "turbo/debugging/stacktrace.h"
#include "turbo/debugging/symbolize.h"
#include "turbo/platform/port.h"
#include "turbo/strings/escaping.h"
#include "turbo/strings/str_split.h"

TURBO_REGISTER_ERRNO(turbo::kOk, "OK");
TURBO_REGISTER_ERRNO(turbo::kCancelled, "CANCELLED");
TURBO_REGISTER_ERRNO(turbo::kUnknown, "UNKNOWN");
TURBO_REGISTER_ERRNO(turbo::kInvalidArgument, "INVALID_ARGUMENT");
TURBO_REGISTER_ERRNO(turbo::kDeadlineExceeded, "DEADLINE_EXCEEDED");
TURBO_REGISTER_ERRNO(turbo::kNotFound, "NOT_FOUND");
TURBO_REGISTER_ERRNO(turbo::kAlreadyExists, "ALREADY_EXISTS");
TURBO_REGISTER_ERRNO(turbo::kPermissionDenied, "PERMISSION_DENIED");
TURBO_REGISTER_ERRNO(turbo::kUnauthenticated, "UNAUTHENTICATED");
TURBO_REGISTER_ERRNO(turbo::kResourceExhausted, "RESOURCE_EXHAUSTED");
TURBO_REGISTER_ERRNO(turbo::kFailedPrecondition, "FAILED_PRECONDITION");
TURBO_REGISTER_ERRNO(turbo::kAborted, "ABORTED");
TURBO_REGISTER_ERRNO(turbo::kOutOfRange, "OUT_OF_RANGE");
TURBO_REGISTER_ERRNO(turbo::kUnimplemented, "UNIMPLEMENTED");
TURBO_REGISTER_ERRNO(turbo::kInternal, "INTERNAL");
TURBO_REGISTER_ERRNO(turbo::kUnavailable, "UNAVAILABLE");
TURBO_REGISTER_ERRNO(turbo::kDataLoss, "DATA_LOSS");

namespace turbo {
    TURBO_NAMESPACE_BEGIN


    std::string StatusCodeToString(StatusCode code) {
        return TurboError(code);
    }

    std::ostream &operator<<(std::ostream &os, StatusCode code) {
        return os << StatusCodeToString(code);
    }

    namespace status_internal {

        static std::optional<size_t> FindPayloadIndexByUrl(
                const Payloads *payloads,
                std::string_view type_url) {
            if (payloads == nullptr)
                return std::nullopt;

            for (size_t i = 0; i < payloads->size(); ++i) {
                if ((*payloads)[i].type_url == type_url) return i;
            }

            return std::nullopt;
        }

        // Convert canonical code to a value known to this binary.
        turbo::StatusCode MapToLocalCode(int value) {
            turbo::StatusCode code = static_cast<turbo::StatusCode>(value);
            switch (code) {
                case turbo::kOk:
                case turbo::kCancelled:
                case turbo::kUnknown:
                case turbo::kInvalidArgument:
                case turbo::kDeadlineExceeded:
                case turbo::kNotFound:
                case turbo::kAlreadyExists:
                case turbo::kPermissionDenied:
                case turbo::kResourceExhausted:
                case turbo::kFailedPrecondition:
                case turbo::kAborted:
                case turbo::kOutOfRange:
                case turbo::kUnimplemented:
                case turbo::kInternal:
                case turbo::kUnavailable:
                case turbo::kDataLoss:
                case turbo::kUnauthenticated:
                    return code;
                default:
                    return turbo::kUnknown;
            }
        }
    }  // namespace status_internal

    std::optional<turbo::Cord> Status::GetPayload(
            std::string_view type_url) const {
        const auto *payloads = GetPayloads();
        std::optional<size_t> index =
                status_internal::FindPayloadIndexByUrl(payloads, type_url);
        if (index.has_value())
            return (*payloads)[index.value()].payload;

        return std::nullopt;
    }

    void Status::SetPayload(std::string_view type_url, turbo::Cord payload) {
        if (ok()) return;

        PrepareToModify();

        status_internal::StatusRep *rep = RepToPointer(rep_);
        if (!rep->payloads) {
            rep->payloads = std::make_unique<status_internal::Payloads>();
        }

        std::optional<size_t> index =
                status_internal::FindPayloadIndexByUrl(rep->payloads.get(), type_url);
        if (index.has_value()) {
            (*rep->payloads)[index.value()].payload = std::move(payload);
            return;
        }

        rep->payloads->push_back({std::string(type_url), std::move(payload)});
    }

    bool Status::ErasePayload(std::string_view type_url) {
        std::optional<size_t> index =
                status_internal::FindPayloadIndexByUrl(GetPayloads(), type_url);
        if (index.has_value()) {
            PrepareToModify();
            GetPayloads()->erase(GetPayloads()->begin() + index.value());
            if (GetPayloads()->empty() && message().empty()) {
                // Special case: If this can be represented inlined, it MUST be
                // inlined (EqualsSlow depends on this behavior).
                StatusCode c = static_cast<StatusCode>(raw_code());
                Unref(rep_);
                rep_ = CodeToInlinedRep(c);
            }
            return true;
        }

        return false;
    }

    void Status::ForEachPayload(
            turbo::FunctionRef<void(std::string_view, const turbo::Cord &)> visitor)
    const {
        if (auto *payloads = GetPayloads()) {
            bool in_reverse =
                    payloads->size() > 1 && reinterpret_cast<uintptr_t>(payloads) % 13 > 6;

            for (size_t index = 0; index < payloads->size(); ++index) {
                const auto &elem =
                        (*payloads)[in_reverse ? payloads->size() - 1 - index : index];

#ifdef NDEBUG
                visitor(elem.type_url, elem.payload);
#else
                // In debug mode invalidate the type url to prevent users from relying on
                // this string lifetime.

                // NOLINTNEXTLINE intentional extra conversion to force temporary.
                visitor(std::string(elem.type_url), elem.payload);
#endif  // NDEBUG
            }
        }
    }

    const std::string *Status::EmptyString() {
        static union EmptyString {
            std::string str;

            ~EmptyString() {}
        } empty = {{}};
        return &empty.str;
    }

#ifndef TURBO_COMPILER_CPP17_ENABLED
    constexpr const char Status::kMovedFromString[];
#endif

    const std::string *Status::MovedFromString() {
        static std::string *moved_from_string = new std::string(kMovedFromString);
        return moved_from_string;
    }

    void Status::UnrefNonInlined(uintptr_t rep) {
        status_internal::StatusRep *r = RepToPointer(rep);
        // Fast path: if ref==1, there is no need for a RefCountDec (since
        // this is the only reference and therefore no other thread is
        // allowed to be mucking with r).
        if (r->ref.load(std::memory_order_acquire) == 1 ||
            r->ref.fetch_sub(1, std::memory_order_acq_rel) - 1 == 0) {
            delete r;
        }
    }

    Status::Status(turbo::StatusCode code, std::string_view msg)
            : rep_(CodeToInlinedRep(code)) {
        if (code != turbo::kOk && !msg.empty()) {
            rep_ = PointerToRep(new status_internal::StatusRep(kTurboModuleIndex, code, msg, nullptr));
        }
    }

    Status::Status(unsigned short int index, turbo::StatusCode code, std::string_view msg)
            : rep_(CodeToInlinedRep(index, code)) {
        if (code != turbo::kOk && !msg.empty()) {
            rep_ = PointerToRep(new status_internal::StatusRep(index, code, msg, nullptr));
        }
    }

    /*
    template <typename... Args>
    Status::Status(unsigned short int module_index, turbo::StatusCode code, const FormatSpec<Args...>& format,
           const Args&... args) {

    }
    */
    int Status::raw_code() const {
        if (IsInlined(rep_)) {
            return static_cast<int>(InlinedRepToCode(rep_));
        }
        status_internal::StatusRep *rep = RepToPointer(rep_);
        return static_cast<int>(rep->code);
    }

    unsigned short int Status::index() const {
        if (IsInlined(rep_)) {
            return static_cast<unsigned short int>(InlinedRepToIndex(rep_));
        }
        status_internal::StatusRep *rep = RepToPointer(rep_);
        return static_cast<unsigned short int>(rep->index);
    }

    turbo::StatusCode Status::code() const {
        //return status_internal::MapToLocalCode(raw_code());
        return raw_code();
    }

    void Status::PrepareToModify() {
        TURBO_RAW_CHECK(!ok(), "PrepareToModify shouldn't be called on OK status.");
        if (IsInlined(rep_)) {
            rep_ = PointerToRep(new status_internal::StatusRep(static_cast<unsigned short int>(index()),
                                                               static_cast<turbo::StatusCode>(raw_code()),
                                                               std::string_view(),
                                                               nullptr));
            return;
        }

        uintptr_t rep_i = rep_;
        status_internal::StatusRep *rep = RepToPointer(rep_);
        if (rep->ref.load(std::memory_order_acquire) != 1) {
            std::unique_ptr<status_internal::Payloads> payloads;
            if (rep->payloads) {
                payloads = std::make_unique<status_internal::Payloads>(*rep->payloads);
            }
            status_internal::StatusRep *const new_rep = new status_internal::StatusRep(
                    rep->index,
                    rep->code, message(), std::move(payloads));
            rep_ = PointerToRep(new_rep);
            UnrefNonInlined(rep_i);
        }
    }

    bool Status::EqualsSlow(const turbo::Status &a, const turbo::Status &b) {
        if (IsInlined(a.rep_) != IsInlined(b.rep_)) return false;
        if (a.message() != b.message()) return false;
        if (a.raw_code() != b.raw_code()) return false;
        if (a.GetPayloads() == b.GetPayloads()) return true;

        const status_internal::Payloads no_payloads;
        const status_internal::Payloads *larger_payloads =
                a.GetPayloads() ? a.GetPayloads() : &no_payloads;
        const status_internal::Payloads *smaller_payloads =
                b.GetPayloads() ? b.GetPayloads() : &no_payloads;
        if (larger_payloads->size() < smaller_payloads->size()) {
            std::swap(larger_payloads, smaller_payloads);
        }
        if ((larger_payloads->size() - smaller_payloads->size()) > 1) return false;
        // Payloads can be ordered differently, so we can't just compare payload
        // vectors.
        for (const auto &payload: *larger_payloads) {

            bool found = false;
            for (const auto &other_payload: *smaller_payloads) {
                if (payload.type_url == other_payload.type_url) {
                    if (payload.payload != other_payload.payload) {
                        return false;
                    }
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }

    std::string Status::ToStringSlow(StatusToStringMode mode) const {
        std::string text;
        const bool with_module = (mode & StatusToStringMode::kWithModule) ==
                                 StatusToStringMode::kWithModule;

        if (with_module) {
            turbo::FormatAppend(&text, "{}::{}: {}", TurboModule(index()),StatusCodeToString(code()), message());
        } else {
            turbo::FormatAppend(&text, "{}: {}", StatusCodeToString(code()), message());
        }

        const bool with_payload = (mode & StatusToStringMode::kWithPayload) ==
                                  StatusToStringMode::kWithPayload;

        if (with_payload) {
            status_internal::StatusPayloadPrinter printer =
                    status_internal::GetStatusPayloadPrinter();
            this->ForEachPayload([&](std::string_view type_url,
                                     const turbo::Cord &payload) {
                std::optional<std::string> result;
                if (printer) result = printer(type_url, payload);
                turbo::FormatAppend(
                        &text, " [{}='{}']", type_url,
                        result.has_value() ? *result : turbo::CHexEscape(std::string(payload)));
            });
        }

        return text;
    }

    std::ostream &operator<<(std::ostream &os, const Status &x) {
        os << x.ToString(StatusToStringMode::kWithEverything);
        return os;
    }

    bool IsAborted(const Status &status) {
        return status.code() == turbo::kAborted;
    }

    bool IsAlreadyExists(const Status &status) {
        return status.code() == turbo::kAlreadyExists;
    }

    bool IsCancelled(const Status &status) {
        return status.code() == turbo::kCancelled;
    }

    bool IsDataLoss(const Status &status) {
        return status.code() == turbo::kDataLoss;
    }

    bool IsDeadlineExceeded(const Status &status) {
        return status.code() == turbo::kDeadlineExceeded;
    }

    bool IsFailedPrecondition(const Status &status) {
        return status.code() == turbo::kFailedPrecondition;
    }

    bool IsInternal(const Status &status) {
        return status.code() == turbo::kInternal;
    }

    bool IsInvalidArgument(const Status &status) {
        return status.code() == turbo::kInvalidArgument;
    }

    bool IsNotFound(const Status &status) {
        return status.code() == turbo::kNotFound;
    }

    bool IsOutOfRange(const Status &status) {
        return status.code() == turbo::kOutOfRange;
    }

    bool IsPermissionDenied(const Status &status) {
        return status.code() == turbo::kPermissionDenied;
    }

    bool IsResourceExhausted(const Status &status) {
        return status.code() == turbo::kResourceExhausted;
    }

    bool IsUnauthenticated(const Status &status) {
        return status.code() == turbo::kUnauthenticated;
    }

    bool IsUnavailable(const Status &status) {
        return status.code() == turbo::kUnavailable;
    }

    bool IsUnimplemented(const Status &status) {
        return status.code() == turbo::kUnimplemented;
    }

    bool IsUnknown(const Status &status) {
        return status.code() == turbo::kUnknown;
    }

    StatusCode ErrnoToStatusCode(int error_number) {
        switch (error_number) {
            case 0:
                return kOk;
            case EINVAL:        // Invalid argument
            case ENAMETOOLONG:  // Filename too long
            case E2BIG:         // Argument list too long
            case EDESTADDRREQ:  // Destination address required
            case EDOM:          // Mathematics argument out of domain of function
            case EFAULT:        // Bad address
            case EILSEQ:        // Illegal byte sequence
            case ENOPROTOOPT:   // Protocol not available
            case ENOSTR:        // Not a STREAM
            case ENOTSOCK:      // Not a socket
            case ENOTTY:        // Inappropriate I/O control operation
            case EPROTOTYPE:    // Protocol wrong type for socket
            case ESPIPE:        // Invalid seek
                return kInvalidArgument;
            case ETIMEDOUT:  // Connection timed out
            case ETIME:      // Timer expired
                return kDeadlineExceeded;
            case ENODEV:  // No such device
            case ENOENT:  // No such file or directory
#ifdef ENOMEDIUM
            case ENOMEDIUM:  // No medium found
#endif
            case ENXIO:  // No such device or address
            case ESRCH:  // No such process
                return kNotFound;
            case EEXIST:         // File exists
            case EADDRNOTAVAIL:  // Address not available
            case EALREADY:       // Connection already in progress
#ifdef ENOTUNIQ
            case ENOTUNIQ:  // Name not unique on network
#endif
                return kAlreadyExists;
            case EPERM:   // Operation not permitted
            case EACCES:  // Permission denied
#ifdef ENOKEY
            case ENOKEY:  // Required key not available
#endif
            case EROFS:  // Read only file system
                return kPermissionDenied;
            case ENOTEMPTY:   // Directory not empty
            case EISDIR:      // Is a directory
            case ENOTDIR:     // Not a directory
            case EADDRINUSE:  // Address already in use
            case EBADF:       // Invalid file descriptor
#ifdef EBADFD
            case EBADFD:  // File descriptor in bad state
#endif
            case EBUSY:    // Device or resource busy
            case ECHILD:   // No child processes
            case EISCONN:  // Socket is connected
#ifdef EISNAM
            case EISNAM:  // Is a named type file
#endif
#ifdef ENOTBLK
            case ENOTBLK:  // Block device required
#endif
            case ENOTCONN:  // The socket is not connected
            case EPIPE:     // Broken pipe
#ifdef ESHUTDOWN
            case ESHUTDOWN:  // Cannot send after transport endpoint shutdown
#endif
            case ETXTBSY:  // Text file busy
#ifdef EUNATCH
            case EUNATCH:  // Protocol driver not attached
#endif
                return kFailedPrecondition;
            case ENOSPC:  // No space left on device
#ifdef EDQUOT
            case EDQUOT:  // Disk quota exceeded
#endif
            case EMFILE:   // Too many open files
            case EMLINK:   // Too many links
            case ENFILE:   // Too many open files in system
            case ENOBUFS:  // No buffer space available
            case ENODATA:  // No message is available on the STREAM read queue
            case ENOMEM:   // Not enough space
            case ENOSR:    // No STREAM resources
#ifdef EUSERS
            case EUSERS:  // Too many users
#endif
                return kResourceExhausted;
#ifdef ECHRNG
            case ECHRNG:  // Channel number out of range
#endif
            case EFBIG:      // File too large
            case EOVERFLOW:  // Value too large to be stored in data type
            case ERANGE:     // Result too large
                return kOutOfRange;
#ifdef ENOPKG
            case ENOPKG:  // Package not installed
#endif
            case ENOSYS:        // Function not implemented
            case ENOTSUP:       // Operation not supported
            case EAFNOSUPPORT:  // Address family not supported
#ifdef EPFNOSUPPORT
            case EPFNOSUPPORT:  // Protocol family not supported
#endif
            case EPROTONOSUPPORT:  // Protocol not supported
#ifdef ESOCKTNOSUPPORT
            case ESOCKTNOSUPPORT:  // Socket type not supported
#endif
            case EXDEV:  // Improper link
                return kUnimplemented;
            case EAGAIN:  // Resource temporarily unavailable
#ifdef ECOMM
            case ECOMM:  // Communication error on send
#endif
            case ECONNREFUSED:  // Connection refused
            case ECONNABORTED:  // Connection aborted
            case ECONNRESET:    // Connection reset
            case EINTR:         // Interrupted function call
#ifdef EHOSTDOWN
            case EHOSTDOWN:  // Host is down
#endif
            case EHOSTUNREACH:  // Host is unreachable
            case ENETDOWN:      // Network is down
            case ENETRESET:     // Connection aborted by network
            case ENETUNREACH:   // Network unreachable
            case ENOLCK:        // No locks available
            case ENOLINK:       // Link has been severed
#ifdef ENONET
            case ENONET:  // Machine is not on the network
#endif
                return kUnavailable;
            case EDEADLK:  // Resource deadlock avoided
#ifdef ESTALE
            case ESTALE:  // Stale file handle
#endif
                return kAborted;
            case ECANCELED:  // Operation cancelled
                return kCancelled;
            default:
                return kUnknown;
        }
    }

    namespace {
        std::string MessageForErrnoToStatus(int error_number,
                                            std::string_view message) {
            return turbo::Format("{}: {}", message,
                                 turbo::base_internal::StrError(error_number));
        }
    }  // namespace

    Status ErrnoToStatus(int error_number, std::string_view message) {
        return Status(ErrnoToStatusCode(error_number),
                      MessageForErrnoToStatus(error_number, message));
    }

    namespace status_internal {

        std::string *MakeCheckFailString(const turbo::Status *status,
                                         const char *prefix) {
            return new std::string(
                    turbo::Format("{} ({})", prefix,
                                  status->ToString(StatusToStringMode::kWithEverything)));
        }

    }  // namespace status_internal

    TURBO_NAMESPACE_END
}  // namespace turbo
