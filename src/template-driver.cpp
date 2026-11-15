// template-driver.cpp
//
// The driver object of the VSCP Level I (CANAL) driver template.
// See template-driver.h.
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

#include "template-driver.h"

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Helpers
//

// Split "a;b;c" into its parts (empty parts kept)
static std::vector<std::string>
splitConfig(const std::string &str)
{
  std::vector<std::string> parts;
  std::string part;
  std::istringstream in(str);
  while (std::getline(in, part, ';')) {
    // Trim white space
    size_t b = part.find_first_not_of(" \t\r\n");
    size_t e = part.find_last_not_of(" \t\r\n");
    parts.push_back((std::string::npos == b) ? std::string() : part.substr(b, e - b + 1));
  }
  return parts;
}

// Parse a decimal or 0x hex number. Empty gives the default.
static bool
parseNumber(const std::string &str, uint32_t def, uint32_t max, uint32_t &value)
{
  if (str.empty()) {
    value = def;
    return true;
  }
  char *end              = nullptr;
  unsigned long long val = std::strtoull(str.c_str(), &end, 0); // 0x.. = hex
  if ((nullptr == end) || ('\0' != *end) || (val > max)) {
    return false;
  }
  value = (uint32_t) val;
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// CTemplateDriver
//

CTemplateDriver::CTemplateDriver()
  : m_flags(0)
  , m_interval(0)
  , m_testId(0x123)
  , m_maxQueue(TEMPLATE_DEFAULT_QUEUE_SIZE)
  , m_bOpen(false)
  , m_bRun(false)
  , m_filter(0)
  , m_mask(0)
  , m_testCounter(0)
{
  std::memset(&m_stat, 0, sizeof(m_stat));
  std::memset(&m_status, 0, sizeof(m_status));
}

CTemplateDriver::~CTemplateDriver()
{
  close();
}

bool
CTemplateDriver::parseConfig(const char *config)
{
  std::vector<std::string> parts = splitConfig((nullptr == config) ? "" : config);
  parts.resize(3); // Missing parts are empty, i.e. defaults

  uint32_t queue;
  if (!parseNumber(parts[0], 0, 3600000, m_interval)) {
    setError(CANAL_ERROR_PARAMETER, "Invalid test frame interval (part 1 of the configuration)");
    return false;
  }
  if (!parseNumber(parts[1], 0x123, CAN_MAX_EXTENDED_ID, m_testId)) {
    setError(CANAL_ERROR_PARAMETER, "Invalid test frame id (part 2 of the configuration)");
    return false;
  }
  if (!parseNumber(parts[2], TEMPLATE_DEFAULT_QUEUE_SIZE, 1000000, queue) || (0 == queue)) {
    setError(CANAL_ERROR_PARAMETER, "Invalid receive queue size (part 3 of the configuration)");
    return false;
  }
  m_maxQueue = queue;
  return true;
}

int
CTemplateDriver::open(const char *config, unsigned long flags)
{
  if (m_bOpen) {
    return CANAL_ERROR_INIT_READY;
  }

  m_flags = flags;
  if (!parseConfig(config)) {
    return CANAL_ERROR_PARAMETER;
  }

  m_start = std::chrono::steady_clock::now();
  if (!hwOpen()) {
    setError(CANAL_ERROR_HARDWARE, "Unable to open the hardware");
    return CANAL_ERROR_HARDWARE;
  }

  m_bRun   = true;
  m_worker = std::thread(&CTemplateDriver::workerThread, this);
  m_bOpen  = true;

  std::lock_guard<std::mutex> lock(m_mutex);
  m_status.channel_status = CANAL_STATUS_ACTIVE;
  return CANAL_ERROR_SUCCESS;
}

void
CTemplateDriver::close(void)
{
  if (!m_bRun && !m_worker.joinable()) {
    return;
  }

  m_bRun = false;
  m_hwCv.notify_all();
  if (m_worker.joinable()) {
    m_worker.join();
  }
  hwClose();
  m_bOpen = false;

  // Wake blocking receivers so they can return
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_status.channel_status = CANAL_STATUS_STOPPED;
  }
  m_cv.notify_all();
}

