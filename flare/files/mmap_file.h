

/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_FILES_MMAP_FILE_H_
#define FLARE_FILES_MMAP_FILE_H_

#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <iterator>
#include <string>
#include <system_error>
#include <type_traits>
#include <cstdint>
#include <memory> // std::shared_ptr
#include "flare/system/sysinfo.h"
#include "flare/base/result_status.h"
#include "flare/files/constants.h"
#include "flare/log/logging.h"
#include "flare/files/filesystem.h"

namespace flare {
    /**
     * This is used by `basic_mmap` to determine whether to create a read-only or
     * a read-write memory mapping.
     */
    enum class access_mode {
        read,
        write
    };

    /**
     * Alligns `offset` to the operating's system page size such that it subtracts the
     * difference until the nearest page boundary before `offset`, or does nothing if
     * `offset` is already page aligned.
     */
    inline size_t make_offset_page_aligned(size_t offset) noexcept {
        const size_t page_size_ = sysinfo::get_page_size();
        // Use integer division to round down to the nearest page alignment.
        return offset / page_size_ * page_size_;
    }

    // This value may be provided as the `length` parameter to the constructor or
    // `map`, in which case a memory mapping of the entire file is created.
    enum {
        map_entire_file = 0
    };


    template<access_mode AccessMode, typename ByteT>
    class basic_mmap {
    public:
        using value_type = ByteT;
        using size_type = size_t;
        using reference = value_type &;
        using const_reference = const value_type &;
        using pointer = value_type *;
        using const_pointer = const value_type *;
        using difference_type = std::ptrdiff_t;
        using iterator = pointer;
        using const_iterator = const_pointer;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using iterator_category = std::random_access_iterator_tag;
        using handle_type = file_handle_type;

        static_assert(sizeof(ByteT) == sizeof(char), "ByteT must be the same size as char.");

    private:
        // Points to the first requested byte, and not to the actual start of the mapping.
        pointer data_ = nullptr;

        // Length--in bytes--requested by user (which may not be the length of the
        // full mapping) and the length of the full mapping.
        size_type length_ = 0;
        size_type mapped_length_ = 0;

        // Letting user map a file using both an existing file handle and a path
        // introcudes some complexity (see `is_handle_internal_`).
        // On POSIX, we only need a file handle to create a mapping, while on
        // Windows systems the file handle is necessary to retrieve a file mapping
        // handle, but any subsequent operations on the mapped region must be done
        // through the latter.
        handle_type file_handle_ = invalid_handle;

        // Letting user map a file using both an existing file handle and a path
        // introcudes some complexity in that we must not close the file handle if
        // user provided it, but we must close it if we obtained it using the
        // provided path. For this reason, this flag is used to determine when to
        // close `file_handle_`.
        bool is_handle_internal_ = true;

    public:
        /**
         * The default constructed mmap object is in a non-mapped state, that is,
         * any operation that attempts to access nonexistent underlying data will
         * result in undefined behaviour/segmentation faults.
         */
        basic_mmap() = default;

        /**
         * `basic_mmap` has single-ownership semantics, so transferring ownership
         * may only be accomplished by moving the object.
         */
        basic_mmap(const basic_mmap &) = delete;

        basic_mmap(basic_mmap &&) noexcept;

        basic_mmap &operator=(const basic_mmap &) = delete;

        basic_mmap &operator=(basic_mmap &&);

        /**
         * If this is a read-write mapping, the destructor invokes sync. Regardless
         * of the access mode, unmap is invoked as a final step.
         */
        ~basic_mmap();

        /**
         * On UNIX systems 'file_handle' and 'mapping_handle' are the same. On Windows,
         * however, a mapped region of a file gets its own handle, which is returned by
         * 'mapping_handle'.
         */
        [[nodiscard]] handle_type file_handle() const noexcept { return file_handle_; }

        [[nodiscard]] handle_type mapping_handle() const noexcept;

        /** Returns whether a valid memory mapping has been created. */
        [[nodiscard]] bool is_open() const noexcept { return file_handle_ != invalid_handle; }

        /**
         * Returns true if no mapping was established, that is, conceptually the
         * same as though the length that was mapped was 0. This function is
         * provided so that this class has Container semantics.
         */
        [[nodiscard]] bool empty() const noexcept { return length() == 0; }

        /** Returns true if a mapping was established. */
        [[nodiscard]] bool is_mapped() const noexcept;

        /**
         * `size` and `length` both return the logical length, i.e. the number of bytes
         * user requested to be mapped, while `mapped_length` returns the actual number of
         * bytes that were mapped which is a multiple of the underlying operating system's
         * page allocation granularity.
         */
        [[nodiscard]] size_type size() const noexcept { return length(); }

        [[nodiscard]] size_type length() const noexcept { return length_; }

        [[nodiscard]] size_type mapped_length() const noexcept { return mapped_length_; }

        /** Returns the offset relative to the start of the mapping. */
        [[nodiscard]] size_type mapping_offset() const noexcept {
            return mapped_length_ - length_;
        }

