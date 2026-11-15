// test-driver.cpp
//
// Tests the built driver the way a host uses it: the shared library is
// loaded at run time and the CANAL functions are looked up by name.
//
//   test-driver path/to/vscpl1drv-template.so
//
// Run by ctest. Exit status 0 when all checks pass.
//
// This file is part of the VSCP (https://www.vscp.org)
// SPDX-License-Identifier: MIT
// Copyright (C) 2000-2026 Ake Hedman, Grodans Paradis AB <info@grodansparadis.com>
//

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#define CALLCONV __stdcall
#else
#include <dlfcn.h>
#define CALLCONV
#endif

#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

#include <canal.h>

// The CANAL functions as the host sees them
typedef long(CALLCONV *fnOpen)(const char *, unsigned long);
typedef int(CALLCONV *fnClose)(long);
typedef unsigned long(CALLCONV *fnGetLevel)(long);
typedef int(CALLCONV *fnSend)(long, PCANALMSG);
typedef int(CALLCONV *fnBlockingSend)(long, PCANALMSG, unsigned long);
typedef int(CALLCONV *fnReceive)(long, PCANALMSG);
typedef int(CALLCONV *fnBlockingReceive)(long, PCANALMSG, unsigned long);
typedef int(CALLCONV *fnDataAvailable)(long);
typedef int(CALLCONV *fnGetStatus)(long, PCANALSTATUS);
typedef int(CALLCONV *fnGetStatistics)(long, PCANALSTATISTICS);
typedef int(CALLCONV *fnSetUlong)(long, unsigned long);
typedef unsigned long(CALLCONV *fnGetUlong)(void);
typedef const char *(CALLCONV *fnGetString)(void);
typedef int(CALLCONV *fnSendFD)(long, PCANALMSGFD);
typedef int(CALLCONV *fnReceiveFD)(long, PCANALMSGFD);
typedef int(CALLCONV *fnBlockingReceiveFD)(long, PCANALMSGFD, unsigned long);

static int g_failed = 0;
static int g_checks = 0;

#define CHECK(cond)                                                                                                    \
  do {                                                                                                                 \
    g_checks++;                                                                                                        \
    if (!(cond)) {                                                                                                     \
      g_failed++;                                                                                                      \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                                      \
    }                                                                                                                  \
  } while (0)

struct Api {
  fnOpen open;
  fnClose close;
  fnGetLevel getLevel;
  fnSend send;
  fnBlockingSend blockingSend;
  fnReceive receive;
  fnBlockingReceive blockingReceive;
  fnDataAvailable dataAvailable;
  fnGetStatus getStatus;
  fnGetStatistics getStatistics;
  fnSetUlong setFilter;
  fnSetUlong setMask;
  fnSetUlong setBaudrate;
  fnGetUlong getVersion;
  fnGetUlong getDllVersion;
  fnGetString getVendor;
  fnGetString getDriverInfo;
  fnSendFD sendFD;
  fnReceiveFD receiveFD;
  fnBlockingReceiveFD blockingReceiveFD;
};

#ifdef _WIN32
static HMODULE g_lib;
static void *
sym(const char *name)
{
  return (void *) GetProcAddress(g_lib, name);
}
#else
static void *g_lib;
static void *
sym(const char *name)
{
  return dlsym(g_lib, name);
}
#endif

template<typename T>
static bool
load(T &fn, const char *name)
{
  fn = (T) sym(name);
  if (nullptr == fn) {
    std::printf("FAIL: the driver does not export %s\n", name);
    g_failed++;
    return false;
  }
  return true;
}

static canalMsg
frame(unsigned long id, unsigned char n, unsigned long flags = CANAL_IDFLAG_STANDARD)
{
  canalMsg m;
  std::memset(&m, 0, sizeof(m));
  m.flags    = flags;
  m.id       = id;
  m.sizeData = n;
  for (unsigned char i = 0; i < n; i++) {
    m.data[i] = (unsigned char) (0x10 + i);
  }
  return m;
}

