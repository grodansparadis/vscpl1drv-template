// canal-api.cpp
//
// The CANAL API of the VSCP Level I driver template.
//
// These are the functions a host (the VSCP daemon, VSCP Works, vscp-cli,
// ...) finds by name in the shared library. Each CanalOpen creates a
// CTemplateDriver and returns a handle to it; the other functions look the
// handle up and call the driver object. Usually only the driver name, the
// vendor string and the driver information (driverinfo.h) need to change in
// this file.
//
// The functions follow the CANAL specification
// (https://grodansparadis.github.io/vscp-doc-canal). The *FD functions are
// the optional CAN FD extension; hosts that do not know about them use the
// standard functions and never get FD frames.
//
// This file is part of the VSCP (https://www.vscp.org)
//
// The MIT License (MIT)
//
// Copyright (C) 2000-2026 Ake Hedman, Grodans Paradis AB <info@grodansparadis.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <cstring>
#include <map>
#include <memory>
#include <mutex>

#include <canal.h>

#include "driverinfo.h"
#include "template-driver.h"
#include "version.h"

// How the exported functions are declared. Windows uses __stdcall
// (WINAPI) as declared in canal.h, and names are exported with the .def
// file so they are not decorated. Elsewhere the functions are made visible
// even when the library is built with hidden visibility.
#ifdef _WIN32
#define CANAL_EXPORT
#define CANAL_CALL WINAPI
#else
#define CANAL_EXPORT __attribute__((visibility("default")))
#define CANAL_CALL
#endif

#define DRIVER_VENDOR "Grodans Paradis AB, Sweden, https://www.grodansparadis.com"

// Version as the CANAL specification wants it: major in the most
// significant byte, then minor, release and build.
#define CANAL_VERSION_NUMBER(major, minor, release, build)                                                             \
  ((((unsigned long) (major) & 0xff) << 24) | (((unsigned long) (minor) & 0xff) << 16) |                              \
   (((unsigned long) (release) & 0xff) << 8) | ((unsigned long) (build) & 0xff))

///////////////////////////////////////////////////////////////////////////////
// Open interfaces
//
// Handles start at 1 (CanalOpen returns <= 0 on failure) and are never
// reused while the library is loaded.

namespace {

std::mutex g_mutex;
std::map<long, std::shared_ptr<CTemplateDriver>> g_drivers;
long g_nextHandle = 1;

std::shared_ptr<CTemplateDriver>
getDriver(long handle)
{
  std::lock_guard<std::mutex> lock(g_mutex);
  auto it = g_drivers.find(handle);
  return (g_drivers.end() == it) ? nullptr : it->second;
}

// Classic frame <-> FD frame structure used inside the driver
canalMsgFD
toFd(const canalMsg &msg)
{
  canalMsgFD fd;
  std::memset(&fd, 0, sizeof(fd));
  fd.flags     = (uint32_t) (msg.flags & ~CANAL_IDFLAG_FD);
  fd.obid      = (uint32_t) msg.obid;
  fd.id        = (uint32_t) msg.id;
  fd.timestamp = (uint32_t) msg.timestamp;
  fd.sizeData  = msg.sizeData;
  std::memcpy(fd.data, msg.data, (msg.sizeData <= 8) ? msg.sizeData : 8);
  return fd;
}

void
fromFd(const canalMsgFD &fd, canalMsg &msg)
{
  std::memset(&msg, 0, sizeof(msg));
  msg.flags     = fd.flags;
  msg.obid      = fd.obid;
  msg.id        = fd.id;
  msg.timestamp = fd.timestamp;
  msg.sizeData  = (fd.sizeData <= 8) ? fd.sizeData : 8;
  std::memcpy(msg.data, fd.data, msg.sizeData);
}

} // namespace

