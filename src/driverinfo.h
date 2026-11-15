// driverinfo.h
//
// What CanalGetDriverInfo returns: an XML description of the configuration
// string and the flags, so that a host such as VSCP Works can show a dialog
// for them. Format: https://grodansparadis.github.io/vscp-doc-canal/#/canalgetdriverinfo
//
// Update it when the configuration string or the flags change.
//
// This file is part of the VSCP (https://www.vscp.org)
// SPDX-License-Identifier: MIT
// Copyright (C) 2000-2026 Ake Hedman, Grodans Paradis AB <info@grodansparadis.com>
//

#ifndef DRIVERINFO_H
#define DRIVERINFO_H

// clang-format off
#define DRIVER_INFO_XML \
"<?xml version = \"1.0\" encoding = \"UTF-8\" ?>\n" \
"<config>\n" \
"  <description format=\"text\">VSCP Level I (CANAL) driver template. A loopback driver: sent frames are received again and a test frame can be received at an interval.</description>\n" \
"  <level>1</level>\n" \
"  <blocking>yes</blocking>\n" \
"  <infourl>https://github.com/grodansparadis/vscpl1drv-template</infourl>\n" \
"  <items>\n" \
"    <item optional=\"true\" type=\"uint32\" min=\"0\" max=\"3600000\" description=\"Interval in milliseconds between test frames. 0 (default) sends none.\" infourl=\"https://github.com/grodansparadis/vscpl1drv-template/blob/master/docs/configure.md\" />\n" \
"    <item optional=\"true\" type=\"uint32\" min=\"0\" max=\"536870911\" description=\"CAN id of the test frame (default 0x123). Ids above 0x7ff are sent as extended frames.\" infourl=\"https://github.com/grodansparadis/vscpl1drv-template/blob/master/docs/configure.md\" />\n" \
"    <item optional=\"true\" type=\"uint32\" min=\"1\" max=\"1000000\" description=\"Size of the receive queue in frames (default 1000).\" infourl=\"https://github.com/grodansparadis/vscpl1drv-template/blob/master/docs/configure.md\" />\n" \
"  </items>\n" \
"  <flags>\n" \
"    <bit pos=\"0\" width=\"1\" type=\"bool\" description=\"Do not loop sent frames back.\" infourl=\"https://github.com/grodansparadis/vscpl1drv-template/blob/master/docs/configure.md\" />\n" \
"    <bit pos=\"1\" width=\"1\" type=\"bool\" description=\"Open the interface for CAN FD frames.\" infourl=\"https://github.com/grodansparadis/vscpl1drv-template/blob/master/docs/configure.md\" />\n" \
"  </flags>\n" \
"</config>\n"
// clang-format on

#endif
