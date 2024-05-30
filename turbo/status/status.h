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
//
// -----------------------------------------------------------------------------
// File: status.h
// -----------------------------------------------------------------------------
//
// This header file defines the Turbo `status` library, consisting of:
//
//   * An `turbo::Status` class for holding error handling information
//   * A set of canonical `turbo::StatusCode` error codes, and associated
//     utilities for generating and propagating status codes.
//   * A set of helper functions for creating status codes and checking their
//     values
//
// Within Google, `turbo::Status` is the primary mechanism for communicating
// errors in C++, and is used to represent error state in both in-process
// library calls as well as RPC calls. Some of these errors may be recoverable,
// but others may not. Most functions that can produce a recoverable error
// should be designed to return an `turbo::Status` (or `turbo::Result`).
//
// Example:
//
// turbo::Status myFunction(turbo::string_view fname, ...) {
//   ...
//   // encounter error
//   if (error condition) {
//     return turbo::invalid_argument_error("bad mode");
//   }
//   // else, return OK
//   return turbo::OkStatus();
// }
//
// An `turbo::Status` is designed to either return "OK" or one of a number of
// different error codes, corresponding to typical error conditions.
// In almost all cases, when using `turbo::Status` you should use the canonical
// error codes (of type `turbo::StatusCode`) enumerated in this header file.
// These canonical codes are understood across the codebase and will be
// accepted across all API and RPC boundaries.
#pragma once

#include <cassert>
#include <cstdint>
#include <ostream>
#include <string>
#include <utility>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/macros.h>
#include <turbo/base/nullability.h>
#include <turbo/base/optimization.h>
#include <turbo/functional/function_ref.h>
#include <turbo/status/internal/status_internal.h>
#include <turbo/strings/cord.h>
#include <turbo/strings/string_view.h>
#include <turbo/types/optional.h>

namespace turbo {

    // turbo::StatusCode
    //
    // An `turbo::StatusCode` is an enumerated type indicating either no error ("OK")
    // or an error condition. In most cases, an `turbo::Status` indicates a
    // recoverable error, and the purpose of signalling an error is to indicate what
    // action to take in response to that error. These error codes map to the proto
    // RPC error codes indicated in https://cloud.google.com/apis/design/errors.
    //
    // The errors listed below are the canonical errors associated with
    // `turbo::Status` and are used throughout the codebase. As a result, these
    // error codes are somewhat generic.
    //
    // In general, try to return the most specific error that applies if more than
    // one error may pertain. For example, prefer `kOutOfRange` over
    // `kFailedPrecondition` if both codes apply. Similarly prefer `kNotFound` or
    // `kAlreadyExists` over `kFailedPrecondition`.
    //
    // Because these errors may cross RPC boundaries, these codes are tied to the
    // `google.rpc.Code` definitions within
    // https://github.com/googleapis/googleapis/blob/master/google/rpc/code.proto
    // The string value of these RPC codes is denoted within each enum below.
    //
    // If your error handling code requires more context, you can attach payloads
    // to your status. See `turbo::Status::set_payload()` and
    // `turbo::Status::get_payload()` below.
    enum class StatusCode : int {
        // StatusCode::kOk
        //
        // kOK (gRPC code "OK") does not indicate an error; this value is returned on
        // success. It is typical to check for this value before proceeding on any
        // given call across an API or RPC boundary. To check this value, use the
        // `turbo::Status::ok()` member function rather than inspecting the raw code.
        kOk = 0,

        // StatusCode::kCancelled
        //
        // kCancelled (gRPC code "CANCELLED") indicates the operation was cancelled,
        // typically by the caller.
        kCancelled = 1,

        // StatusCode::kUnknown
        //
        // kUnknown (gRPC code "UNKNOWN") indicates an unknown error occurred. In
        // general, more specific errors should be raised, if possible. Errors raised
        // by APIs that do not return enough error information may be converted to
        // this error.
        kUnknown = 2,

        // StatusCode::kInvalidArgument
        //
        // kInvalidArgument (gRPC code "INVALID_ARGUMENT") indicates the caller
        // specified an invalid argument, such as a malformed filename. Note that use
        // of such errors should be narrowly limited to indicate the invalid nature of
        // the arguments themselves. Errors with validly formed arguments that may
        // cause errors with the state of the receiving system should be denoted with
        // `kFailedPrecondition` instead.
        kInvalidArgument = 3,

        // StatusCode::kDeadlineExceeded
        //
        // kDeadlineExceeded (gRPC code "DEADLINE_EXCEEDED") indicates a deadline
        // expired before the operation could complete. For operations that may change
        // state within a system, this error may be returned even if the operation has
        // completed successfully. For example, a successful response from a server
        // could have been delayed long enough for the deadline to expire.
        kDeadlineExceeded = 4,