int
CTemplateDriver::send(const canalMsgFD &msg, uint32_t /*timeout*/)
{
  if (!m_bOpen) {
    return CANAL_ERROR_NOT_OPEN;
  }

  const bool fdFrame = (msg.flags & CANAL_IDFLAG_FD);
  if (fdFrame && !isFd()) {
    return CANAL_ERROR_FD_SUPPORT;
  }
  if (fdFrame) {
    // CAN FD has these lengths only
    static const uint8_t fdSizes[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64 };
    bool ok = false;
    for (uint8_t n : fdSizes) {
      ok = ok || (n == msg.sizeData);
    }
    if (!ok) {
      return CANAL_ERROR_MTU;
    }
  }
  else if (msg.sizeData > 8) {
    return CANAL_ERROR_MTU;
  }
  const uint32_t maxId = (msg.flags & CANAL_IDFLAG_EXTENDED) ? CAN_MAX_EXTENDED_ID : CAN_MAX_STANDARD_ID;
  if (msg.id > maxId) {
    return CANAL_ERROR_CAN_ID;
  }

  int rv = hwWrite(msg);
  if (CANAL_ERROR_SUCCESS == rv) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stat.cntTransmitFrames++;
    m_stat.cntTransmitData += msg.sizeData;
  }
  return rv;
}

void
CTemplateDriver::addReceived(const canalMsgFD &msgIn)
{
  canalMsgFD msg = msgIn;

  // The filter is applied to the id. A mask bit set to one means that
  // bit must be equal to the filter bit. Mask 0 lets everything through.
  if ((msg.id ^ m_filter) & m_mask) {
    return;
  }

  // Time stamp in microseconds (wraps after ~71 minutes, as in CANAL)
  msg.timestamp = (uint32_t) std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - m_start)
                    .count();

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.size() >= m_maxQueue) {
      m_stat.cntOverruns++;
      m_status.channel_status |= CANAL_STATUS_RECIVE_BUFFER_FULL;
      return;
    }
    m_status.channel_status &= ~CANAL_STATUS_RECIVE_BUFFER_FULL;
    m_queue.push_back(msg);
    m_stat.cntReceiveFrames++;
    m_stat.cntReceiveData += msg.sizeData;
  }
  m_cv.notify_one();
}

int
CTemplateDriver::receive(canalMsgFD &msg, bool fd, bool block, uint32_t timeout)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  // Classic callers do not get FD frames: drop them from the head
  auto dropFd = [&]() {
    if (!fd) {
      while (!m_queue.empty() && (m_queue.front().flags & CANAL_IDFLAG_FD)) {
        m_queue.pop_front();
      }
    }
  };

  dropFd();
  if (m_queue.empty()) {
    if (!block) {
      return m_bOpen ? CANAL_ERROR_FIFO_EMPTY : CANAL_ERROR_NOT_OPEN;
    }
    auto ready = [&]() {
      dropFd();
      return !m_queue.empty() || !m_bOpen;
    };
    if (CANAL_BLOCK_FOREVER == timeout) {
      m_cv.wait(lock, ready);
    }
    else if (!m_cv.wait_for(lock, std::chrono::milliseconds(timeout), ready)) {
      return CANAL_ERROR_TIMEOUT;
    }
    if (m_queue.empty()) {
      return CANAL_ERROR_NOT_OPEN; // Closed while waiting
    }
  }

  msg = m_queue.front();
  m_queue.pop_front();
  return CANAL_ERROR_SUCCESS;
}

int
CTemplateDriver::dataAvailable(bool fd)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (fd) {
    return (int) m_queue.size();
  }
  int count = 0;
  for (const auto &m : m_queue) {
    if (!(m.flags & CANAL_IDFLAG_FD)) {
      count++;
    }
  }
  return count;
}

void
CTemplateDriver::setFilter(uint32_t filter)
{
  m_filter = filter;
}