        /**
         * Returns a pointer to the first requested byte, or `nullptr` if no memory mapping
         * exists.
         */
        template<
                access_mode A = AccessMode,
                typename = typename std::enable_if<A == access_mode::write>::type
        >
        pointer data() noexcept { return data_; }

        [[nodiscard]] const_pointer data() const noexcept { return data_; }

        /**
         * Returns an iterator to the first requested byte, if a valid memory mapping
         * exists, otherwise this function call is undefined behaviour.
         */
        template<
                access_mode A = AccessMode,
                typename = typename std::enable_if<A == access_mode::write>::type
        >
        iterator begin() noexcept { return data(); }

        [[nodiscard]] const_iterator begin() const noexcept { return data(); }

        [[nodiscard]] const_iterator cbegin() const noexcept { return data(); }

        /**
         * Returns an iterator one past the last requested byte, if a valid memory mapping
         * exists, otherwise this function call is undefined behaviour.
         */
        template<
                access_mode A = AccessMode,
                typename = typename std::enable_if<A == access_mode::write>::type
        >
        iterator end() noexcept { return data() + length(); }

        [[nodiscard]] const_iterator end() const noexcept { return data() + length(); }

        [[nodiscard]] const_iterator cend() const noexcept { return data() + length(); }

        /**
         * Returns a reverse iterator to the last memory mapped byte, if a valid
         * memory mapping exists, otherwise this function call is undefined
         * behaviour.
         */
        template<
                access_mode A = AccessMode,
                typename = typename std::enable_if<A == access_mode::write>::type
        >
        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

        [[nodiscard]] const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }

        [[nodiscard]] const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

        /**
         * Returns a reverse iterator past the first mapped byte, if a valid memory
         * mapping exists, otherwise this function call is undefined behaviour.
         */
        template<
                access_mode A = AccessMode,
                typename = typename std::enable_if<A == access_mode::write>::type
        >
        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

        [[nodiscard]] const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

        [[nodiscard]] const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

        /**
         * Returns a reference to the `i`th byte from the first requested byte (as returned
         * by `data`). If this is invoked when no valid memory mapping has been created
         * prior to this call, undefined behaviour ensues.
         */
        reference operator[](const size_type i) noexcept { return data_[i]; }

        const_reference operator[](const size_type i) const noexcept { return data_[i]; }

        /**
         * Establishes a memory mapping with AccessMode. If the mapping is unsuccesful, the
         * reason is reported via `error` and the object remains in a state as if this
         * function hadn't been called.
         *
         * `path`, which must be a path to an existing file, is used to retrieve a file
         * handle (which is closed when the object destructs or `unmap` is called), which is
         * then used to memory map the requested region. Upon failure, `error` is set to
         * indicate the reason and the object remains in an unmapped state.
         *
         * `offset` is the number of bytes, relative to the start of the file, where the
         * mapping should begin. When specifying it, there is no need to worry about
         * providing a value that is aligned with the operating system's page allocation
         * granularity. This is adjusted by the implementation such that the first requested
         * byte (as returned by `data` or `begin`), so long as `offset` is valid, will be at
         * `offset` from the start of the file.
         *
         * `length` is the number of bytes to map. It may be `map_entire_file`, in which
         * case a mapping of the entire file is created.
         */
        template<typename String>
        [[nodiscard]] result_status open(const String &path, const size_type offset,
                                         const size_type length);

        /**
         * Establishes a memory mapping with AccessMode. If the mapping is unsuccesful, the
         * reason is reported via `error` and the object remains in a state as if this
         * function hadn't been called.
         *
         * `path`, which must be a path to an existing file, is used to retrieve a file
         * handle (which is closed when the object destructs or `unmap` is called), which is
         * then used to memory map the requested region. Upon failure, `error` is set to
         * indicate the reason and the object remains in an unmapped state.
         *
         * The entire file is mapped.
         */
        template<typename String>
        [[nodiscard]] result_status open(const String &path) {
            return open(path, 0, map_entire_file);
        }

        /**
         * Establishes a memory mapping with AccessMode. If the mapping is
         * unsuccesful, the reason is reported via `error` and the object remains in
         * a state as if this function hadn't been called.
         *
         * `handle`, which must be a valid file handle, which is used to memory map the
         * requested region. Upon failure, `error` is set to indicate the reason and the
         * object remains in an unmapped state.
         *
         * `offset` is the number of bytes, relative to the start of the file, where the
         * mapping should begin. When specifying it, there is no need to worry about
         * providing a value that is aligned with the operating system's page allocation
         * granularity. This is adjusted by the implementation such that the first requested
         * byte (as returned by `data` or `begin`), so long as `offset` is valid, will be at
         * `offset` from the start of the file.
         *
         * `length` is the number of bytes to map. It may be `map_entire_file`, in which
         * case a mapping of the entire file is created.
         */
        [[nodiscard]] result_status open(const handle_type handle, const size_type offset, const size_type length);

        /**
         * Establishes a memory mapping with AccessMode. If the mapping is
         * unsuccesful, the reason is reported via `error` and the object remains in
         * a state as if this function hadn't been called.
         *
         * `handle`, which must be a valid file handle, which is used to memory map the
         * requested region. Upon failure, `error` is set to indicate the reason and the
         * object remains in an unmapped state.
         *
         * The entire file is mapped.
         */
        [[nodiscard]] result_status open(const handle_type handle) {
            return open(handle, 0, map_entire_file);
        }

