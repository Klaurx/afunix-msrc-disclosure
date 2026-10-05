# Static Analysis

Full disassembly of AfUnixDeliverDataToClient walked end to end.
Not sampled, not cherry picked, every call to every relevant primitive
was catalogued before drawing any conclusions.

## The unlocked window

Lock release:

```
0x14000aaef  CALL [ExReleasePushLockExclusiveEx]
0x14000aafe  CALL [KeLeaveCriticalRegion]
```

Lock reacquire:

```
0x14000accb  CALL [KeEnterCriticalRegion]
0x14000acdc  CALL [ExAcquirePushLockExclusiveEx]
```

Unlocked window spans 0x14000ab05 through 0x14000accb. Every call to
ExAcquirePushLockExclusiveEx in this function was checked. There is
exactly one, at function entry, and it targets the table lock, not the
per-endpoint lock at param_1+8. The per-endpoint lock is never acquired
anywhere inside this function after the release at 0x14000aaef.

## What gets cleared before the lock drops

Right before releasing the lock, the function zeroes the list head fields:

```
0x14000aab0  MOV qword ptr [RDI],   R8    (param_1+0x190 = 0)
0x14000aab3  MOV qword ptr [RDI+8], R8    (param_1+0x198 = 0)
```

This detaches the pending data list so no other thread can reach those
nodes through the endpoint. It does nothing for param_1+0x1c0, the
delivery flags word. That field stays live in shared memory, reachable
by any thread holding a reference to the endpoint.

## The two bad writes

Both inside the unlocked window, no LOCK prefix on either:

```
+0x1d0 (0x14000abf8):  OR dword ptr [RDI+0x30], 1
+0x22b (0x14000ac53):  OR dword ptr [RDI+0x30], 1
```

RDI = param_1+0x190, so RDI+0x30 = param_1+0x1c0.

First one hits when AFD returns STATUS_DEVICE_NOT_READY, meaning no
pending receive IRPs on the other end. Second one hits when AFD accepts
only a partial amount of the offered data.

## The only other writer of this field

AfUnixDeliverCancelPendingIo (0x1400016f0) is the only other function
in the binary that touches param_1+0x1c0. It does it right:

```c
*(uint*)(param_1 + 0x1c0) |= 2;
do {
    ExReleasePushLockExclusiveEx(param_1 + 8, 0);
    KeLeaveCriticalRegion();
    AfUnixDeliverCompleteReceiveRequests(queue, status);
    KeEnterCriticalRegion();
    ExAcquirePushLockExclusiveEx(param_1 + 8);
    queue = *(param_1 + 400);
} while (queue != 0);
*(uint*)(param_1 + 0x1c0) &= ~2;
```

Both the set and the clear happen under the lock. The bug is that simple.
One writer follows the discipline, the other does not, same exact field.

## Why this cannot be intentional

A plain OR on x86-64 is not atomic. It is read, modify, write as three
separate micro-ops with no guarantee of atomicity against another core.

If the intent was to partition the word so bit 0 and bit 1 could be
handled by separate code paths independently, that still does not work.
Partitioning bits inside one word still requires LOCK OR or LOCK AND on
every accessor touching that word. A plain OR gives zero safety no
matter which bits u think u own.

If this was a performance optimization, the correct primitive was already
available and cheap. LOCK OR is atomic on its own, roughly 10 cycles on
a hot cache line. Nobody optimizing on purpose picks plain OR when
LOCK OR does the same job safely for basically the same cost.

Every other accessor of this field in the binary uses the lock. This one
spot does not. That asymmetry is the tell. The lock got dropped to make
the AFD callback, the callback returned, and the write went back to
shared state without reapplying the lock discipline from the rest of the
function. An oversight, not a design choice.

## The specific lost-update scenario

This is not just a theoretical bad interleave. There is a concrete
corrupting sequence:

```
Thread A (DeliverDataToClient, no lock held):
    reads  [+0x1c0] = 0x00000002

Thread B (DeliverCancelPendingIo, holds the lock):
    reads  [+0x1c0] = 0x00000002
    clears bit 1
    writes [+0x1c0] = 0x00000000

Thread A:
    ORs bit 0 into its stale copy
    writes [+0x1c0] = 0x00000001
```

Bit 1 gets resurrected from Thread A's stale read. Bit 1 is the flag
that DeliverCancelPendingIo checks on entry to decide whether to defer
instead of actually cancelling pending receive requests. Next time
DeliverCancelPendingIo runs, it sees bit 1 set, thinks delivery is
already in progress, and skips cancelling the pending receives on that
endpoint. Those requests stay pending wit no bound in the code I traced.

## Binary identity

Binary: afunix.sys
SHA256: 217317a359147fe6b0855f4b5abde876a21ca39c0760844026b4922e049e7ceb
PDB GUID: CDF7B3740E839EBFFBF7F38A35024F391
Version: 10.0.28000.2804 (WinBuild.160101.0800)
OS build: Windows 11 Insider Preview
Function: AfUnixDeliverDataToClient (0x14000aa28)
Concurrent writer: AfUnixDeliverCancelPendingIo (0x1400016f0)
Field: endpoint+0x1c0, delivery flags, inside deliver-state struct at +0x190