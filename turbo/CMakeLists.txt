#
# Copyright (C) 2024 EA group inc.
# Author: Jeff.li lijippy@163.com
# All rights reserved.
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
###########################################################################
# below are examples, after initialization, modify it to your own.
###########################################################################
###carbin_example
file(GLOB_RECURSE TURBO_SRC "*.cc" "*.c")

carbin_cc_library(
        NAMESPACE turbo
        NAME turbo
        SOURCES
        ${TURBO_SRC}
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        COPTS
        "-fvisibility=hidden"
        DEFINES
        -DURI_STATIC_BUILD
        PLINKS
        ${CARBIN_DEPS_LINK}
        PUBLIC
)
###carbin_example