        // StatusCode::kNotFound
        //
        // kNotFound (gRPC code "NOT_FOUND") indicates some requested entity (such as
        // a file or directory) was not found.
        //
        // `kNotFound` is useful if a request should be denied for an entire class of
        // users, such as during a gradual feature rollout or undocumented allow list.
        // If a request should be denied for specific sets of users, such as through
        // user-based access control, use `kPermissionDenied` instead.
        kNotFound = 5,

        // StatusCode::kAlreadyExists
        //
        // kAlreadyExists (gRPC code "ALREADY_EXISTS") indicates that the entity a
        // caller attempted to create (such as a file or directory) is already
        // present.
        kAlreadyExists = 6,

        // StatusCode::kPermissionDenied
        //
        // kPermissionDenied (gRPC code "PERMISSION_DENIED") indicates that the caller
        // does not have permission to execute the specified operation. Note that this
        // error is different than an error due to an *un*authenticated user. This
        // error code does not imply the request is valid or the requested entity
        // exists or satisfies any other pre-conditions.
        //
        // `kPermissionDenied` must not be used for rejections caused by exhausting
        // some resource. Instead, use `kResourceExhausted` for those errors.
        // `kPermissionDenied` must not be used if the caller cannot be identified.
        // Instead, use `kUnauthenticated` for those errors.
        kPermissionDenied = 7,

        // StatusCode::kResourceExhausted
        //
        // kResourceExhausted (gRPC code "RESOURCE_EXHAUSTED") indicates some resource
        // has been exhausted, perhaps a per-user quota, or perhaps the entire file
        // system is out of space.
        kResourceExhausted = 8,

        // StatusCode::kFailedPrecondition
        //
        // kFailedPrecondition (gRPC code "FAILED_PRECONDITION") indicates that the
        // operation was rejected because the system is not in a state required for
        // the operation's execution. For example, a directory to be deleted may be
        // non-empty, an "rmdir" operation is applied to a non-directory, etc.
        //
        // Some guidelines that may help a service implementer in deciding between
        // `kFailedPrecondition`, `kAborted`, and `kUnavailable`:
        //
        //  (a) Use `kUnavailable` if the client can retry just the failing call.
        //  (b) Use `kAborted` if the client should retry at a higher transaction
        //      level (such as when a client-specified test-and-set fails, indicating
        //      the client should restart a read-modify-write sequence).
        //  (c) Use `kFailedPrecondition` if the client should not retry until
        //      the system state has been explicitly fixed. For example, if a "rmdir"
        //      fails because the directory is non-empty, `kFailedPrecondition`
        //      should be returned since the client should not retry unless
        //      the files are deleted from the directory.
        kFailedPrecondition = 9,

        // StatusCode::kAborted
        //
        // kAborted (gRPC code "ABORTED") indicates the operation was aborted,
        // typically due to a concurrency issue such as a sequencer check failure or a
        // failed transaction.
        //
        // See the guidelines above for deciding between `kFailedPrecondition`,
        // `kAborted`, and `kUnavailable`.
        kAborted = 10,

        // StatusCode::kOutOfRange
        //
        // kOutOfRange (gRPC code "OUT_OF_RANGE") indicates the operation was
        // attempted past the valid range, such as seeking or reading past an
        // end-of-file.
        //
        // Unlike `kInvalidArgument`, this error indicates a problem that may
        // be fixed if the system state changes. For example, a 32-bit file
        // system will generate `kInvalidArgument` if asked to read at an
        // offset that is not in the range [0,2^32-1], but it will generate
        // `kOutOfRange` if asked to read from an offset past the current
        // file size.
        //
        // There is a fair bit of overlap between `kFailedPrecondition` and
        // `kOutOfRange`.  We recommend using `kOutOfRange` (the more specific
        // error) when it applies so that callers who are iterating through
        // a space can easily look for an `kOutOfRange` error to detect when
        // they are done.
        kOutOfRange = 11,

        // StatusCode::kUnimplemented
        //
        // kUnimplemented (gRPC code "UNIMPLEMENTED") indicates the operation is not
        // implemented or supported in this service. In this case, the operation
        // should not be re-attempted.
        kUnimplemented = 12,

        // StatusCode::kInternal
        //
        // kInternal (gRPC code "INTERNAL") indicates an internal error has occurred
        // and some invariants expected by the underlying system have not been
        // satisfied. This error code is reserved for serious errors.
        kInternal = 13,

