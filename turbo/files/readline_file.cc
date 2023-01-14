
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "turbo/files/readline_file.h"
#include "turbo/files/sequential_read_file.h"
#include "turbo/base/errno.h"
#include "turbo/strings/str_split.h"
#include "turbo/strings/utility.h"

namespace turbo {

    result_status readline_file::open(const turbo::file_path &path,  readline_option option) {
        TURBO_CHECK(_path.empty()) << "do not reopen";
        sequential_read_file file;
        _path = path;
        _status = file.open(_path);
        if (!_status.is_ok()) {
            TURBO_LOG(ERROR) << "open file :"<<_path<<" eroor "<< turbo_error();
            return _status;
        }

        _status = file.read(&_content);
        if (!_status.is_ok()) {
            TURBO_LOG(ERROR) << "read file :"<<_path<<" eroor "<< turbo_error();
            return _status;
        }
        if(option == readline_option::eSkipEmptyLine) {
            _lines = turbo::string_split(_content, turbo::by_any_char("\n"), turbo::skip_empty());
        } else if(option == readline_option::eTrimWhitespace) {
            _lines = turbo::string_split(_content, turbo::by_any_char("\n"), turbo::skip_whitespace());
        } else {
            _lines = turbo::string_split(_content, turbo::by_any_char("\n"));
            _lines.pop_back();
        }
        for(size_t i = 0; i < _lines.size(); i++) {
            if(_lines[i].empty() && turbo::back_char(_lines[i]) == '\r') {
                _lines[i].remove_suffix(1);
            }

        }
        return _status;
    }

}  // namespace turbo