extern "C" {

///////////////////////////////////////////////////////////////////////////////
// CanalOpen
//
// pDevice is the configuration string, flags the driver flags.
// Returns a handle > 0, or <= 0 on failure.

CANAL_EXPORT long CANAL_CALL
CanalOpen(const char *pDevice, unsigned long flags)
{
  auto drv = std::make_shared<CTemplateDriver>();
  if (CANAL_ERROR_SUCCESS != drv->open(pDevice, flags)) {
    return 0;
  }

  std::lock_guard<std::mutex> lock(g_mutex);
  long handle       = g_nextHandle++;
  g_drivers[handle] = drv;
  return handle;
}

///////////////////////////////////////////////////////////////////////////////
// CanalClose
//

CANAL_EXPORT int CANAL_CALL
CanalClose(long handle)
{
  std::shared_ptr<CTemplateDriver> drv;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = g_drivers.find(handle);
    if (g_drivers.end() == it) {
      return CANAL_ERROR_NOT_OPEN;
    }
    drv = it->second;
    g_drivers.erase(it);
  }
  drv->close();
  return CANAL_ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// CanalGetLevel
//

CANAL_EXPORT unsigned long CANAL_CALL
CanalGetLevel(long handle)
{
  return getDriver(handle) ? CANAL_LEVEL_STANDARD : 0;
}

///////////////////////////////////////////////////////////////////////////////
// CanalSend / CanalBlockingSend
//

CANAL_EXPORT int CANAL_CALL
CanalSend(long handle, PCANALMSG pCanalMsg)
{
  auto drv = getDriver(handle);
  if (!drv) {
    return CANAL_ERROR_NOT_OPEN;
  }
  if (nullptr == pCanalMsg) {
    return CANAL_ERROR_PARAMETER;
  }
  if (pCanalMsg->flags & CANAL_IDFLAG_FD) {
    return CANAL_ERROR_PARAMETER; // Use CanalSendFD
  }
  return drv->send(toFd(*pCanalMsg), 0);
}

CANAL_EXPORT int CANAL_CALL
CanalBlockingSend(long handle, PCANALMSG pCanalMsg, unsigned long timeout)
{
  auto drv = getDriver(handle);
  if (!drv) {
    return CANAL_ERROR_NOT_OPEN;
  }
  if (nullptr == pCanalMsg) {
    return CANAL_ERROR_PARAMETER;
  }
  if (pCanalMsg->flags & CANAL_IDFLAG_FD) {
    return CANAL_ERROR_PARAMETER;
  }
  return drv->send(toFd(*pCanalMsg), (uint32_t) timeout);
}

///////////////////////////////////////////////////////////////////////////////
// CanalReceive / CanalBlockingReceive
//

CANAL_EXPORT int CANAL_CALL
CanalReceive(long handle, PCANALMSG pCanalMsg)
{
  auto drv = getDriver(handle);
  if (!drv) {
    return CANAL_ERROR_NOT_OPEN;
  }
  if (nullptr == pCanalMsg) {
    return CANAL_ERROR_PARAMETER;
  }
  canalMsgFD fd;
  int rv = drv->receive(fd, false, false, 0);
  if (CANAL_ERROR_SUCCESS == rv) {
    fromFd(fd, *pCanalMsg);
  }
  return rv;
}

CANAL_EXPORT int CANAL_CALL
CanalBlockingReceive(long handle, PCANALMSG pCanalMsg, unsigned long timeout)
{
  auto drv = getDriver(handle);
  if (!drv) {
    return CANAL_ERROR_NOT_OPEN;
  }
  if (nullptr == pCanalMsg) {
    return CANAL_ERROR_PARAMETER;
  }
  canalMsgFD fd;
  int rv = drv->receive(fd, false, true, (uint32_t) timeout);
  if (CANAL_ERROR_SUCCESS == rv) {
    fromFd(fd, *pCanalMsg);
  }
  return rv;
}

///////////////////////////////////////////////////////////////////////////////
// CanalDataAvailable
//
// Number of frames waiting. 0 also for an invalid handle (the API has no
// error return here).

CANAL_EXPORT int CANAL_CALL
CanalDataAvailable(long handle)
{
  auto drv = getDriver(handle);
  return drv ? drv->dataAvailable(false) : 0;
}

///////////////////////////////////////////////////////////////////////////////
// CanalGetStatus / CanalGetStatistics
//

CANAL_EXPORT int CANAL_CALL
CanalGetStatus(long handle, PCANALSTATUS pCanalStatus)
{
  auto drv = getDriver(handle);
  if (!drv) {
    return CANAL_ERROR_NOT_OPEN;
  }
  if (nullptr == pCanalStatus) {
    return CANAL_ERROR_PARAMETER;
  }
  drv->getStatus(*pCanalStatus);
  return CANAL_ERROR_SUCCESS;
}

CANAL_EXPORT int CANAL_CALL
CanalGetStatistics(long handle, PCANALSTATISTICS pCanalStatistics)
{
  auto drv = getDriver(handle);
  if (!drv) {
    return CANAL_ERROR_NOT_OPEN;
  }
  if (nullptr == pCanalStatistics) {
    return CANAL_ERROR_PARAMETER;
  }
  drv->getStatistics(*pCanalStatistics);
  return CANAL_ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// CanalSetFilter / CanalSetMask / CanalSetBaudrate
//

CANAL_EXPORT int CANAL_CALL
CanalSetFilter(long handle, unsigned long filter)
{
  auto drv = getDriver(handle);
  if (!drv) {
    return CANAL_ERROR_NOT_OPEN;
  }
  drv->setFilter((uint32_t) filter);
  return CANAL_ERROR_SUCCESS;
}

CANAL_EXPORT int CANAL_CALL
CanalSetMask(long handle, unsigned long mask)
{
  auto drv = getDriver(handle);
  if (!drv) {
    return CANAL_ERROR_NOT_OPEN;
  }
  drv->setMask((uint32_t) mask);
  return CANAL_ERROR_SUCCESS;
}

CANAL_EXPORT int CANAL_CALL
CanalSetBaudrate(long handle, unsigned long baudrate)
{
  auto drv = getDriver(handle);
  if (!drv) {
    return CANAL_ERROR_NOT_OPEN;
  }
  return drv->setBaudrate((uint32_t) baudrate);
}

///////////////////////////////////////////////////////////////////////////////
// Versions, vendor and driver information
//

CANAL_EXPORT unsigned long CANAL_CALL
CanalGetVersion(void)
{
  return CANAL_VERSION_NUMBER(CANAL_MAIN_VERSION, CANAL_MINOR_VERSION, CANAL_SUB_VERSION, 0);
}

CANAL_EXPORT unsigned long CANAL_CALL
CanalGetDllVersion(void)
{
  return CANAL_VERSION_NUMBER(DRIVER_VERSION_YEAR, DRIVER_VERSION_MONTH, (DRIVER_VERSION_PATCH >> 8),
                              DRIVER_VERSION_PATCH);
}

CANAL_EXPORT const char *CANAL_CALL
CanalGetVendorString(void)
{
  return DRIVER_VENDOR;
}

CANAL_EXPORT const char *CANAL_CALL
CanalGetDriverInfo(void)
{
  return DRIVER_INFO_XML;
}

///////////////////////////////////////////////////////////////////////////////
//                    CAN FD extension (optional, CANAL 1.1)
//
// Remove these four functions (and their lines in the .def file) for a
// driver whose hardware can not do CAN FD. Hosts then use the standard
// functions only.
///////////////////////////////////////////////////////////////////////////////

CANAL_EXPORT int CANAL_CALL
CanalSendFD(long handle, PCANALMSGFD pCanalMsgFD)
{
  auto drv = getDriver(handle);
  if (!drv) {
    return CANAL_ERROR_NOT_OPEN;
  }
  if (nullptr == pCanalMsgFD) {
    return CANAL_ERROR_PARAMETER;
  }
  return drv->send(*pCanalMsgFD, 0);
}

CANAL_EXPORT int CANAL_CALL
CanalBlockingSendFD(long handle, PCANALMSGFD pCanalMsgFD, unsigned long timeout)
{
  auto drv = getDriver(handle);
  if (!drv) {
    return CANAL_ERROR_NOT_OPEN;
  }
  if (nullptr == pCanalMsgFD) {
    return CANAL_ERROR_PARAMETER;
  }
  return drv->send(*pCanalMsgFD, (uint32_t) timeout);
}

CANAL_EXPORT int CANAL_CALL
CanalReceiveFD(long handle, PCANALMSGFD pCanalMsgFD)
{
  auto drv = getDriver(handle);
  if (!drv) {
    return CANAL_ERROR_NOT_OPEN;
  }
  if (nullptr == pCanalMsgFD) {
    return CANAL_ERROR_PARAMETER;
  }
  return drv->receive(*pCanalMsgFD, true, false, 0);
}

CANAL_EXPORT int CANAL_CALL
CanalBlockingReceiveFD(long handle, PCANALMSGFD pCanalMsgFD, unsigned long timeout)
{
  auto drv = getDriver(handle);
  if (!drv) {
    return CANAL_ERROR_NOT_OPEN;
  }
  if (nullptr == pCanalMsgFD) {
    return CANAL_ERROR_PARAMETER;
  }
  return drv->receive(*pCanalMsgFD, true, true, (uint32_t) timeout);
}

} // extern "C"
