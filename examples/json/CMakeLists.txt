#
# Copyright 2023 The Turbo Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

turbo_cc_binary(
        NAME
        json_archiver
        SRCS
        "archiver_test.cc"
        "archiver.cc"
        DEPS
        turbo::turbo
)

turbo_cc_binary(
        NAME
        json_capitalize
        SRCS
        "capitalize.cc"
        DEPS
        turbo::turbo
)

turbo_cc_binary(
        NAME
        json_condense
        SRCS
        "condense.cc"
        DEPS
        turbo::turbo
)

turbo_cc_binary(
        NAME
        json_filterkey
        SRCS
        "filterkey.cc"
        DEPS
        turbo::turbo
)

turbo_cc_binary(
        NAME
        json_filterkeydom
        SRCS
        "filterkeydom.cc"
        DEPS
        turbo::turbo
)

turbo_cc_binary(
        NAME
        json_jsonx
        SRCS
        "jsonx.cc"
        DEPS
        turbo::turbo
)

turbo_cc_binary(
        NAME
        json_lookaheadparser
        SRCS
        "lookaheadparser.cc"
        DEPS
        turbo::turbo
)

turbo_cc_binary(
        NAME
        json_messagereader
        SRCS
        "messagereader.cc"
        DEPS
        turbo::turbo
)

turbo_cc_binary(
        NAME
        json_parsebyparts
        SRCS
        "parsebyparts.cc"
        DEPS
        turbo::turbo
)


turbo_cc_binary(
        NAME
        json_pretty
        SRCS
        "pretty.cc"
        DEPS
        turbo::turbo
)

turbo_cc_binary(
        NAME
        json_prettyauto
        SRCS
        "prettyauto.cc"
        DEPS
        turbo::turbo
)

turbo_cc_binary(
        NAME
        json_schemavalidator
        SRCS
        "schemavalidator.cc"
        DEPS
        turbo::turbo
)


turbo_cc_binary(
        NAME
        json_serialize
        SRCS
        "serialize.cc"
        DEPS
        turbo::turbo
)

turbo_cc_binary(
        NAME
        json_simpledom
        SRCS
        "simpledom.cc"
        DEPS
        turbo::turbo
)

turbo_cc_binary(
        NAME
        json_simplepullreader
        SRCS
        "simplepullreader.cc"
        DEPS
        turbo::turbo
)

turbo_cc_binary(
        NAME
        json_simplereader
        SRCS
        "simplereader.cc"
        DEPS
        turbo::turbo
)

turbo_cc_binary(
        NAME
        json_simplewriter
        SRCS
        "simplewriter.cc"
        DEPS
        turbo::turbo
)

turbo_cc_binary(
        NAME
        json_sortkeys
        SRCS
        "sortkeys.cc"
        DEPS
        turbo::turbo
)

turbo_cc_binary(
        NAME
        json_traverseaspointer
        SRCS
        "traverseaspointer.cc"
        DEPS
        turbo::turbo
)

turbo_cc_binary(
        NAME
        json_tutorial
        SRCS
        "tutorial.cc"
        DEPS
        turbo::turbo
)