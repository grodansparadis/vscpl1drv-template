// template-driver.h
//
// The driver object of the VSCP Level I (CANAL) driver template.
//
// One CTemplateDriver is created for every CanalOpen. It owns the
// connection to the hardware, a worker thread that reads from it and the
// receive queue the CANAL functions take frames from.
//
// The template is a working loopback driver: frames that are sent are put
// in the receive queue, and an optional test frame is received at a fixed
// interval. To make a driver for real hardware, replace the parts marked
// HARDWARE in template-driver.cpp (hwOpen, hwClose, hwWrite, hwRead,
// hwSetBaudrate). Everything else - queueing, filtering, statistics,
// blocking calls and the CANAL API in canal-api.cpp - can usually stay.
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

#ifndef TEMPLATE_DRIVER_H
#define TEMPLATE_DRIVER_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

#include <canal.h>

// Flags given to CanalOpen
#define TEMPLATE_FLAG_NO_LOOPBACK 0x00000001 // Sent frames are not received again
#define TEMPLATE_FLAG_FD          0x00000002 // Open the interface for CAN FD

// Defaults for the configuration string
#define TEMPLATE_DEFAULT_QUEUE_SIZE 1000

class CTemplateDriver {
public:
  CTemplateDriver();
  ~CTemplateDriver();

  /*!
    Open the interface.
    @param config CANAL configuration string (see parseConfig)
    @param flags CanalOpen flags (TEMPLATE_FLAG_*)
    @return CANAL_ERROR_SUCCESS or a CANAL error code
  */
  int open(const char *config, unsigned long flags);

  /*!
    Close the interface. Safe to call more than once.
  */
  void close(void);

  /*!
    Send a frame. Classic frames have sizeData <= 8 and no
    CANAL_IDFLAG_FD; FD frames need TEMPLATE_FLAG_FD at open.
    @param msg Frame to send
    @param timeout Milliseconds to wait for room in the transmit path,
           0 = do not wait (CanalSend), CANAL_BLOCK_FOREVER is not used here
    @return CANAL_ERROR_SUCCESS or a CANAL error code
  */
  int send(const canalMsgFD &msg, uint32_t timeout = 0);

  /*!
    Get the next received frame that passes the filter.
    @param msg Receives the frame
    @param fd true when called from the FD functions. Classic calls never
           get FD frames; they are skipped.
    @param block true to wait for a frame
    @param timeout Milliseconds to wait when blocking, 0 = forever
    @return CANAL_ERROR_SUCCESS, CANAL_ERROR_FIFO_EMPTY (not blocking) or
            CANAL_ERROR_TIMEOUT
  */
  int receive(canalMsgFD &msg, bool fd, bool block, uint32_t timeout);

  /*!
    Number of frames waiting (classic frames only when fd is false)
  */
  int dataAvailable(bool fd);

  void setFilter(uint32_t filter);
  void setMask(uint32_t mask);
  int setBaudrate(uint32_t baudrate);

  void getStatus(canalStatus &status);
  void getStatistics(canalStatistics &statistics);

  bool isOpen(void) const { return m_bOpen; }
  bool isFd(void) const { return m_flags & TEMPLATE_FLAG_FD; }

private:
  /*!
    Parse the CANAL configuration string "interval;id;queue-size", all
    optional (see docs/configure.md).
  */
  bool parseConfig(const char *config);

  // Put a received frame in the queue (filter, statistics, overrun)
  void addReceived(const canalMsgFD &msg);

  // Set the last error reported by CanalGetStatus
  void setError(unsigned long code, const std::string &text);

  // The worker thread reads from the hardware
  void workerThread(void);

  // --- HARDWARE: replace these for real hardware ---------------------------
  bool hwOpen(void);
  void hwClose(void);
  int hwWrite(const canalMsgFD &msg);
  bool hwRead(canalMsgFD &msg, uint32_t timeout);
  int hwSetBaudrate(uint32_t baudrate);
  // -------------------------------------------------------------------------

  // Configuration
  unsigned long m_flags;
  uint32_t m_interval;  // ms between test frames, 0 = off
  uint32_t m_testId;    // CAN id of the test frame
  size_t m_maxQueue;    // Max frames in the receive queue

  std::atomic<bool> m_bOpen;
  std::atomic<bool> m_bRun;
  std::thread m_worker;

  // Receive queue
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::deque<canalMsgFD> m_queue;

  // Filter/mask: a frame passes when ((id ^ filter) & mask) == 0
  std::atomic<uint32_t> m_filter;
  std::atomic<uint32_t> m_mask;

  // Status and statistics (protected by m_mutex)
  canalStatistics m_stat;
  canalStatus m_status;

  std::chrono::steady_clock::time_point m_start; // Time stamp base

  // HARDWARE: the loopback "hardware" is a queue of written frames
  std::mutex m_hwMutex;
  std::condition_variable m_hwCv;
  std::deque<canalMsgFD> m_hwLoop;
  std::chrono::steady_clock::time_point m_nextTest;
  uint8_t m_testCounter;
};

#endif