void
CTemplateDriver::setMask(uint32_t mask)
{
  m_mask = mask;
}

int
CTemplateDriver::setBaudrate(uint32_t baudrate)
{
  if (!m_bOpen) {
    return CANAL_ERROR_NOT_OPEN;
  }
  return hwSetBaudrate(baudrate);
}

void
CTemplateDriver::getStatus(canalStatus &status)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  status = m_status;
}

void
CTemplateDriver::getStatistics(canalStatistics &statistics)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  statistics = m_stat;
}

void
CTemplateDriver::setError(unsigned long code, const std::string &text)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_status.lasterrorcode    = code;
  m_status.lasterrorsubcode = 0;
  std::strncpy(m_status.lasterrorstr, text.c_str(), sizeof(m_status.lasterrorstr) - 1);
  m_status.lasterrorstr[sizeof(m_status.lasterrorstr) - 1] = '\0';
}

void
CTemplateDriver::workerThread(void)
{
  canalMsgFD msg;
  while (m_bRun) {
    std::memset(&msg, 0, sizeof(msg));
    if (hwRead(msg, 100)) {
      addReceived(msg);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//                                HARDWARE
//
// The functions below talk to the "hardware". In the template the hardware
// is a loopback: written frames are read back, and a test frame is produced
// every m_interval milliseconds. Replace them with code for your interface
// (serial port, USB, socket, vendor library...).
///////////////////////////////////////////////////////////////////////////////

bool
CTemplateDriver::hwOpen(void)
{
  // Open the device here. Return false on failure.
  m_nextTest = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_interval);
  return true;
}

void
CTemplateDriver::hwClose(void)
{
  // Close the device here.
  std::lock_guard<std::mutex> lock(m_hwMutex);
  m_hwLoop.clear();
}

int
CTemplateDriver::hwWrite(const canalMsgFD &msg)
{
  // Write the frame to the device here. Return CANAL_ERROR_TRM_FULL if the
  // device can not take it right now.
  if (m_flags & TEMPLATE_FLAG_NO_LOOPBACK) {
    return CANAL_ERROR_SUCCESS; // Sent "nowhere"
  }
  {
    std::lock_guard<std::mutex> lock(m_hwMutex);
    if (m_hwLoop.size() >= m_maxQueue) {
      return CANAL_ERROR_TRM_FULL;
    }
    m_hwLoop.push_back(msg);
  }
  m_hwCv.notify_one();
  return CANAL_ERROR_SUCCESS;
}

bool
CTemplateDriver::hwRead(canalMsgFD &msg, uint32_t timeout)
{
  // Wait at most timeout ms for a frame from the device. Return true and
  // fill in msg (flags, id, sizeData, data) when one arrived.
  std::unique_lock<std::mutex> lock(m_hwMutex);

  auto now = std::chrono::steady_clock::now();
  auto end = now + std::chrono::milliseconds(timeout);
  if (m_interval && (m_nextTest < end)) {
    end = m_nextTest;
  }

  m_hwCv.wait_until(lock, end, [&]() { return !m_hwLoop.empty() || !m_bRun; });

  if (!m_hwLoop.empty()) {
    msg = m_hwLoop.front();
    m_hwLoop.pop_front();
    return true;
  }

  // The periodic test frame: one byte that counts up
  if (m_interval && (std::chrono::steady_clock::now() >= m_nextTest)) {
    m_nextTest += std::chrono::milliseconds(m_interval);
    msg.flags    = (m_testId > CAN_MAX_STANDARD_ID) ? CANAL_IDFLAG_EXTENDED : CANAL_IDFLAG_STANDARD;
    msg.id       = m_testId;
    msg.sizeData = 1;
    msg.data[0]  = m_testCounter++;
    return true;
  }

  return false;
}

int
CTemplateDriver::hwSetBaudrate(uint32_t /*baudrate*/)
{
  // Set the bus speed here. The loopback has no bus speed.
  return CANAL_ERROR_SUCCESS;
}