        // StatusCode::kUnavailable
        //
        // kUnavailable (gRPC code "UNAVAILABLE") indicates the service is currently
        // unavailable and that this is most likely a transient condition. An error
        // such as this can be corrected by retrying with a backoff scheme. Note that
        // it is not always safe to retry non-idempotent operations.
        //
        // See the guidelines above for deciding between `kFailedPrecondition`,
        // `kAborted`, and `kUnavailable`.
        kUnavailable = 14,

        // StatusCode::kDataLoss
        //
        // kDataLoss (gRPC code "DATA_LOSS") indicates that unrecoverable data loss or
        // corruption has occurred. As this error is serious, proper alerting should
        // be attached to errors such as this.
        kDataLoss = 15,

        // StatusCode::kUnauthenticated
        //
        // kUnauthenticated (gRPC code "UNAUTHENTICATED") indicates that the request
        // does not have valid authentication credentials for the operation. Correct
        // the authentication and try again.
        kUnauthenticated = 16,

        // StatusCode::DoNotUseReservedForFutureExpansionUseDefaultInSwitchInstead_
        //
        // NOTE: this error code entry should not be used and you should not rely on
        // its value, which may change.
        //
        // The purpose of this enumerated value is to force people who handle status
        // codes with `switch()` statements to *not* simply enumerate all possible
        // values, but instead provide a "default:" case. Providing such a default
        // case ensures that code will compile when new codes are added.
        kDoNotUseReservedForFutureExpansionUseDefaultInSwitchInstead_ = 20
    };

    // StatusCodeToString()
    //
    // Returns the name for the status code, or "" if it is an unknown value.
    std::string StatusCodeToString(StatusCode code);

    // operator<<
    //
    // Streams StatusCodeToString(code) to `os`.
    std::ostream &operator<<(std::ostream &os, StatusCode code);

    // turbo::StatusToStringMode
    //
    // An `turbo::StatusToStringMode` is an enumerated type indicating how
    // `turbo::Status::ToString()` should construct the output string for a non-ok
    // status.
    enum class StatusToStringMode : int {
        // ToString will not contain any extra data (such as payloads). It will only
        // contain the error code and message, if any.
        kWithNoExtraData = 0,
        // ToString will contain the payloads.
        kWithPayload = 1 << 0,
        // ToString will include all the extra data this Status has.
        kWithEverything = ~kWithNoExtraData,
        // Default mode used by ToString. Its exact value might change in the future.
        kDefault = kWithPayload,
    };

    // turbo::StatusToStringMode is specified as a bitmask type, which means the
    // following operations must be provided:
    inline constexpr StatusToStringMode operator&(StatusToStringMode lhs,
                                                  StatusToStringMode rhs) {
        return static_cast<StatusToStringMode>(static_cast<int>(lhs) &
                                               static_cast<int>(rhs));
    }

    inline constexpr StatusToStringMode operator|(StatusToStringMode lhs,
                                                  StatusToStringMode rhs) {
        return static_cast<StatusToStringMode>(static_cast<int>(lhs) |
                                               static_cast<int>(rhs));
    }

    inline constexpr StatusToStringMode operator^(StatusToStringMode lhs,
                                                  StatusToStringMode rhs) {
        return static_cast<StatusToStringMode>(static_cast<int>(lhs) ^
                                               static_cast<int>(rhs));
    }

    inline constexpr StatusToStringMode operator~(StatusToStringMode arg) {
        return static_cast<StatusToStringMode>(~static_cast<int>(arg));
    }

    inline StatusToStringMode &operator&=(StatusToStringMode &lhs,
                                          StatusToStringMode rhs) {
        lhs = lhs & rhs;
        return lhs;
    }

    inline StatusToStringMode &operator|=(StatusToStringMode &lhs,
                                          StatusToStringMode rhs) {
        lhs = lhs | rhs;
        return lhs;
    }

    inline StatusToStringMode &operator^=(StatusToStringMode &lhs,
                                          StatusToStringMode rhs) {
        lhs = lhs ^ rhs;
        return lhs;
    }

