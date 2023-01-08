
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "flare/files/readline_file.h"
#include "flare/files/sequential_read_file.h"
#include "flare/base/errno.h"
#include "flare/strings/str_split.h"
#include "flare/strings/utility.h"

namespace flare {

    result_status readline_file::open(const flare::file_path &path,  readline_option option) {
        FLARE_CHECK(_path.empty()) << "do not reopen";
        sequential_read_file file;
        _path = path;
        _status = file.open(_path);
        if (!_status.is_ok()) {
            FLARE_LOG(ERROR) << "open file :"<<_path<<" eroor "<< flare_error();
            return _status;
        }

        _status = file.read(&_content);
        if (!_status.is_ok()) {
            FLARE_LOG(ERROR) << "read file :"<<_path<<" eroor "<< flare_error();
            return _status;
        }
        if(option == readline_option::eSkipEmptyLine) {
            _lines = flare::string_split(_content, flare::by_any_char("\n"), flare::skip_empty());
        } else if(option == readline_option::eTrimWhitespace) {
            _lines = flare::string_split(_content, flare::by_any_char("\n"), flare::skip_whitespace());
        } else {
            _lines = flare::string_split(_content, flare::by_any_char("\n"));
            _lines.pop_back();
        }
        for(size_t i = 0; i < _lines.size(); i++) {
            if(_lines[i].empty() && flare::back_char(_lines[i]) == '\r') {
                _lines[i].remove_suffix(1);
            }

        }
        return _status;
    }

}  // namespace flare