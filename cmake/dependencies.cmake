# dependencies.cmake
#
# The VSCP sources are fetched at configure time with FetchContent, pinned
# to a commit and checked with SHA256. The driver only needs the CANAL
# headers (canal.h) from src/vscp/common.
#
# To build against a local checkout instead:
#   cmake -B build -DFETCHCONTENT_SOURCE_DIR_VSCP=/path/to/vscp
# FETCHCONTENT_FULLY_DISCONNECTED=ON reuses an earlier download offline.
#
# Update the pin: set the commit below, then
#   curl -sL https://codeload.github.com/grodansparadis/vscp/tar.gz/<commit> | sha256sum
# and put the result in URL_HASH.
#
# Sets VSCP_SOURCE_DIR.
#

include_guard(GLOBAL)
include(FetchContent)

set(_vscp_options)
if(NOT CMAKE_VERSION VERSION_LESS "3.24")
  list(APPEND _vscp_options DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
endif()

# grodansparadis/vscp @ d75d4f5c6c097a1dca841b63727c79c522b701ce (master)
FetchContent_Declare(
  vscp
  URL https://codeload.github.com/grodansparadis/vscp/tar.gz/d75d4f5c6c097a1dca841b63727c79c522b701ce
  URL_HASH SHA256=709763c16c995b65787e72947f9c3f45a03a404c708286c7cad00b53dcaedbb5
  ${_vscp_options}
)

# Only the sources are used, VSCP's own CMake project is not added
FetchContent_GetProperties(vscp)
if(NOT vscp_POPULATED)
  if(POLICY CMP0169)
    cmake_policy(PUSH)
    cmake_policy(SET CMP0169 OLD)
    FetchContent_Populate(vscp)
    cmake_policy(POP)
  else()
    FetchContent_Populate(vscp)
  endif()
endif()

set(VSCP_SOURCE_DIR "${vscp_SOURCE_DIR}")
