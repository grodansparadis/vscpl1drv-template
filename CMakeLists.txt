# CMakeLists.txt
#
# VSCP Level I (CANAL) driver template
#
# Builds the driver as a shared library for Linux (.so), macOS (.dylib) and
# Windows (.dll), the test program and the man page.
#
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
#   ctest --test-dir build --output-on-failure
#   cmake --install build
#
# Copyright (C) 2000-2026 Ake Hedman, Grodans Paradis AB
# SPDX-License-Identifier: MIT
#

cmake_minimum_required(VERSION 3.16)

# Version year.month.patch from src/version.h (updated on every push to
# master by the "Update version" workflow)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h" _driver_version_lines
     REGEX "^#define[ \t]+DRIVER_VERSION_(YEAR|MONTH|PATCH)[ \t]+[0-9]+")
foreach(_part YEAR MONTH PATCH)
  string(REGEX MATCH "DRIVER_VERSION_${_part}[ \t]+([0-9]+)" _match "${_driver_version_lines}")
  set(DRIVER_VERSION_${_part} "${CMAKE_MATCH_1}")
endforeach()

project(vscpl1drv-template
  VERSION ${DRIVER_VERSION_YEAR}.${DRIVER_VERSION_MONTH}.${DRIVER_VERSION_PATCH}
  DESCRIPTION "VSCP Level I (CANAL) driver template"
  HOMEPAGE_URL "https://github.com/grodansparadis/vscpl1drv-template"
  LANGUAGES CXX)

# The name of the driver library (and its .def file on Windows)
set(DRIVER_NAME ${PROJECT_NAME})

message(STATUS "${DRIVER_NAME} version ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
include(GNUInstallDirs)
include(${PROJECT_SOURCE_DIR}/cmake/dependencies.cmake)

find_package(Threads REQUIRED)

# --- The driver -------------------------------------------------------------

add_library(${DRIVER_NAME} SHARED
  src/canal-api.cpp
  src/template-driver.cpp
)

target_include_directories(${DRIVER_NAME} PRIVATE
  ${PROJECT_SOURCE_DIR}/src
  ${VSCP_SOURCE_DIR}/src/vscp/common
)

target_link_libraries(${DRIVER_NAME} PRIVATE Threads::Threads)

# Hosts load the driver by its file name: vscpl1drv-template.so/.dylib/.dll,
# without the "lib" prefix and without a version in the name.
set_target_properties(${DRIVER_NAME} PROPERTIES
  PREFIX ""
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)

if(WIN32)
  # Undecorated export names, see src/vscpl1drv-template.def
  target_sources(${DRIVER_NAME} PRIVATE src/${DRIVER_NAME}.def)
  target_compile_definitions(${DRIVER_NAME} PRIVATE WIN32 _CRT_SECURE_NO_WARNINGS NOMINMAX)
  if(MSVC)
    target_compile_options(${DRIVER_NAME} PRIVATE /W4 /utf-8)
  endif()
else()
  target_compile_options(${DRIVER_NAME} PRIVATE -Wall -Wextra)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Fail at link time, not at load time, if a symbol is missing
    target_link_options(${DRIVER_NAME} PRIVATE -Wl,--no-undefined)
  endif()
endif()

# --- Install ----------------------------------------------------------------
#
# Linux: /var/lib/vscp/drivers/level1, where the VSCP daemon looks for
# Level I drivers. macOS and Windows: drivers/level1 below the prefix.

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(DRIVER_INSTALL_DIR "/var/lib/vscp/drivers/level1" CACHE PATH "Where the driver is installed")
else()
  set(DRIVER_INSTALL_DIR "drivers/level1" CACHE PATH "Where the driver is installed")
endif()

install(TARGETS ${DRIVER_NAME}
  LIBRARY DESTINATION ${DRIVER_INSTALL_DIR}
  RUNTIME DESTINATION ${DRIVER_INSTALL_DIR})

install(FILES README.md LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR})

# Man page (man/vscpl1drv-template.1.md, converted with pandoc)
include(ManPages)
vscp_add_man_pages(${PROJECT_SOURCE_DIR}/man)

# --- Tests ------------------------------------------------------------------

include(CTest)
if(BUILD_TESTING)
  add_executable(test-driver tests/test-driver.cpp)
  target_include_directories(test-driver PRIVATE ${VSCP_SOURCE_DIR}/src/vscp/common)
  target_link_libraries(test-driver PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
  # The test loads the driver at run time, as a host does
  add_dependencies(test-driver ${DRIVER_NAME})
  add_test(NAME driver COMMAND test-driver $<TARGET_FILE:${DRIVER_NAME}>)

  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_FOUND)
    add_test(NAME update_version
             COMMAND ${Python3_EXECUTABLE} -B ${PROJECT_SOURCE_DIR}/tests/test_update_version.py)
    # The template's own tool; drivers made from the template do not have it
    if(EXISTS ${PROJECT_SOURCE_DIR}/tests/test_new_driver.py)
      add_test(NAME new_driver
               COMMAND ${Python3_EXECUTABLE} -B ${PROJECT_SOURCE_DIR}/tests/test_new_driver.py)
    endif()
  endif()
endif()

# --- Packages (cpack) -------------------------------------------------------

set(CPACK_PACKAGE_NAME ${DRIVER_NAME})
set(CPACK_PACKAGE_VENDOR "Grodans Paradis AB")
set(CPACK_PACKAGE_CONTACT "Ake Hedman <akhe@grodansparadis.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE)
set(CPACK_PACKAGE_CHECKSUM SHA256)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CPACK_GENERATOR "DEB;TGZ")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
  set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
elseif(WIN32)
  set(CPACK_GENERATOR "ZIP")
else()
  set(CPACK_GENERATOR "TGZ")
endif()
include(CPack)
