

################################################################################
# Create an executable.
#
# Example usage:
#
# carbin_cc_binary(  NAME myExe
#                  SOURCES
#                       main.cc
#                  PUBLIC_DEFINITIONS
#                     USE_DOUBLE_PRECISION=1
#                  PRIVATE_DEFINITIONS
#                     DEBUG_VERBOSE
#                  PUBLIC_INCLUDE_PATHS
#                     ${CMAKE_SOURCE_DIR}/mylib/include
#                  PRIVATE_INCLUDE_PATHS
#                     ${CMAKE_SOURCE_DIR}/include
#                  PRIVATE_LINKED_TARGETS
#                     Threads::Threads
#                  PUBLIC_LINKED_TARGETS
#                     Threads::Threads
#                  LINKED_TARGETS
#                     Threads::Threads
#                     myNamespace::myLib
# )
#
# The above example creates an alias target, myNamespace::myLibrary which can be
# linked to by other tar gets.
# PUBLIC_DEFINITIONS -  preprocessor defines which are inherated by targets which
#                       link to this library
#
# PRIVATE_DEFINITIONS - preprocessor defines which are private and only seen by
#                       myLibrary
#
# PUBLIC_INCLUDE_PATHS - include paths which are public, therefore inherted by
#                        targest which link to this library.
#
# PRIVATE_INCLUDE_PATHS - private include paths which are only visible by MyExe
#
# LINKED_TARGETS        - targets to link to.
################################################################################

function(carbin_cc_binary)
    set(options
            VERBOSE)
    set(args NAME
            )

    set(list_args
            PUBLIC_LINKED_TARGETS
            PRIVATE_LINKED_TARGETS
            SOURCES
            PUBLIC_DEFINITIONS
            PRIVATE_DEFINITIONS
            PUBLIC_INCLUDE_PATHS
            PRIVATE_INCLUDE_PATHS
            PUBLIC_COMPILE_FEATURES
            PRIVATE_COMPILE_FEATURES
            PUBLIC_COMPILE_OPTIONS
            PRIVATE_COMPILE_OPTIONS
            )

    cmake_parse_arguments(
            PARSE_ARGV 0
            CARBIN_CC_BINARY
            "${options}"
            "${args}"
            "${list_args}"
    )
    carbin_raw("-----------------------------------")
    carbin_print_label("Building Binary" "${CARBIN_CC_BINARY_NAME}")
    carbin_raw("-----------------------------------")
    if(CARBIN_CC_BINARY_VERBOSE)
        carbin_print_list_label("Sources" CARBIN_CC_BINARY_SOURCES)
        carbin_print_list_label("Public Linked Targest"  CARBIN_CC_BINARY_PUBLIC_LINKED_TARGETS)
        carbin_print_list_label("Private Linked Targest"  CARBIN_CC_BINARY_PRIVATE_LINKED_TARGETS)
        carbin_print_list_label("Public Include Paths"  CARBIN_CC_BINARY_PUBLIC_INCLUDE_PATHS)
        carbin_print_list_label("Private Include Paths" CARBIN_CC_BINARY_PRIVATE_INCLUDE_PATHS)
        carbin_print_list_label("Public Compile Features" CARBIN_CC_BINARY_PUBLIC_COMPILE_FEATURES)
        carbin_print_list_label("Private Compile Features" CARBIN_CC_BINARY_PRIVATE_COMPILE_FEATURES)
        carbin_print_list_label("Public Definitions" CARBIN_CC_BINARY_PUBLIC_DEFINITIONS)
        carbin_print_list_label("Private Definitions" CARBIN_CC_BINARY_PRIVATE_DEFINITIONS)
        carbin_raw("-----------------------------------")
    endif()
    add_executable( ${CARBIN_CC_BINARY_NAME} ${CARBIN_CC_BINARY_SOURCES} )
    target_link_libraries(${CARBIN_CC_BINARY_NAME}
            PUBLIC ${CARBIN_CC_BINARY_PUBLIC_LINKED_TARGETS}
            PRIVATE ${CARBIN_CC_BINARY_PRIVATE_LINKED_TARGETS})



    target_include_directories( ${CARBIN_CC_BINARY_NAME}
            PUBLIC
            ${CARBIN_CC_BINARY_PUBLIC_INCLUDE_PATHS}
            PRIVATE
            ${CARBIN_CC_BINARY_PRIVATE_INCLUDE_PATHS}
            )

    target_compile_definitions( ${CARBIN_CC_BINARY_NAME}
            PUBLIC
            ${CARBIN_CC_BINARY_PUBLIC_DEFINITIONS}
            PRIVATE
            ${CARBIN_CC_BINARY_PRIVATE_DEFINITIONS}
            )
    target_compile_features(${CARBIN_CC_BINARY_NAME} PUBLIC ${CARBIN_CC_BINARY_PUBLIC_COMPILE_FEATURES} )
    target_compile_features(${CARBIN_CC_BINARY_NAME} PRIVATE ${CARBIN_CC_BINARY_PRIVATE_COMPILE_FEATURES} )

    target_compile_options(${CARBIN_CC_BINARY_NAME} PUBLIC ${CARBIN_CC_BINARY_PUBLIC_COMPILE_OPTIONS} )
    target_compile_options(${CARBIN_CC_BINARY_NAME} PRIVATE ${CARBIN_CC_BINARY_PRIVATE_COMPILE_OPTIONS} )


    set_property(TARGET ${CARBIN_CC_BINARY_NAME} PROPERTY CXX_STANDARD ${CARBIN_CXX_STANDARD})
    set_property(TARGET ${CARBIN_CC_BINARY_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

    install(TARGETS ${CARBIN_CC_BINARY_NAME} EXPORT ${PROJECT_NAME}Targets
            RUNTIME DESTINATION ${CARBIN_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CARBIN_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CARBIN_INSTALL_LIBDIR}
            )

    ################################################################################

    foreach(arg IN LISTS CARBIN_CC_BINARY_UNPARSED_ARGUMENTS)
        carbin_warn( "Unparsed argument: ${arg}")
    endforeach()

endfunction(carbin_cc_binary)