int
main(int argc, char **argv)
{
  if (argc < 2) {
    std::printf("Usage: %s path-to-driver\n", argv[0]);
    return 2;
  }

#ifdef _WIN32
  g_lib = LoadLibraryA(argv[1]);
#else
  g_lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
#endif
  if (!g_lib) {
    std::printf("FAIL: unable to load %s\n", argv[1]);
    return 1;
  }

  Api a;
  bool ok = load(a.open, "CanalOpen") & load(a.close, "CanalClose") & load(a.getLevel, "CanalGetLevel") &
            load(a.send, "CanalSend") & load(a.blockingSend, "CanalBlockingSend") &
            load(a.receive, "CanalReceive") & load(a.blockingReceive, "CanalBlockingReceive") &
            load(a.dataAvailable, "CanalDataAvailable") & load(a.getStatus, "CanalGetStatus") &
            load(a.getStatistics, "CanalGetStatistics") & load(a.setFilter, "CanalSetFilter") &
            load(a.setMask, "CanalSetMask") & load(a.setBaudrate, "CanalSetBaudrate") &
            load(a.getVersion, "CanalGetVersion") & load(a.getDllVersion, "CanalGetDllVersion") &
            load(a.getVendor, "CanalGetVendorString") & load(a.getDriverInfo, "CanalGetDriverInfo") &
            load(a.sendFD, "CanalSendFD") & load(a.receiveFD, "CanalReceiveFD") &
            load(a.blockingReceiveFD, "CanalBlockingReceiveFD");
  if (!ok) {
    return 1;
  }

  // --- Information ---------------------------------------------------------
  CHECK((a.getVersion() >> 24) == CANAL_MAIN_VERSION);
  CHECK(((a.getVersion() >> 16) & 0xff) == CANAL_MINOR_VERSION);
  CHECK(a.getDllVersion() != 0);
  CHECK(std::strlen(a.getVendor()) > 0);
  CHECK(std::strstr(a.getDriverInfo(), "<config>") != nullptr);
  CHECK(std::strstr(a.getDriverInfo(), "<level>1</level>") != nullptr);

  // --- Invalid configuration and handles -----------------------------------
  CHECK(a.open("abc", 0) <= 0);
  CHECK(a.open("10;0x20000000", 0) <= 0); // Id too large
  canalMsg m = frame(0x100, 2);
  CHECK(a.send(9999, &m) == CANAL_ERROR_NOT_OPEN);
  CHECK(a.close(9999) == CANAL_ERROR_NOT_OPEN);
  CHECK(a.getLevel(9999) == 0);

  // --- Loopback ------------------------------------------------------------
  long h = a.open("", 0);
  CHECK(h > 0);
  CHECK(a.getLevel(h) == CANAL_LEVEL_STANDARD);
  CHECK(a.setBaudrate(h, CANAL_BAUD_125) == CANAL_ERROR_SUCCESS);

  canalMsg rx;
  CHECK(a.receive(h, &rx) == CANAL_ERROR_FIFO_EMPTY);

  m = frame(0x123, 3);
  CHECK(a.send(h, &m) == CANAL_ERROR_SUCCESS);
  CHECK(a.blockingReceive(h, &rx, 1000) == CANAL_ERROR_SUCCESS);
  CHECK(rx.id == 0x123);
  CHECK(rx.sizeData == 3);
  CHECK(0 == std::memcmp(rx.data, m.data, 3));
  CHECK(a.dataAvailable(h) == 0);

  m = frame(0x1abcdef, 8, CANAL_IDFLAG_EXTENDED);
  CHECK(a.blockingSend(h, &m, 100) == CANAL_ERROR_SUCCESS);
  CHECK(a.blockingReceive(h, &rx, 1000) == CANAL_ERROR_SUCCESS);
  CHECK((rx.id == 0x1abcdef) && (rx.flags & CANAL_IDFLAG_EXTENDED) && (rx.sizeData == 8));

  // Invalid frames
  m = frame(0x800, 1); // Standard id too large
  CHECK(a.send(h, &m) == CANAL_ERROR_CAN_ID);
  m          = frame(0x100, 1);
  m.sizeData = 9;
  CHECK(a.send(h, &m) == CANAL_ERROR_MTU);
  CHECK(a.blockingReceive(h, &rx, 100) == CANAL_ERROR_TIMEOUT);

  // --- Filter and mask -----------------------------------------------------
  CHECK(a.setFilter(h, 0x100) == CANAL_ERROR_SUCCESS);
  CHECK(a.setMask(h, 0x7ff) == CANAL_ERROR_SUCCESS);
  m = frame(0x200, 1);
  a.send(h, &m);
  m = frame(0x100, 1);
  a.send(h, &m);
  CHECK(a.blockingReceive(h, &rx, 1000) == CANAL_ERROR_SUCCESS);
  CHECK(rx.id == 0x100); // 0x200 was filtered out
  CHECK(a.blockingReceive(h, &rx, 100) == CANAL_ERROR_TIMEOUT);
  a.setMask(h, 0);

  // --- Statistics and status -----------------------------------------------
  canalStatistics st;
  CHECK(a.getStatistics(h, &st) == CANAL_ERROR_SUCCESS);
  CHECK(st.cntTransmitFrames == 4);
  CHECK(st.cntReceiveFrames == 3);
  CHECK(st.cntTransmitData == 3 + 8 + 1 + 1);
  canalStatus status;
  CHECK(a.getStatus(h, &status) == CANAL_ERROR_SUCCESS);
  CHECK(status.channel_status & CANAL_STATUS_ACTIVE);

  // --- CAN FD --------------------------------------------------------------
  canalMsgFD fd;
  std::memset(&fd, 0, sizeof(fd));
  fd.flags    = CANAL_IDFLAG_FD;
  fd.id       = 0x321;
  fd.sizeData = 64;
  CHECK(a.sendFD(h, &fd) == CANAL_ERROR_FD_SUPPORT); // Not opened for FD
  CHECK(a.close(h) == CANAL_ERROR_SUCCESS);
  CHECK(a.close(h) == CANAL_ERROR_NOT_OPEN);

  h = a.open("", 2); // TEMPLATE_FLAG_FD
  CHECK(h > 0);
  for (int i = 0; i < 64; i++) {
    fd.data[i] = (uint8_t) i;
  }
  CHECK(a.sendFD(h, &fd) == CANAL_ERROR_SUCCESS);
  m = frame(0x55, 2);
  CHECK(a.send(h, &m) == CANAL_ERROR_SUCCESS);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  // A classic receive skips the FD frame
  CHECK(a.dataAvailable(h) == 1);
  CHECK(a.receive(h, &rx) == CANAL_ERROR_SUCCESS);
  CHECK(rx.id == 0x55);
  // FD receive gets FD frames
  CHECK(a.sendFD(h, &fd) == CANAL_ERROR_SUCCESS);
  canalMsgFD rxfd;
  CHECK(a.blockingReceiveFD(h, &rxfd, 1000) == CANAL_ERROR_SUCCESS);
  CHECK((rxfd.id == 0x321) && (rxfd.sizeData == 64) && (rxfd.flags & CANAL_IDFLAG_FD) && (rxfd.data[63] == 63));
  fd.sizeData = 13; // Not a valid CAN FD length
  CHECK(a.sendFD(h, &fd) == CANAL_ERROR_MTU);
  a.close(h);

  // --- No loopback ---------------------------------------------------------
  h = a.open("", 1); // TEMPLATE_FLAG_NO_LOOPBACK
  CHECK(h > 0);
  m = frame(0x10, 1);
  CHECK(a.send(h, &m) == CANAL_ERROR_SUCCESS);
  CHECK(a.blockingReceive(h, &rx, 200) == CANAL_ERROR_TIMEOUT);
  a.close(h);

  // --- Test frames ---------------------------------------------------------
  h = a.open("20;0x456;10", 1);
  CHECK(h > 0);
  CHECK(a.blockingReceive(h, &rx, 2000) == CANAL_ERROR_SUCCESS);
  CHECK((rx.id == 0x456) && (rx.sizeData == 1));
  unsigned char first = rx.data[0];
  CHECK(a.blockingReceive(h, &rx, 2000) == CANAL_ERROR_SUCCESS);
  CHECK(rx.data[0] == (unsigned char) (first + 1));
  // The queue holds at most 10 frames; the rest are overruns
  std::this_thread::sleep_for(std::chrono::milliseconds(400));
  CHECK(a.dataAvailable(h) == 10);
  CHECK(a.getStatistics(h, &st) == CANAL_ERROR_SUCCESS);
  CHECK(st.cntOverruns > 0);
  a.close(h);

  // --- Close while another thread is blocked in a receive ------------------
  h = a.open("", 1);
  CHECK(h > 0);
  int blockedRv = -1;
  std::thread t([&]() {
    canalMsg msg;
    blockedRv = a.blockingReceive(h, &msg, 0); // Forever
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  CHECK(a.close(h) == CANAL_ERROR_SUCCESS);
  t.join();
  CHECK(blockedRv == CANAL_ERROR_NOT_OPEN);

  // --- Several interfaces at the same time ---------------------------------
  long h1 = a.open("", 0);
  long h2 = a.open("", 0);
  CHECK((h1 > 0) && (h2 > 0) && (h1 != h2));
  m = frame(0x77, 1);
  a.send(h1, &m);
  CHECK(a.blockingReceive(h1, &rx, 1000) == CANAL_ERROR_SUCCESS);
  CHECK(a.blockingReceive(h2, &rx, 100) == CANAL_ERROR_TIMEOUT); // Separate loopbacks
  a.close(h1);
  a.close(h2);

  std::printf("%d checks, %d failed\n", g_checks, g_failed);
  return g_failed ? 1 : 0;
}
