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

#ifndef TURBO_NETWORK_POLLER_PIPE_WRAP_H_
#define TURBO_NETWORK_POLLER_PIPE_WRAP_H_


namespace turbo {

class PipeWrap {
public:
    PipeWrap();
    ~PipeWrap();
    int write(const void *buf, int n);
    int read(void *buf, int n);
    int readFD() const { return _pipe_fd[0]; }
    int writeFD() const { return _pipe_fd[1]; }
    void reOpen();

private:
    void clearFD();

private:
    int _pipe_fd[2] = { -1, -1 };
};

} /* namespace turbo */
#endif // TURBO_NETWORK_POLLER_PIPE_WRAP_H_