    // turbo::Status
    //
    // The `turbo::Status` class is generally used to gracefully handle errors
    // across API boundaries (and in particular across RPC boundaries). Some of
    // these errors may be recoverable, but others may not. Most
    // functions which can produce a recoverable error should be designed to return
    // either an `turbo::Status` (or the similar `turbo::Result<T>`, which holds
    // either an object of type `T` or an error).
    //
    // API developers should construct their functions to return `turbo::OkStatus()`
    // upon success, or an `turbo::StatusCode` upon another type of error (e.g
    // an `turbo::StatusCode::kInvalidArgument` error). The API provides convenience
    // functions to construct each status code.
    //
    // Example:
    //
    // turbo::Status myFunction(turbo::string_view fname, ...) {
    //   ...
    //   // encounter error
    //   if (error condition) {
    //     // Construct an turbo::StatusCode::kInvalidArgument error
    //     return turbo::invalid_argument_error("bad mode");
    //   }
    //   // else, return OK
    //   return turbo::OkStatus();
    // }
    //
    // Users handling status error codes should prefer checking for an OK status
    // using the `ok()` member function. Handling multiple error codes may justify
    // use of switch statement, but only check for error codes you know how to
    // handle; do not try to exhaustively match against all canonical error codes.
    // Errors that cannot be handled should be logged and/or propagated for higher
    // levels to deal with. If you do use a switch statement, make sure that you
    // also provide a `default:` switch case, so that code does not break as other
    // canonical codes are added to the API.
    //
    // Example:
    //
    //   turbo::Status result = DoSomething();
    //   if (!result.ok()) {
    //     LOG(ERROR) << result;
    //   }
    //
    //   // Provide a default if switching on multiple error codes
    //   switch (result.code()) {
    //     // The user hasn't authenticated. Ask them to reauth
    //     case turbo::StatusCode::kUnauthenticated:
    //       DoReAuth();
    //       break;
    //     // The user does not have permission. Log an error.
    //     case turbo::StatusCode::kPermissionDenied:
    //       LOG(ERROR) << result;
    //       break;
    //     // Propagate the error otherwise.
    //     default:
    //       return true;
    //   }
    //
    // An `turbo::Status` can optionally include a payload with more information
    // about the error. Typically, this payload serves one of several purposes:
    //
    //   * It may provide more fine-grained semantic information about the error to
    //     facilitate actionable remedies.
    //   * It may provide human-readable contextual information that is more
    //     appropriate to display to an end user.
    //
    // Example:
    //
    //   turbo::Status result = DoSomething();
    //   // Inform user to retry after 30 seconds
    //   // See more error details in googleapis/google/rpc/error_details.proto
    //   if (turbo::is_resource_exhausted(result)) {
    //     google::rpc::RetryInfo info;
    //     info.retry_delay().seconds() = 30;
    //     // Payloads require a unique key (a URL to ensure no collisions with
    //     // other payloads), and an `turbo::Cord` to hold the encoded data.
    //     turbo::string_view url = "type.googleapis.com/google.rpc.RetryInfo";
    //     result.set_payload(url, info.SerializeAsCord());
    //     return result;
    //   }
    //
    // For documentation see https://abseil.io/docs/cpp/guides/status.
    //
    // Returned Status objects may not be ignored. status_internal.h has a forward
    // declaration of the form
    // class TURBO_MUST_USE_RESULT Status;
    class TURBO_ATTRIBUTE_TRIVIAL_ABI Status final {
    public:
        // Constructors

        // This default constructor creates an OK status with no message or payload.
        // Avoid this constructor and prefer explicit construction of an OK status
        // with `turbo::OkStatus()`.
        Status();

        // Creates a status in the canonical error space with the specified
        // `turbo::StatusCode` and error message.  If `code == turbo::StatusCode::kOk`,  // NOLINT
        // `msg` is ignored and an object identical to an OK status is constructed.
        //
        // The `msg` string must be in UTF-8. The implementation may complain (e.g.,  // NOLINT
        // by printing a warning) if it is not.
        Status(turbo::StatusCode code, turbo::string_view msg);

        Status(const Status &);

        Status &operator=(const Status &x);

        // Move operators

        // The moved-from state is valid but unspecified.
        Status(Status &&) noexcept;

        Status &operator=(Status &&) noexcept;

        ~Status();

        // Status::Update()
        //
        // Updates the existing status with `new_status` provided that `this->ok()`.
        // If the existing status already contains a non-OK error, this update has no
        // effect and preserves the current data. Note that this behavior may change
        // in the future to augment a current non-ok status with additional
        // information about `new_status`.
        //
        // `Update()` provides a convenient way of keeping track of the first error
        // encountered.
        //
        // Example:
        //   // Instead of "if (overall_status.ok()) overall_status = new_status"
        //   overall_status.Update(new_status);
        //
        void update(const Status &new_status);

        void update(Status &&new_status);

        // Status::ok()
        //
        // Returns `true` if `this->code()` == `turbo::StatusCode::kOk`,
        // indicating the absence of an error.
        // Prefer checking for an OK status using this member function.
        TURBO_MUST_USE_RESULT bool ok() const;

