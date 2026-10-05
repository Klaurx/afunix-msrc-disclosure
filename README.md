# afunix-msrc-disclosure

Race condition in afunix.sys, the Windows kernel AF_UNIX transport driver.
Submitted to MSRC. Closed as "non MSRC case" without a real review.
This repo is the full story.

## What this is

A live, confirmed, unprivileged race condition in afunix.sys where
AfUnixDeliverDataToClient performs a non-atomic write to shared endpoint
state outside the pushlock that is supposed to protect it. Confirmed wit
WinDbg, hardware watchpoints, and a working PoC across 3 CPU cores.

MSRC closed it as a non security issue. This repo documents the bug in
full so it exists publicly regardless of what they decide to do wit it.

## Affected component

Binary: afunix.sys
SHA256: 217317a359147fe6b0855f4b5abde876a21ca39c0760844026b4922e049e7ceb
PDB GUID: CDF7B3740E839EBFFBF7F38A35024F391
Version: 10.0.28000.2804
Function: AfUnixDeliverDataToClient (0x14000aa28)
Concurrent writer: AfUnixDeliverCancelPendingIo (0x1400016f0)
Field: endpoint+0x1c0, delivery flags inside deliver-state struct at +0x190

## Repo structure

- README.md - this file
- poc/probe.c - minimal setup tool used during investigation
- poc/repro6.c - main PoC, triggers the race across multiple cores
- bug/static-analysis.md - full disassembly evidence and locking audit
- bug/live-confirmation.md - WinDbg output, watchpoint logs, callstack
- msrc/timeline.md - full MSRC submission and response documentation

## Quick summary of what was found

Any unprivileged process can open an AF_UNIX socket and drive
AfUnixDeliverDataToClient into a code path where it writes back to
shared endpoint state wit a plain OR instead of a locked write. Every
other accessor of this field in the driver holds the pushlock. This one
spot does not.

Confirmed live:
- Breakpoint at the unlocked OR, lock state measured at the instruction
- Hardware watchpoint showing the write firing from 3 different CPU cores
- Full callstack from unprivileged WSASend all the way down to the bad instruction
- Flags word observed latching and never recovering across the entire test run

## MSRC outcome

Submitted with full static analysis, working PoC, live debugger output,
callstack, and steps to reproduce. Closed as "non MSRC case" with a
boilerplate response asking for a proof of concept that was already in
the report.

This is consistent wit a well documented pattern of MSRC closing
technically valid reports without reading them. Resources on that are
in msrc/timeline.md.

## Disclosure

Bug reported to MSRC before this repo was made public.
PoC is included here because MSRC closed the case.
No exploitation beyond demonstrating the race condition was attempted.
The ceiling of this bug is unknown. DoS is what was demonstrated.
Anything beyond that was not tested.