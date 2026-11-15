# How the driver works

```
 host (vscpd, VSCP Works, vscp-cli, ...)
   │  CanalOpen / CanalSend / CanalBlockingReceive / ...      canal-api.cpp
   ▼
 handle table ──► CTemplateDriver (one per CanalOpen)      template-driver.cpp
                    │ send() ───────────────► hwWrite() ─┐
                    │                                     │   HARDWARE
                    │ worker thread: hwRead() ◄───────────┘   (loopback in
                    │      │                                    the template)
                    │      ▼ filter/mask, time stamp, statistics
                    │   receive queue (max queue-size frames)
                    └ receive() ◄── blocking with time-out, wakes on close
```

## canal-api.cpp

The exported functions. `CanalOpen` creates a `CTemplateDriver`, opens it and
stores it in a table under a new handle (1, 2, 3, ...; never reused while the
library is loaded). The other functions look the handle up and call the
object. An unknown handle gives `CANAL_ERROR_NOT_OPEN`.

The table holds `std::shared_ptr`s, so `CanalClose` can remove a driver while
another thread is still inside `CanalBlockingReceive` on it: that call wakes
up, returns `CANAL_ERROR_NOT_OPEN`, and the object is freed when the last
user is done.

Classic frames (`canalMsg`) are converted to the FD structure (`canalMsgFD`)
inside the driver, so the driver object has one code path for both.

### Exports

| Platform | How the functions are exported |
| :------- | :----------------------------- |
| Linux, macOS | The library is built with hidden visibility; `CANAL_EXPORT` makes the CANAL functions visible. |
| Windows | `__stdcall` (`WINAPI`) as declared in `canal.h`, exported by name with `src/vscpl1drv-template.def`, so 32-bit DLLs have undecorated names too. |

## The driver object

`open()` parses the configuration string, opens the hardware and starts the
worker thread. The worker calls `hwRead()` in a loop with a 100 ms time-out
and passes every frame to `addReceived()`, which

1. applies the filter and mask,
2. sets the time stamp (microseconds since open, wraps after ~71 minutes as
   in CANAL),
3. puts the frame in the receive queue, or counts an overrun when the queue
   is full,
4. updates the statistics and wakes one waiting receiver.

`receive()` takes frames from the queue. A classic receiver (`CanalReceive`)
never gets a CAN FD frame; FD frames at the head of the queue are skipped.
Blocking calls wait on a condition variable with the time-out
(`CANAL_BLOCK_FOREVER` = 0 waits forever) and return when a frame arrives or
the interface is closed.

`send()` checks the frame (id range for standard/extended ids, 0-8 data
bytes, or the CAN FD sizes 0-8, 12, 16, 20, 24, 32, 48 and 64 for FD frames
when the interface was opened for FD) and calls `hwWrite()`.

`close()` stops the worker, closes the hardware and wakes all waiting
receivers. It is also called by the destructor.

## Thread safety

Hosts call the CANAL functions from several threads (typically one sending
and one receiving). The queue, status and statistics are protected by
`m_mutex`, the handle table by its own mutex, and filter/mask are atomics.
The hardware functions are called from two threads: `hwWrite` from the
callers of send, `hwRead` from the worker. Protect any state they share.

## The hardware functions

See [Make a new driver](creating-a-driver.md#_2-talk-to-your-hardware). The
loopback implementation in the template shows the contract: `hwWrite` stores
the frame, and `hwRead` waits on a condition variable until a frame is there,
the next test frame is due or the time-out ends.
