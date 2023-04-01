

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