        // Status::code()
        //
        // Returns the canonical error code of type `turbo::StatusCode` of this status.
        turbo::StatusCode code() const;

        // Status::raw_code()
        //
        // Returns a raw (canonical) error code corresponding to the enum value of
        // `google.rpc.Code` definitions within
        // https://github.com/googleapis/googleapis/blob/master/google/rpc/code.proto.
        // These values could be out of the range of canonical `turbo::StatusCode`
        // enum values.
        //
        // NOTE: This function should only be called when converting to an associated
        // wire format. Use `Status::code()` for error handling.
        int raw_code() const;

        // Status::message()
        //
        // Returns the error message associated with this error code, if available.
        // Note that this message rarely describes the error code.  It is not unusual
        // for the error message to be the empty string. As a result, prefer
        // `operator<<` or `Status::ToString()` for debug logging.
        turbo::string_view message() const;

        friend bool operator==(const Status &, const Status &);

        friend bool operator!=(const Status &, const Status &);

        // Status::ToString()
        //
        // Returns a string based on the `mode`. By default, it returns combination of
        // the error code name, the message and any associated payload messages. This
        // string is designed simply to be human readable and its exact format should
        // not be load bearing. Do not depend on the exact format of the result of
        // `ToString()` which is subject to change.
        //
        // The printed code name and the message are generally substrings of the
        // result, and the payloads to be printed use the status payload printer
        // mechanism (which is internal).
        std::string to_string(
                StatusToStringMode mode = StatusToStringMode::kDefault) const;

        // Support `turbo::str_cat`, `turbo::str_format`, etc.
        template<typename Sink>
        friend void turbo_stringify(Sink &sink, const Status &status) {
            sink.Append(status.to_string(StatusToStringMode::kWithEverything));
        }

        // Status::ignore_error()
        //
        // Ignores any errors. This method does nothing except potentially suppress
        // complaints from any tools that are checking that errors are not dropped on
        // the floor.
        void ignore_error() const;

        // swap()
        //
        // Swap the contents of one status with another.
        friend void swap(Status &a, Status &b) noexcept;

        //----------------------------------------------------------------------------
        // Payload Management APIs
        //----------------------------------------------------------------------------

        // A payload may be attached to a status to provide additional context to an
        // error that may not be satisfied by an existing `turbo::StatusCode`.
        // Typically, this payload serves one of several purposes:
        //
        //   * It may provide more fine-grained semantic information about the error
        //     to facilitate actionable remedies.
        //   * It may provide human-readable contextual information that is more
        //     appropriate to display to an end user.
        //
        // A payload consists of a [key,value] pair, where the key is a string
        // referring to a unique "type URL" and the value is an object of type
        // `turbo::Cord` to hold the contextual data.
        //
        // The "type URL" should be unique and follow the format of a URL
        // (https://en.wikipedia.org/wiki/URL) and, ideally, provide some
        // documentation or schema on how to interpret its associated data. For
        // example, the default type URL for a protobuf message type is
        // "type.googleapis.com/packagename.messagename". Other custom wire formats
        // should define the format of type URL in a similar practice so as to
        // minimize the chance of conflict between type URLs.
        // Users should ensure that the type URL can be mapped to a concrete
        // C++ type if they want to deserialize the payload and read it effectively.
        //
        // To attach a payload to a status object, call `Status::set_payload()`,
        // passing it the type URL and an `turbo::Cord` of associated data. Similarly,
        // to extract the payload from a status, call `Status::get_payload()`. You
        // may attach multiple payloads (with differing type URLs) to any given
        // status object, provided that the status is currently exhibiting an error
        // code (i.e. is not OK).

        // Status::get_payload()
        //
        // Gets the payload of a status given its unique `type_url` key, if present.
        turbo::optional<turbo::Cord> get_payload(turbo::string_view type_url) const;

        // Status::set_payload()
        //
        // Sets the payload for a non-ok status using a `type_url` key, overwriting
        // any existing payload for that `type_url`.
        //
        // NOTE: This function does nothing if the Status is ok.
        void set_payload(turbo::string_view type_url, turbo::Cord payload);

        // Status::erase_payload()
        //
        // Erases the payload corresponding to the `type_url` key.  Returns `true` if
        // the payload was present.
        bool erase_payload(turbo::string_view type_url);

        // Status::for_each_payload()
        //
        // Iterates over the stored payloads and calls the
        // `visitor(type_key, payload)` callable for each one.
        //
        // NOTE: The order of calls to `visitor()` is not specified and may change at
        // any time.
        //
        // NOTE: Any mutation on the same 'turbo::Status' object during visitation is
        // forbidden and could result in undefined behavior.
        void for_each_payload(
                turbo::FunctionRef<void(turbo::string_view, const turbo::Cord &)> visitor)
        const;

