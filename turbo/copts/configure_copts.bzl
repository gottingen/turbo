"""turbo specific copts.

This file simply selects the correct options from the generated files.  To
change Turbo copts, edit turbo/copts/copts.py
"""

load(
    "//turbo:copts/GENERATED_copts.bzl",
    "TURBO_CLANG_CL_FLAGS",
    "TURBO_CLANG_CL_TEST_FLAGS",
    "TURBO_GCC_FLAGS",
    "TURBO_GCC_TEST_FLAGS",
    "TURBO_LLVM_FLAGS",
    "TURBO_LLVM_TEST_FLAGS",
    "TURBO_MSVC_FLAGS",
    "TURBO_MSVC_LINKOPTS",
    "TURBO_MSVC_TEST_FLAGS",
    "TURBO_RANDOM_HWAES_ARM32_FLAGS",
    "TURBO_RANDOM_HWAES_ARM64_FLAGS",
    "TURBO_RANDOM_HWAES_MSVC_X64_FLAGS",
    "TURBO_RANDOM_HWAES_X64_FLAGS",
)

TURBO_DEFAULT_COPTS = select({
    "//turbo:msvc_compiler": TURBO_MSVC_FLAGS,
    "//turbo:clang-cl_compiler": TURBO_CLANG_CL_FLAGS,
    "//turbo:clang_compiler": TURBO_LLVM_FLAGS,
    "//turbo:gcc_compiler": TURBO_GCC_FLAGS,
    "//conditions:default": TURBO_GCC_FLAGS,
})

TURBO_TEST_COPTS = select({
    "//turbo:msvc_compiler": TURBO_MSVC_TEST_FLAGS,
    "//turbo:clang-cl_compiler": TURBO_CLANG_CL_TEST_FLAGS,
    "//turbo:clang_compiler": TURBO_LLVM_TEST_FLAGS,
    "//turbo:gcc_compiler": TURBO_GCC_TEST_FLAGS,
    "//conditions:default": TURBO_GCC_TEST_FLAGS,
})

TURBO_DEFAULT_LINKOPTS = select({
    "//turbo:msvc_compiler": TURBO_MSVC_LINKOPTS,
    "//conditions:default": [],
})

# TURBO_RANDOM_RANDEN_COPTS blaze copts flags which are required by each
# environment to build an accelerated RandenHwAes library.
TURBO_RANDOM_RANDEN_COPTS = select({
    # APPLE
    ":cpu_darwin_x86_64": TURBO_RANDOM_HWAES_X64_FLAGS,
    ":cpu_darwin": TURBO_RANDOM_HWAES_X64_FLAGS,
    ":cpu_x64_windows_msvc": TURBO_RANDOM_HWAES_MSVC_X64_FLAGS,
    ":cpu_x64_windows": TURBO_RANDOM_HWAES_MSVC_X64_FLAGS,
    ":cpu_k8": TURBO_RANDOM_HWAES_X64_FLAGS,
    ":cpu_ppc": ["-mcrypto"],
    ":cpu_aarch64": TURBO_RANDOM_HWAES_ARM64_FLAGS,

    # Supported by default or unsupported.
    "//conditions:default": [],
})

# turbo_random_randen_copts_init:
#  Initialize the config targets based on cpu, os, etc. used to select
#  the required values for TURBO_RANDOM_RANDEN_COPTS
def turbo_random_randen_copts_init():
    """Initialize the config_settings used by TURBO_RANDOM_RANDEN_COPTS."""

    # CPU configs.
    # These configs have consistent flags to enable HWAES intsructions.
    cpu_configs = [
        "ppc",
        "k8",
        "darwin_x86_64",
        "darwin",
        "x64_windows_msvc",
        "x64_windows",
        "aarch64",
    ]
    for cpu in cpu_configs:
        native.config_setting(
            name = "cpu_%s" % cpu,
            values = {"cpu": cpu},
        )
