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

#include "turbo/platform/port.h"
#include "turbo/log/logging.h"
#include "turbo/log/sinks/ansicolor_sink.h"
#include "turbo/log/log_sink.h"
#include "turbo/log/log_sink_registry.h"
#include "gtest/gtest.h"

TEST(sinks, color_sinks) {
  turbo::InitializeLog();
  turbo::AnsicolorSink *as  = new turbo::AnsicolorSink(stderr);
  turbo::AddLogSink(as);
  TURBO_LOG(INFO)<<"hello";
  TURBO_LOG(WARNING)<<"hello";
  TURBO_LOG(ERROR)<<"hello";
  //TURBO_LOG(FATAL)<<"hello";
}