    private:
        friend Status cancelled_error();

        // Creates a status in the canonical error space with the specified
        // code, and an empty error message.
        explicit Status(turbo::StatusCode code);

        // Underlying constructor for status from a rep_.
        explicit Status(uintptr_t rep) : rep_(rep) {}

        static void Ref(uintptr_t rep);

        static void Unref(uintptr_t rep);

        // REQUIRES: !ok()
        // Ensures rep is not inlined or shared with any other Status.
        static turbo::Nonnull<status_internal::StatusRep *> PrepareToModify(
                uintptr_t rep);

        // MSVC 14.0 limitation requires the const.
        static constexpr const char kMovedFromString[] =
                "Status accessed after move.";

        static turbo::Nonnull<const std::string *> EmptyString();

        static turbo::Nonnull<const std::string *> MovedFromString();

        // Returns whether rep contains an inlined representation.
        // See rep_ for details.
        static constexpr bool IsInlined(uintptr_t rep);

        // Indicates whether this Status was the rhs of a move operation. See rep_
        // for details.
        static constexpr bool IsMovedFrom(uintptr_t rep);

        static constexpr uintptr_t MovedFromRep();

        // Convert between error::Code and the inlined uintptr_t representation used
        // by rep_. See rep_ for details.
        static constexpr uintptr_t CodeToInlinedRep(turbo::StatusCode code);

        static constexpr turbo::StatusCode InlinedRepToCode(uintptr_t rep);

        // Converts between StatusRep* and the external uintptr_t representation used
        // by rep_. See rep_ for details.
        static uintptr_t PointerToRep(status_internal::StatusRep *r);

        static turbo::Nonnull<const status_internal::StatusRep *> RepToPointer(
                uintptr_t r);

        static std::string ToStringSlow(uintptr_t rep, StatusToStringMode mode);

        // Status supports two different representations.
        //  - When the low bit is set it is an inlined representation.
        //    It uses the canonical error space, no message or payload.
        //    The error code is (rep_ >> 2).
        //    The (rep_ & 2) bit is the "moved from" indicator, used in IsMovedFrom().
        //  - When the low bit is off it is an external representation.
        //    In this case all the data comes from a heap allocated Rep object.
        //    rep_ is a status_internal::StatusRep* pointer to that structure.
        uintptr_t rep_;

        friend class status_internal::StatusRep;
    };

    // OkStatus()
    //
    // Returns an OK status, equivalent to a default constructed instance. Prefer
    // usage of `turbo::OkStatus()` when constructing such an OK status.
    Status OkStatus();

    // operator<<()
    //
    // Prints a human-readable representation of `x` to `os`.
    std::ostream &operator<<(std::ostream &os, const Status &x);

    // is_aborted()
    // is_already_exists()
    // is_cancelled()
    // is_data_loss()
    // is_deadline_exceeded()
    // is_failed_precondition()
    // is_internal()
    // is_invalid_argument()
    // is_not_found()
    // is_out_of_range()
    // is_permission_denied()
    // is_resource_exhausted()
    // is_unauthenticated()
    // is_unavailable()
    // is_unimplemented()
    // is_unknown()
    //
    // These convenience functions return `true` if a given status matches the
    // `turbo::StatusCode` error code of its associated function.
    TURBO_MUST_USE_RESULT bool is_aborted(const Status &status);

    TURBO_MUST_USE_RESULT bool is_already_exists(const Status &status);

    TURBO_MUST_USE_RESULT bool is_cancelled(const Status &status);

    TURBO_MUST_USE_RESULT bool is_data_loss(const Status &status);

    TURBO_MUST_USE_RESULT bool is_deadline_exceeded(const Status &status);

    TURBO_MUST_USE_RESULT bool is_failed_precondition(const Status &status);

    TURBO_MUST_USE_RESULT bool is_internal(const Status &status);

    TURBO_MUST_USE_RESULT bool is_invalid_argument(const Status &status);

    TURBO_MUST_USE_RESULT bool is_not_found(const Status &status);

    TURBO_MUST_USE_RESULT bool is_out_of_range(const Status &status);

    TURBO_MUST_USE_RESULT bool is_permission_denied(const Status &status);

    TURBO_MUST_USE_RESULT bool is_resource_exhausted(const Status &status);

    TURBO_MUST_USE_RESULT bool is_unauthenticated(const Status &status);

    TURBO_MUST_USE_RESULT bool is_unavailable(const Status &status);