        /**
         * If a valid memory mapping has been created prior to this call, this call
         * instructs the kernel to unmap the memory region and disassociate this object
         * from the file.
         *
         * The file handle associated with the file that is mapped is only closed if the
         * mapping was created using a file path. If, on the other hand, an existing
         * file handle was used to create the mapping, the file handle is not closed.
         */
        void unmap();

        void swap(basic_mmap &other);

        /** Flushes the memory mapped page to disk. Errors are reported via `error`. */
        template<access_mode A = AccessMode>
        [[nodiscard]] typename std::enable_if<A == access_mode::write, result_status>::type
        sync();

        /**
         * All operators compare the address of the first byte and size of the two mapped
         * regions.
         */

    private:

        template<
                access_mode A = AccessMode,
                typename = typename std::enable_if<A == access_mode::write>::type
        >
        pointer get_mapping_start() noexcept {
            return !data() ? nullptr : data() - mapping_offset();
        }

        [[nodiscard]] const_pointer get_mapping_start() const noexcept {
            return !data() ? nullptr : data() - mapping_offset();
        }

        /**
         * The destructor syncs changes to disk if `AccessMode` is `write`, but not
         * if it's `read`, but since the destructor cannot be templated, we need to
         * do SFINAE in a dedicated function, where one syncs and the other is a noop.
         */
        template<access_mode A = AccessMode>
        typename std::enable_if<A == access_mode::write, result_status>::type
        conditional_sync();

