
include(carbin_print)
include(carbin_platform)

set(CARBIN_GENERATOR "STGZ;RPM")

carbin_print("on platform ${CMAKE_HOST_SYSTEM_NAME} package type tgz")

string(TOLOWER ${CMAKE_HOST_SYSTEM_NAME} HOST_SYSTEM_NAME)

if(SYSTEM_NAME MATCHES "centos")
    set(CARBIN_GENERATOR "TGZ;RPM")
    include(carbin_package_rpm)
    string(REGEX MATCH "([0-9])" ELV "${LINUX_VER}")
    set(HOST_SYSTEM_NAME el${CMAKE_MATCH_1})
elseif(SYSTEM_NAME MATCHES "rhel")
    set(CARBIN_GENERATOR "TGZ;RPM")
    include(carbin_package_rpm)
    string(REGEX MATCH "([0-9])" ELV "${LINUX_VER}")
    set(HOST_SYSTEM_NAME el${CMAKE_MATCH_1})
endif()

set(CPACK_RPM_PACKAGE_DEBUG 1)
set(CPACK_RPM_RUNTIME_DEBUGINFO_PACKAGE ON)
set(CPACK_RPM_PACKAGE_RELOCATABLE ON)
SET(CPACK_RPM_PRE_INSTALL_SCRIPT_FILE "${PROJECT_SOURCE_DIR}/cmake/package/preinst")
SET(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${PROJECT_SOURCE_DIR}/cmake/package/postinst")
SET(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${PROJECT_SOURCE_DIR}/cmake/package/prerm")
SET(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${PROJECT_SOURCE_DIR}/cmake/package/postrm")