    TURBO_MUST_USE_RESULT bool is_unimplemented(const Status &status);

    TURBO_MUST_USE_RESULT bool is_unknown(const Status &status);

    // aborted_error()
    // already_exists_error()
    // cancelled_error()
    // data_loss_error()
    // deadline_exceeded_error()
    // failed_precondition_error()
    // internal_error()
    // invalid_argument_error()
    // not_found_error()
    // out_of_range_error()
    // permission_denied_error()
    // resource_exhausted_error()
    // unauthenticated_error()
    // unavailable_error()
    // unimplemented_error()
    // unknown_error()
    //
    // These convenience functions create an `turbo::Status` object with an error
    // code as indicated by the associated function name, using the error message
    // passed in `message`.
    Status aborted_error(turbo::string_view message);

    Status already_exists_error(turbo::string_view message);

    Status cancelled_error(turbo::string_view message);

    Status data_loss_error(turbo::string_view message);

    Status deadline_exceeded_error(turbo::string_view message);

    Status failed_precondition_error(turbo::string_view message);

    Status internal_error(turbo::string_view message);

    Status invalid_argument_error(turbo::string_view message);

    Status not_found_error(turbo::string_view message);

    Status out_of_range_error(turbo::string_view message);

    Status permission_denied_error(turbo::string_view message);

    Status resource_exhausted_error(turbo::string_view message);

    Status unauthenticated_error(turbo::string_view message);

    Status unavailable_error(turbo::string_view message);

    Status unimplemented_error(turbo::string_view message);

    Status unknown_error(turbo::string_view message);

    // ErrnoToStatusCode()
    //
    // Returns the StatusCode for `error_number`, which should be an `errno` value.
    // See https://en.cppreference.com/w/cpp/error/errno_macros and similar
    // references.
    turbo::StatusCode ErrnoToStatusCode(int error_number);

    // ErrnoToStatus()
    //
    // Convenience function that creates a `turbo::Status` using an `error_number`,
    // which should be an `errno` value.
    Status ErrnoToStatus(int error_number, turbo::string_view message);

    //------------------------------------------------------------------------------
    // Implementation details follow
    //------------------------------------------------------------------------------

    inline Status::Status() : Status(turbo::StatusCode::kOk) {}

    inline Status::Status(turbo::StatusCode code) : Status(CodeToInlinedRep(code)) {}

    inline Status::Status(const Status &x) : Status(x.rep_) { Ref(rep_); }

    inline Status &Status::operator=(const Status &x) {
        uintptr_t old_rep = rep_;
        if (x.rep_ != old_rep) {
            Ref(x.rep_);
            rep_ = x.rep_;
            Unref(old_rep);
        }
        return *this;
    }

    inline Status::Status(Status &&x) noexcept: Status(x.rep_) {
        x.rep_ = MovedFromRep();
    }

    inline Status &Status::operator=(Status &&x) noexcept {
        uintptr_t old_rep = rep_;
        if (x.rep_ != old_rep) {
            rep_ = x.rep_;
            x.rep_ = MovedFromRep();
            Unref(old_rep);
        }
        return *this;
    }

    inline void Status::update(const Status &new_status) {
        if (ok()) {
            *this = new_status;
        }
    }

    inline void Status::update(Status &&new_status) {
        if (ok()) {
            *this = std::move(new_status);
        }
    }

    inline Status::~Status() { Unref(rep_); }

    inline bool Status::ok() const {
        return rep_ == CodeToInlinedRep(turbo::StatusCode::kOk);
    }

    inline turbo::StatusCode Status::code() const {
        return status_internal::MapToLocalCode(raw_code());
    }

    inline int Status::raw_code() const {
        if (IsInlined(rep_)) return static_cast<int>(InlinedRepToCode(rep_));
        return static_cast<int>(RepToPointer(rep_)->code());
    }

    inline turbo::string_view Status::message() const {
        return !IsInlined(rep_)
               ? RepToPointer(rep_)->message()
               : (IsMovedFrom(rep_) ? turbo::string_view(kMovedFromString)
                                    : turbo::string_view());
    }

    inline bool operator==(const Status &lhs, const Status &rhs) {
        if (lhs.rep_ == rhs.rep_) return true;
        if (Status::IsInlined(lhs.rep_)) return false;
        if (Status::IsInlined(rhs.rep_)) return false;
        return *Status::RepToPointer(lhs.rep_) == *Status::RepToPointer(rhs.rep_);
    }

    inline bool operator!=(const Status &lhs, const Status &rhs) {
        return !(lhs == rhs);
    }