        template<access_mode A = AccessMode>
        typename std::enable_if<A == access_mode::read, result_status>::type conditional_sync();
    };

    template<access_mode AccessMode, typename ByteT>
    bool operator==(const basic_mmap<AccessMode, ByteT> &a,
                    const basic_mmap<AccessMode, ByteT> &b);

    template<access_mode AccessMode, typename ByteT>
    bool operator!=(const basic_mmap<AccessMode, ByteT> &a,
                    const basic_mmap<AccessMode, ByteT> &b);

    template<access_mode AccessMode, typename ByteT>
    bool operator<(const basic_mmap<AccessMode, ByteT> &a,
                   const basic_mmap<AccessMode, ByteT> &b);

    template<access_mode AccessMode, typename ByteT>
    bool operator<=(const basic_mmap<AccessMode, ByteT> &a,
                    const basic_mmap<AccessMode, ByteT> &b);

    template<access_mode AccessMode, typename ByteT>
    bool operator>(const basic_mmap<AccessMode, ByteT> &a,
                   const basic_mmap<AccessMode, ByteT> &b);

    template<access_mode AccessMode, typename ByteT>
    bool operator>=(const basic_mmap<AccessMode, ByteT> &a,
                    const basic_mmap<AccessMode, ByteT> &b);

    /**
     * This is the basis for all read-only mmap objects and should be preferred over
     * directly using `basic_mmap`.
     */
    template<typename ByteT>
    using basic_mmap_source = basic_mmap<access_mode::read, ByteT>;

    /**
     * This is the basis for all read-write mmap objects and should be preferred over
     * directly using `basic_mmap`.
     */
    template<typename ByteT>
    using basic_mmap_sink = basic_mmap<access_mode::write, ByteT>;

    /**
     * These aliases cover the most common use cases, both representing a raw byte stream
     * (either with a char or an unsigned char/uint8_t).
     */
    using mmap_source = basic_mmap_source<char>;
    using ummap_source = basic_mmap_source<unsigned char>;

    using mmap_sink = basic_mmap_sink<char>;
    using ummap_sink = basic_mmap_sink<unsigned char>;

    /**
     * Convenience factory method that constructs a mapping for any `basic_mmap` or
     * `basic_mmap` type.
     */
    template<
            typename MMap,
            typename MappingToken
    >
    result_status make_mmap(const MappingToken &token,
                            int64_t offset, int64_t length, MMap &mmap) {
        return mmap.open(token, offset, length);
    }

    /**
     * Convenience factory method.
     *
     * MappingToken may be a String (`std::string`, `std::string_view`, `const char*`,
     * `std::filesystem::path`, `std::vector<char>`, or similar), or a
     * `mmap_source::handle_type`.
     */
    template<typename MappingToken>
    result_status make_mmap_source(const MappingToken &token, mmap_source::size_type offset,
                                   mmap_source::size_type length, mmap_source &mmap) {
        return make_mmap<mmap_source>(token, offset, length, mmap);
    }

    template<typename MappingToken>
    result_status make_mmap_source(const MappingToken &token, mmap_source &mmap) {
        return make_mmap_source(token, 0, map_entire_file, mmap);
    }

    /**
     * Convenience factory method.
     *
     * MappingToken may be a String (`std::string`, `std::string_view`, `const char*`,
     * `std::filesystem::path`, `std::vector<char>`, or similar), or a
     * `mmap_sink::handle_type`.
     */
    template<typename MappingToken>
    result_status make_mmap_sink(const MappingToken &token, mmap_sink::size_type offset,
                                 mmap_sink::size_type length, mmap_sink &mmap) {
        return make_mmap<mmap_sink>(token, offset, length, mmap);
    }

    template<typename MappingToken>
    result_status make_mmap_sink(const MappingToken &token, mmap_sink &mmap) {
        return make_mmap_sink(token, 0, map_entire_file, mmap);
    }

    namespace detail {

        template<
                typename S,
                typename C = typename std::decay<S>::type,
                typename = decltype(std::declval<C>().data()),
                typename = typename std::enable_if<
                        std::is_same<typename C::value_type, char>::value>::type
        >
        struct char_type_helper {
            using type = typename C::value_type;
        };

        template<class T>
        struct char_type {
            using type = typename char_type_helper<T>::type;
        };

        // TODO: can we avoid this brute force approach?
        template<>
        struct char_type<char *> {
            using type = char;
        };

        template<>
        struct char_type<const char *> {
            using type = char;
        };

        template<size_t N>
        struct char_type<char[N]> {
            using type = char;
        };

        template<size_t N>
        struct char_type<const char[N]> {
            using type = char;
        };
        template<typename CharT, typename S>
        struct is_c_str_helper {
            static constexpr bool value = std::is_same<
                    CharT *,
                    // TODO: I'm so sorry for this... Can this be made cleaner?
                    typename std::add_pointer<
                            typename std::remove_cv<
                                    typename std::remove_pointer<
                                            typename std::decay<
                                                    S
                                            >::type
                                    >::type
                            >::type
                    >::type
            >::value;
        };

        template<typename S>
        struct is_c_str {
            static constexpr bool value = is_c_str_helper<char, S>::value;
        };


        template<typename S>
        struct is_c_str_or_c_wstr {
            static constexpr bool value = is_c_str<S>::value;
        };

        template<
                typename String,
                typename = decltype(std::declval<String>().data()),
                typename = typename std::enable_if<!is_c_str_or_c_wstr<String>::value>::type
        >
        const typename char_type<String>::type *c_str(const String &path) {
            return path.data();
        }

        template<
                typename String,
                typename = decltype(std::declval<String>().empty()),
                typename = typename std::enable_if<!is_c_str_or_c_wstr<String>::value>::type
        >
        bool empty(const String &path) {
            return path.empty();
        }

        template<
                typename String,
                typename = typename std::enable_if<is_c_str_or_c_wstr<String>::value>::type
        >
        const typename char_type<String>::type *c_str(String path) {
            return path;
        }

        template<
                typename String,
                typename = typename std::enable_if<is_c_str_or_c_wstr<String>::value>::type
        >
        bool empty(String path) {
            return !path || (*path == 0);
        }

        template<typename String>
        [[nodiscard]] result_status open_file(const String &path, const access_mode mode, file_handle_type &handler) {
            if (detail::empty(path)) {
                return result_status::from_error_code(std::make_error_code(std::errc::invalid_argument));
            }
            handler = ::open(c_str(path),
                             mode == access_mode::read ? O_RDONLY : O_RDWR);
            if (handler == invalid_handle) {
                return result_status::from_last_error();
            }
            return result_status::success();
        }

        [[nodiscard]] inline result_status query_file_size(file_handle_type handle, size_t &size) {
            struct stat sbuf;
            if (::fstat(handle, &sbuf) == -1) {
                return result_status::from_last_error();
            }
            size = sbuf.st_size;
            return result_status::success();
        }

        struct mmap_context {
            char *data = nullptr;
            int64_t length = 0;
            int64_t mapped_length = 0;
        };

        [[nodiscard]] inline result_status memory_map(const file_handle_type file_handle, const int64_t offset,
                                                      const int64_t length, const access_mode mode, mmap_context &ctx) {
            const int64_t aligned_offset = make_offset_page_aligned(offset);
            const int64_t length_to_map = offset - aligned_offset + length;
            char *mapping_start = static_cast<char *>(::mmap(
                    0, // Don't give hint as to where to map.
                    length_to_map,
                    mode == access_mode::read ? PROT_READ : PROT_WRITE,
                    MAP_SHARED,
                    file_handle,
                    aligned_offset));
            if (mapping_start == MAP_FAILED) {
                return result_status::from_last_error();
            }
            ctx.data = mapping_start + offset - aligned_offset;
            ctx.length = length;
            ctx.mapped_length = length_to_map;
            return result_status::success();
        }

    } // namespace detail

    // -- basic_mmap --

    template<access_mode AccessMode, typename ByteT>
    basic_mmap<AccessMode, ByteT>::~basic_mmap() {
        conditional_sync();
        unmap();
    }

    template<access_mode AccessMode, typename ByteT>
    basic_mmap<AccessMode, ByteT>::basic_mmap(basic_mmap &&other) noexcept
            : data_(std::move(other.data_)), length_(std::move(other.length_)),
              mapped_length_(std::move(other.mapped_length_)), file_handle_(std::move(other.file_handle_)),
              is_handle_internal_(std::move(other.is_handle_internal_)) {
        other.data_ = nullptr;
        other.length_ = other.mapped_length_ = 0;
        other.file_handle_ = invalid_handle;
    }

    template<access_mode AccessMode, typename ByteT>
    basic_mmap<AccessMode, ByteT> &
    basic_mmap<AccessMode, ByteT>::operator=(basic_mmap &&other) {
        if (this != &other) {
            // First the existing mapping needs to be removed.
            unmap();
            data_ = std::move(other.data_);
            length_ = std::move(other.length_);
            mapped_length_ = std::move(other.mapped_length_);
            file_handle_ = std::move(other.file_handle_);
            is_handle_internal_ = std::move(other.is_handle_internal_);

            // The moved from basic_mmap's fields need to be reset, because
            // otherwise other's destructor will unmap the same mapping that was
            // just moved into this.
            other.data_ = nullptr;
            other.length_ = other.mapped_length_ = 0;
            other.file_handle_ = invalid_handle;
            other.is_handle_internal_ = false;
        }
        return *this;
    }

    template<access_mode AccessMode, typename ByteT>
    typename basic_mmap<AccessMode, ByteT>::handle_type
    basic_mmap<AccessMode, ByteT>::mapping_handle() const noexcept {
        return file_handle_;
    }

    template<access_mode AccessMode, typename ByteT>
    template<typename String>
    result_status basic_mmap<AccessMode, ByteT>::open(const String &path, const size_type offset,
                                                      const size_type length) {
        if (detail::empty(path)) {
            return result_status::from_error_code(std::make_error_code(std::errc::invalid_argument));
        }
        file_handle_type handler;
        const auto frs = detail::open_file(path, AccessMode, handler);
        if (!frs.is_ok()) {
            return frs;
        }
        frs = open(handler, offset, length);
        // This MUST be after the call to map, as that sets this to true.
        if (frs.is_ok()) {
            is_handle_internal_ = true;
        }
        return frs;
    }

    template<access_mode AccessMode, typename ByteT>
    [[nodiscard]] result_status basic_mmap<AccessMode, ByteT>::open(const handle_type handle,
                                                                    const size_type offset, const size_type length) {
        if (handle == invalid_handle) {
            return result_status::from_error_code(std::make_error_code(std::errc::bad_file_descriptor));
        }

        size_type file_size;
        auto rs = detail::query_file_size(handle, file_size);
        if (!rs.is_ok()) {
            return rs;
        }

        if (offset + length > file_size) {
            return result_status::from_error_code(std::make_error_code(std::errc::invalid_argument));
        }

        detail::mmap_context ctx;
        rs = detail::memory_map(handle, offset,
                                length == map_entire_file ? (file_size - offset) : length,
                                AccessMode, ctx);
        if (!rs.is_ok()) {
            // We must unmap the previous mapping that may have existed prior to this call.
            // Note that this must only be invoked after a new mapping has been created in
            // order to provide the strong guarantee that, should the new mapping fail, the
            // `map` function leaves this instance in a state as though the function had
            // never been invoked.
            unmap();
            file_handle_ = handle;
            is_handle_internal_ = false;
            data_ = reinterpret_cast<pointer>(ctx.data);
            length_ = ctx.length;
            mapped_length_ = ctx.mapped_length;
            return rs;
        }
        return result_status::success();
    }

    template<access_mode AccessMode, typename ByteT>
    template<access_mode A>
    typename std::enable_if<A == access_mode::write, result_status>::type
    basic_mmap<AccessMode, ByteT>::sync() {
        if (!is_open()) {
            return result_status::from_error_code(std::make_error_code(std::errc::bad_file_descriptor));
        }

        if (data()) {
            if (::msync(get_mapping_start(), mapped_length_, MS_SYNC) != 0) {
                return result_status::from_last_error();
            }
        }
        result_status::success();
    }

    template<access_mode AccessMode, typename ByteT>
    void basic_mmap<AccessMode, ByteT>::unmap() {
        if (!is_open()) { return; }
        // TODO do we care about errors here?
        if (data_) { ::munmap(const_cast<pointer>(get_mapping_start()), mapped_length_); }

        // If `file_handle_` was obtained by our opening it (when map is called with
        // a path, rather than an existing file handle), we need to close it,
        // otherwise it must not be closed as it may still be used outside this
        // instance.
        if (is_handle_internal_) {
            ::close(file_handle_);
        }

        // Reset fields to their default values.
        data_ = nullptr;
        length_ = mapped_length_ = 0;
        file_handle_ = invalid_handle;
    }

    template<access_mode AccessMode, typename ByteT>
    bool basic_mmap<AccessMode, ByteT>::is_mapped() const noexcept {
        return is_open();
    }

    template<access_mode AccessMode, typename ByteT>
    void basic_mmap<AccessMode, ByteT>::swap(basic_mmap &other) {
        if (this != &other) {
            using std::swap;
            swap(data_, other.data_);
            swap(file_handle_, other.file_handle_);
            swap(length_, other.length_);
            swap(mapped_length_, other.mapped_length_);
            swap(is_handle_internal_, other.is_handle_internal_);
        }
    }

    template<access_mode AccessMode, typename ByteT>
    template<access_mode A>
    typename std::enable_if<A == access_mode::write, result_status>::type
    basic_mmap<AccessMode, ByteT>::conditional_sync() {
        // This is invoked from the destructor, so not much we can do about
        // failures here.
        return sync();
    }

    template<access_mode AccessMode, typename ByteT>
    template<access_mode A>
    typename std::enable_if<A == access_mode::read, result_status>::type
    basic_mmap<AccessMode, ByteT>::conditional_sync() {
        return result_status::success();
        // noop
    }

    template<access_mode AccessMode, typename ByteT>
    bool operator==(const basic_mmap<AccessMode, ByteT> &a,
                    const basic_mmap<AccessMode, ByteT> &b) {
        return a.data() == b.data()
               && a.size() == b.size();
    }

    template<access_mode AccessMode, typename ByteT>
    bool operator!=(const basic_mmap<AccessMode, ByteT> &a,
                    const basic_mmap<AccessMode, ByteT> &b) {
        return !(a == b);
    }

    template<access_mode AccessMode, typename ByteT>
    bool operator<(const basic_mmap<AccessMode, ByteT> &a,
                   const basic_mmap<AccessMode, ByteT> &b) {
        if (a.data() == b.data()) { return a.size() < b.size(); }
        return a.data() < b.data();
    }

    template<access_mode AccessMode, typename ByteT>
    bool operator<=(const basic_mmap<AccessMode, ByteT> &a,
                    const basic_mmap<AccessMode, ByteT> &b) {
        return !(a > b);
    }

    template<access_mode AccessMode, typename ByteT>
    bool operator>(const basic_mmap<AccessMode, ByteT> &a,
                   const basic_mmap<AccessMode, ByteT> &b) {
        if (a.data() == b.data()) { return a.size() > b.size(); }
        return a.data() > b.data();
    }

    template<access_mode AccessMode, typename ByteT>
    bool operator>=(const basic_mmap<AccessMode, ByteT> &a,
                    const basic_mmap<AccessMode, ByteT> &b) {
        return !(a < b);
    }

    /**
     * Exposes (nearly) the same interface as `basic_mmap`, but endowes it with
     * `std::shared_ptr` semantics.
     *
     * This is not the default behaviour of `basic_mmap` to avoid allocating on the heap if
     * shared semantics are not required.
     */
    template<
            access_mode AccessMode,
            typename ByteT
    >
    class basic_shared_mmap {
        using impl_type = basic_mmap<AccessMode, ByteT>;
        std::shared_ptr<impl_type> pimpl_;

    public:
        using value_type = typename impl_type::value_type;
        using size_type = typename impl_type::size_type;
        using reference = typename impl_type::reference;
        using const_reference = typename impl_type::const_reference;
        using pointer = typename impl_type::pointer;
        using const_pointer = typename impl_type::const_pointer;
        using difference_type = typename impl_type::difference_type;
        using iterator = typename impl_type::iterator;
        using const_iterator = typename impl_type::const_iterator;
        using reverse_iterator = typename impl_type::reverse_iterator;
        using const_reverse_iterator = typename impl_type::const_reverse_iterator;
        using iterator_category = typename impl_type::iterator_category;
        using handle_type = typename impl_type::handle_type;
        using mmap_type = impl_type;

        basic_shared_mmap() = default;

        basic_shared_mmap(const basic_shared_mmap &) = default;

        basic_shared_mmap &operator=(const basic_shared_mmap &) = default;

        basic_shared_mmap(basic_shared_mmap &&) = default;

        basic_shared_mmap &operator=(basic_shared_mmap &&) = default;

        /** Takes ownership of an existing mmap object. */
        basic_shared_mmap(mmap_type &&mmap)
                : pimpl_(std::make_shared<mmap_type>(std::move(mmap))) {}

        /** Takes ownership of an existing mmap object. */
        basic_shared_mmap &operator=(mmap_type &&mmap) {
            pimpl_ = std::make_shared<mmap_type>(std::move(mmap));
            return *this;
        }

        /** Initializes this object with an already established shared mmap. */
        basic_shared_mmap(std::shared_ptr<mmap_type> mmap) : pimpl_(std::move(mmap)) {}

        /** Initializes this object with an already established shared mmap. */
        basic_shared_mmap &operator=(std::shared_ptr<mmap_type> mmap) {
            pimpl_ = std::move(mmap);
            return *this;
        }

        /**
         * If this is a read-write mapping and the last reference to the mapping,
         * the destructor invokes sync. Regardless of the access mode, unmap is
         * invoked as a final step.
         */
        ~basic_shared_mmap() = default;

        /** Returns the underlying `std::shared_ptr` instance that holds the mmap. */
        std::shared_ptr<mmap_type> get_shared_ptr() { return pimpl_; }

        /**
         * On UNIX systems 'file_handle' and 'mapping_handle' are the same. On Windows,
         * however, a mapped region of a file gets its own handle, which is returned by
         * 'mapping_handle'.
         */
        handle_type file_handle() const noexcept {
            return pimpl_ ? pimpl_->file_handle() : invalid_handle;
        }

        handle_type mapping_handle() const noexcept {
            return pimpl_ ? pimpl_->mapping_handle() : invalid_handle;
        }

        /** Returns whether a valid memory mapping has been created. */
        [[nodiscard]] bool is_open() const noexcept { return pimpl_ && pimpl_->is_open(); }

        /**
         * Returns true if no mapping was established, that is, conceptually the
         * same as though the length that was mapped was 0. This function is
         * provided so that this class has Container semantics.
         */
        [[nodiscard]] bool empty() const noexcept { return !pimpl_ || pimpl_->empty(); }

        /**
         * `size` and `length` both return the logical length, i.e. the number of bytes
         * user requested to be mapped, while `mapped_length` returns the actual number of
         * bytes that were mapped which is a multiple of the underlying operating system's
         * page allocation granularity.
         */
        size_type size() const noexcept { return pimpl_ ? pimpl_->length() : 0; }

        size_type length() const noexcept { return pimpl_ ? pimpl_->length() : 0; }

        size_type mapped_length() const noexcept {
            return pimpl_ ? pimpl_->mapped_length() : 0;
        }

        /**
         * Returns a pointer to the first requested byte, or `nullptr` if no memory mapping
         * exists.
         */
        template<
                access_mode A = AccessMode,
                typename = typename std::enable_if<A == access_mode::write>::type
        >
        pointer data() noexcept { return pimpl_->data(); }

        const_pointer data() const noexcept { return pimpl_ ? pimpl_->data() : nullptr; }

        /**
         * Returns an iterator to the first requested byte, if a valid memory mapping
         * exists, otherwise this function call is undefined behaviour.
         */
        iterator begin() noexcept { return pimpl_->begin(); }

        const_iterator begin() const noexcept { return pimpl_->begin(); }

        const_iterator cbegin() const noexcept { return pimpl_->cbegin(); }

        /**
         * Returns an iterator one past the last requested byte, if a valid memory mapping
         * exists, otherwise this function call is undefined behaviour.
         */
        template<
                access_mode A = AccessMode,
                typename = typename std::enable_if<A == access_mode::write>::type
        >
        iterator end() noexcept { return pimpl_->end(); }

        const_iterator end() const noexcept { return pimpl_->end(); }

        const_iterator cend() const noexcept { return pimpl_->cend(); }

        /**
         * Returns a reverse iterator to the last memory mapped byte, if a valid
         * memory mapping exists, otherwise this function call is undefined
         * behaviour.
         */
        template<
                access_mode A = AccessMode,
                typename = typename std::enable_if<A == access_mode::write>::type
        >
        reverse_iterator rbegin() noexcept { return pimpl_->rbegin(); }

        const_reverse_iterator rbegin() const noexcept { return pimpl_->rbegin(); }

        const_reverse_iterator crbegin() const noexcept { return pimpl_->crbegin(); }

        /**
         * Returns a reverse iterator past the first mapped byte, if a valid memory
         * mapping exists, otherwise this function call is undefined behaviour.
         */
        template<
                access_mode A = AccessMode,
                typename = typename std::enable_if<A == access_mode::write>::type
        >
        reverse_iterator rend() noexcept { return pimpl_->rend(); }

        const_reverse_iterator rend() const noexcept { return pimpl_->rend(); }

        const_reverse_iterator crend() const noexcept { return pimpl_->crend(); }

        /**
         * Returns a reference to the `i`th byte from the first requested byte (as returned
         * by `data`). If this is invoked when no valid memory mapping has been created
         * prior to this call, undefined behaviour ensues.
         */
        reference operator[](const size_type i) noexcept { return (*pimpl_)[i]; }

        const_reference operator[](const size_type i) const noexcept { return (*pimpl_)[i]; }

        /**
         * Establishes a memory mapping with AccessMode. If the mapping is unsuccesful, the
         * reason is reported via `error` and the object remains in a state as if this
         * function hadn't been called.
         *
         * `path`, which must be a path to an existing file, is used to retrieve a file
         * handle (which is closed when the object destructs or `unmap` is called), which is
         * then used to memory map the requested region. Upon failure, `error` is set to
         * indicate the reason and the object remains in an unmapped state.
         *
         * `offset` is the number of bytes, relative to the start of the file, where the
         * mapping should begin. When specifying it, there is no need to worry about
         * providing a value that is aligned with the operating system's page allocation
         * granularity. This is adjusted by the implementation such that the first requested
         * byte (as returned by `data` or `begin`), so long as `offset` is valid, will be at
         * `offset` from the start of the file.
         *
         * `length` is the number of bytes to map. It may be `map_entire_file`, in which
         * case a mapping of the entire file is created.
         */
        template<typename String>
        result_status open(const String &path, const size_type offset,
                           const size_type length) {
            return open_impl(path, offset, length);
        }

        /**
         * Establishes a memory mapping with AccessMode. If the mapping is unsuccesful, the
         * reason is reported via `error` and the object remains in a state as if this
         * function hadn't been called.
         *
         * `path`, which must be a path to an existing file, is used to retrieve a file
         * handle (which is closed when the object destructs or `unmap` is called), which is
         * then used to memory map the requested region. Upon failure, `error` is set to
         * indicate the reason and the object remains in an unmapped state.
         *
         * The entire file is mapped.
         */
        template<typename String>
        result_status open(const String &path) {
            return open_impl(path, 0, map_entire_file);
        }

        /**
         * Establishes a memory mapping with AccessMode. If the mapping is unsuccesful, the
         * reason is reported via `error` and the object remains in a state as if this
         * function hadn't been called.
         *
         * `handle`, which must be a valid file handle, which is used to memory map the
         * requested region. Upon failure, `error` is set to indicate the reason and the
         * object remains in an unmapped state.
         *
         * `offset` is the number of bytes, relative to the start of the file, where the
         * mapping should begin. When specifying it, there is no need to worry about
         * providing a value that is aligned with the operating system's page allocation
         * granularity. This is adjusted by the implementation such that the first requested
         * byte (as returned by `data` or `begin`), so long as `offset` is valid, will be at
         * `offset` from the start of the file.
         *
         * `length` is the number of bytes to map. It may be `map_entire_file`, in which
         * case a mapping of the entire file is created.
         */
        result_status open(const handle_type handle, const size_type offset,
                           const size_type length) {
            return open_impl(handle, offset, length);
        }

        /**
         * Establishes a memory mapping with AccessMode. If the mapping is unsuccesful, the
         * reason is reported via `error` and the object remains in a state as if this
         * function hadn't been called.
         *
         * `handle`, which must be a valid file handle, which is used to memory map the
         * requested region. Upon failure, `error` is set to indicate the reason and the
         * object remains in an unmapped state.
         *
         * The entire file is mapped.
         */
        result_status open(const handle_type handle) {
            return open_impl(handle, 0, map_entire_file);
        }

        /**
         * If a valid memory mapping has been created prior to this call, this call
         * instructs the kernel to unmap the memory region and disassociate this object
         * from the file.
         *
         * The file handle associated with the file that is mapped is only closed if the
         * mapping was created using a file path. If, on the other hand, an existing
         * file handle was used to create the mapping, the file handle is not closed.
         */
        void unmap() { if (pimpl_) pimpl_->unmap(); }

        void swap(basic_shared_mmap &other) { pimpl_.swap(other.pimpl_); }

        /** Flushes the memory mapped page to disk. Errors are reported via `error`. */
        template<
                access_mode A = AccessMode,
                typename = typename std::enable_if<A == access_mode::write>::type
        >
        result_status sync() { if (pimpl_) pimpl_->sync(); }

        /** All operators compare the underlying `basic_mmap`'s addresses. */

        friend bool operator==(const basic_shared_mmap &a, const basic_shared_mmap &b) {
            return a.pimpl_ == b.pimpl_;
        }

        friend bool operator!=(const basic_shared_mmap &a, const basic_shared_mmap &b) {
            return !(a == b);
        }

        friend bool operator<(const basic_shared_mmap &a, const basic_shared_mmap &b) {
            return a.pimpl_ < b.pimpl_;
        }

        friend bool operator<=(const basic_shared_mmap &a, const basic_shared_mmap &b) {
            return a.pimpl_ <= b.pimpl_;
        }

        friend bool operator>(const basic_shared_mmap &a, const basic_shared_mmap &b) {
            return a.pimpl_ > b.pimpl_;
        }

        friend bool operator>=(const basic_shared_mmap &a, const basic_shared_mmap &b) {
            return a.pimpl_ >= b.pimpl_;
        }

    private:

        template<typename MappingToken>
        result_status open_impl(const MappingToken &token, const size_type offset,
                                const size_type length) {
            result_status rs;
            if (!pimpl_) {
                mmap_type mmap;
                make_mmap<mmap_type>(token, offset, length, mmap);
                if (!rs.is_ok()) {
                    return rs;
                }
                pimpl_ = std::make_shared<mmap_type>(std::move(mmap));
            } else {
                rs = pimpl_->open(token, offset, length);
            }
            return rs;
        }
    };

    /**
     * This is the basis for all read-only mmap objects and should be preferred over
     * directly using basic_shared_mmap.
     */
    template<typename ByteT>
    using basic_shared_mmap_source = basic_shared_mmap<access_mode::read, ByteT>;

    /**
     * This is the basis for all read-write mmap objects and should be preferred over
     * directly using basic_shared_mmap.
     */
    template<typename ByteT>
    using basic_shared_mmap_sink = basic_shared_mmap<access_mode::write, ByteT>;

    /**
     * These aliases cover the most common use cases, both representing a raw byte stream
     * (either with a char or an unsigned char/uint8_t).
     */
    using shared_mmap_source = basic_shared_mmap_source<char>;
    using shared_ummap_source = basic_shared_mmap_source<unsigned char>;

    using shared_mmap_sink = basic_shared_mmap_sink<char>;
    using shared_ummap_sink = basic_shared_mmap_sink<unsigned char>;

} // namespace flare

#endif  // FLARE_FILES_MMAP_FILE_H_
