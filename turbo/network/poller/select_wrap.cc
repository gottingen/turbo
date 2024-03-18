// Copyright 2023 The turbo Authors.
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

#include "turbo/network/poller/select_wrap.h"

using namespace std;

namespace turbo {

FdSet::FdSet() {
    _ptr = new fd_set;
}

FdSet::~FdSet() {
    delete (fd_set *)_ptr;
}

void FdSet::fdZero() {
    FD_ZERO((fd_set *)_ptr);
}

void FdSet::fdClr(int fd) {
    FD_CLR(fd, (fd_set *)_ptr);
}

void FdSet::fdSet(int fd) {
    FD_SET(fd, (fd_set *)_ptr);
}

bool FdSet::isSet(int fd) {
    return  FD_ISSET(fd, (fd_set *)_ptr);
}

int turbo_select(int cnt, FdSet *read, FdSet *write, FdSet *err, struct timeval *tv) {
    void *rd, *wt, *er;
    rd = read ? read->_ptr : nullptr;
    wt = write ? write->_ptr : nullptr;
    er = err ? err->_ptr : nullptr;
    return ::select(cnt, (fd_set *) rd, (fd_set *) wt, (fd_set *) er, tv);
}

} /* namespace turbo */