    inline std::string Status::to_string(StatusToStringMode mode) const {
        return ok() ? "OK" : ToStringSlow(rep_, mode);
    }

    inline void Status::ignore_error() const {
        // no-op
    }

    inline void swap(turbo::Status &a, turbo::Status &b) noexcept {
        using std::swap;
        swap(a.rep_, b.rep_);
    }

    inline turbo::optional<turbo::Cord> Status::get_payload(
            turbo::string_view type_url) const {
        if (IsInlined(rep_)) return turbo::nullopt;
        return RepToPointer(rep_)->get_payload(type_url);
    }

    inline void Status::set_payload(turbo::string_view type_url, turbo::Cord payload) {
        if (ok()) return;
        status_internal::StatusRep *rep = PrepareToModify(rep_);
        rep->set_payload(type_url, std::move(payload));
        rep_ = PointerToRep(rep);
    }

    inline bool Status::erase_payload(turbo::string_view type_url) {
        if (IsInlined(rep_)) return false;
        status_internal::StatusRep *rep = PrepareToModify(rep_);
        auto res = rep->erase_payload(type_url);
        rep_ = res.new_rep;
        return res.erased;
    }

    inline void Status::for_each_payload(
            turbo::FunctionRef<void(turbo::string_view, const turbo::Cord &)> visitor)
    const {
        if (IsInlined(rep_)) return;
        RepToPointer(rep_)->for_each_payload(visitor);
    }

    constexpr bool Status::IsInlined(uintptr_t rep) { return (rep & 1) != 0; }

    constexpr bool Status::IsMovedFrom(uintptr_t rep) { return (rep & 2) != 0; }

    constexpr uintptr_t Status::CodeToInlinedRep(turbo::StatusCode code) {
        return (static_cast<uintptr_t>(code) << 2) + 1;
    }

    constexpr turbo::StatusCode Status::InlinedRepToCode(uintptr_t rep) {
        TURBO_ASSERT(IsInlined(rep));
        return static_cast<turbo::StatusCode>(rep >> 2);
    }

    constexpr uintptr_t Status::MovedFromRep() {
        return CodeToInlinedRep(turbo::StatusCode::kInternal) | 2;
    }

    inline turbo::Nonnull<const status_internal::StatusRep *> Status::RepToPointer(
            uintptr_t rep) {
        assert(!IsInlined(rep));
        return reinterpret_cast<const status_internal::StatusRep *>(rep);
    }

    inline uintptr_t Status::PointerToRep(
            turbo::Nonnull<status_internal::StatusRep *> rep) {
        return reinterpret_cast<uintptr_t>(rep);
    }

    inline void Status::Ref(uintptr_t rep) {
        if (!IsInlined(rep)) RepToPointer(rep)->Ref();
    }

    inline void Status::Unref(uintptr_t rep) {
        if (!IsInlined(rep)) RepToPointer(rep)->Unref();
    }

    inline Status OkStatus() { return Status(); }

    // Creates a `Status` object with the `turbo::StatusCode::kCancelled` error code
    // and an empty message. It is provided only for efficiency, given that
    // message-less kCancelled errors are common in the infrastructure.
    inline Status cancelled_error() { return Status(turbo::StatusCode::kCancelled); }

    // Retrieves a message's status as a null terminated C string. The lifetime of
    // this string is tied to the lifetime of the status object itself.
    //
    // If the status's message is empty, the empty string is returned.
    //
    // StatusMessageAsCStr exists for C support. Use `status.message()` in C++.
    turbo::Nonnull<const char *> StatusMessageAsCStr(
            const Status &status TURBO_ATTRIBUTE_LIFETIME_BOUND);

    namespace internal {

        // Extract Status from Status or Result<T>
        // Useful for the status check macros such as RETURN_NOT_OK.
        inline const Status &generic_to_status(const Status &st) { return st; }

        inline Status generic_to_status(Status &&st) { return std::move(st); }

    }  // namespace internal

}  // namespace turbo

#ifndef TURBO_RETURN_NOT_OK

#define TURBO_RETURN_IF_(condition, status, _) \
  do {                                         \
    if (TURBO_UNLIKELY(condition)) {      \
      return (status);                         \
    }                                          \
  } while (0)

/// \brief Propagate any non-successful Status to the caller
#define TURBO_RETURN_NOT_OK(status)                                   \
  do {                                                                \
    ::collie::Status __s = ::turbo::internal::generic_to_status(status); \
    TURBO_RETURN_IF_(!__s.ok(), __s, TURBO_STRINGIFY(status));        \
  } while (false)

#endif  // TURBO_RETURN_NOT_OK
