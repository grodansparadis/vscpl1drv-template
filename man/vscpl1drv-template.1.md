% VSCPL1DRV-TEMPLATE(1) VSCP Level I driver
% Åke Hedman, Grodans Paradis AB
% November 2026

# NAME

vscpl1drv-template - VSCP Level I (CANAL) driver template, a loopback driver

# SYNOPSIS

**vscpl1drv-template.so** (Linux), **vscpl1drv-template.dylib** (macOS),
**vscpl1drv-template.dll** (Windows)

A CANAL driver. It is loaded by the VSCP daemon (**vscpd**(8)), VSCP Works,
**vscp-cli**, the VSCP simulator or any other program that uses the CANAL
API. The host passes a configuration string and flags to **CanalOpen**.

# DESCRIPTION

**vscpl1drv-template** is the starting point for new VSCP Level I drivers.
It implements the whole CANAL API (version 1.1, including the optional CAN
FD functions) on top of a small driver object, and works as a loopback
driver: frames that are sent are received again, and a test frame can be
received at a fixed interval. This makes it useful for testing hosts and
setups without hardware.

To make a driver for real hardware, copy the template with
**tools/new_driver.py** and replace the functions marked HARDWARE in
*src/NAME-driver.cpp*. See the documentation in the *docs* folder.

Several interfaces can be open at the same time; each has its own loopback.

# CONFIGURATION STRING

The configuration string is a semicolon separated list. All parts are
optional; numbers are decimal or hex (0x...).

    interval;id;queue-size

*interval*
:   Milliseconds between test frames, 0 to 3600000. 0 (default) sends none.
    The test frame has one data byte that counts up.

*id*
:   CAN id of the test frame, default 0x123. Ids above 0x7ff are sent as
    extended (29-bit) frames.

*queue-size*
:   Size of the receive queue in frames, 1 to 1000000, default 1000. Frames
    that arrive when the queue is full are counted as overruns.

An invalid configuration string makes **CanalOpen** fail.

# FLAGS

Bit 0 (1)
:   Do not loop sent frames back.

Bit 1 (2)
:   Open the interface for CAN FD. Without it **CanalSendFD** of an FD frame
    returns CANAL_ERROR_FD_SUPPORT.

# FILTER AND MASK

**CanalSetFilter** and **CanalSetMask** select the received frames by id. A
frame is received when ((id XOR filter) AND mask) is zero, that is, mask bits
that are one must match the filter. The default mask 0 receives everything.

# CAN FD

Hosts that use the standard functions (**CanalReceive**,
**CanalBlockingReceive**) never get CAN FD frames; they are skipped. The FD
functions (**CanalSendFD**, **CanalBlockingSendFD**, **CanalReceiveFD**,
**CanalBlockingReceiveFD**) handle both classic and FD frames. FD frames can
have 0-8, 12, 16, 20, 24, 32, 48 or 64 data bytes.

# EXAMPLES

Configuration of the driver in the VSCP daemon (*/etc/vscp/vscpd.json*):

    "drivers": {
      "level1": [
        {
          "enable": true,
          "name": "loopback",
          "config": "1000;0x123",
          "flags": 0,
          "translation": 0,
          "path": "/var/lib/vscp/drivers/level1/vscpl1drv-template.so",
          "guid": "FF:FF:FF:FF:FF:FF:FF:F5:00:00:00:00:00:00:00:00"
        }
      ]
    }

Watch the test frames with **vscp-dump**(1):

    vscp-dump -i canal --driver /var/lib/vscp/drivers/level1/vscpl1drv-template.so \
              --driver-config "1000;0x123"

# FILES

*/var/lib/vscp/drivers/level1/vscpl1drv-template.so*
:   The driver (Linux).

# SEE ALSO

**vscpd**(8), **vscp-dump**(1), **vscp-send**(1), **vscpsim**(1)

The CANAL API: <https://grodansparadis.github.io/vscp-doc-canal>

The source code and the documentation:
<https://github.com/grodansparadis/vscpl1drv-template>

# COPYRIGHT

Copyright 2000-2026 Åke Hedman, Grodans Paradis AB. MIT license.
