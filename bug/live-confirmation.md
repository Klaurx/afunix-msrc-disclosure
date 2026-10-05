# Live Confirmation

Static analysis says the race exists. This file is the live proof it fires.

## Setup

Kernel debug session, WinDbg on host attached to Windows 11 x64 guest
over kdnet. afunix.pdb loaded for symbol resolution.

## Breakpoint at the unlocked write

Resolved function base:

```
1: kd> ? afunix!AfUnixDeliverDataToClient
Evaluate expression: -8768114283992 = fffff806`83a9aa28
```

Breakpoint set at +0x1d0:

```
bp fffff806`83a9aa28+0x1d0
g
```

Ran repro6.exe on the guest. Breakpoint hit during overlapped send
traffic to a non-draining receiver:

```
Breakpoint 0 hit
afunix!AfUnixDeliverDataToClient+0x1d0:
fffff806`83a9abf8 834f3001        or      dword ptr [rdi+30h],1
```

## Lock state measured at the instruction

At the exact moment the breakpoint fired, checked the per-endpoint
pushlock state:

```
2: kd> r rdi
rdi=ffff87855bb456f8

2: kd> dq @rdi-0x190+8 L1
ffff8785`5bb45570  fffff807`98e9e3f0

2: kd> ? (fffff807`98e9e3f0 & 1)
Evaluate expression: 0 = 00000000`00000000
```

Lock exclusive bit is 0. Not held. Measured at the exact instruction,
not inferred, not reconstructed. The write happens wit no lock.

## Cross-core race confirmed via hardware watchpoint

Set a hardware watchpoint on the flags field wit an inline logger:

```
ba w4 <endpoint_base+0x1c0> "r @rip; dd <endpoint_base+0x1c0> L1; g"
```

Ran repro6.exe, two threads, one flooding overlapped WSASend, one
spamming CancelIoEx. Watchpoint output:

```
afunix!AfUnixDeliverDataToClient+0x1d4    (CPU 1)
afunix!AfUnixDeliverDataToClient+0x359    (CPU 1)
afunix!AfUnixDeliverDataToClient+0x80     (CPU 2)
afunix!AfUnixDeliverDataToClient+0x359    (CPU 2)
afunix!AfUnixDeliverDataToClient+0x80     (CPU 0)
```

The unlocked write at +0x1d4 firing interleaved wit the locked entry
(+0x80) and locked exit (+0x359) writes, across three different cores,
on the same endpoint, same run. Not a narrow theoretical window. This
is actually happening on real hardware.

## Flag value logged across every hit

```
00000003    (+0x1d4, unlocked OR fired, bit 0 set)
00000001    (+0x359, locked clear of bit 1, bit 0 stays)
00000003    (+0x080, next call entry, bit 0 already set)
00000003
00000001
00000003
```

Expected entry value is 0x00000000. After the first unlocked OR fires,
every subsequent call enters wit bit 0 already set. It never went back
to 0x00000000 in any invocation across the entire run.

## Full callstack at the vulnerable instruction

Captured wit kb at the +0x1d0 breakpoint:

```
 # RetAddr               : Call Site
00 fffff806`83a9b048     : afunix!AfUnixDeliverDataToClient+0x1d0
01 fffff806`83a916a8     : afunix!AfUnixDeliverEndpointSend+0x1e4
02 fffff806`83abe008     : afunix!AfUnixTlConnectEndpointSend+0x118
03 fffff806`83ab7c6b     : afd!AfdTLStartBufferedVcSend+0xd8
04 fffff806`83ab6cac     : afd!AfdSend+0xe1b
05 fffff806`edd597bb     : afd!AfdDispatchDeviceControl+0x6c
06 fffff806`edd59559     : nt!IofCallDriver+0x4b
07 fffff806`ee3f2fe9     : nt!IopCallDriverReference+0xe9
08 fffff806`ee373a68     : nt!IopSynchronousServiceTail+0x2bd
09 fffff806`ee372eae     : nt!IopXxxControlFile+0xba8
0a fffff806`ee149c15     : nt!NtDeviceIoControlFile+0x5e
0b 00007ffc`88d62144     : nt!KiSystemServiceCopyEnd+0x25
```

Normal unprivileged WSASend call crossing the syscall boundary at
KiSystemServiceCopyEnd, going through NtDeviceIoControlFile into
afd.sys, reaching afunix.sys via the WSK provider dispatch table,
landing at the unlocked OR wit the per-endpoint pushlock confirmed
not held.

## What was not demonstrated

The driver has what looks like a clearing path for bit 0 through
AfUnixEndpointIoctl case 4 into AfUnixDeliverCancelPendingIo, gated
on endpoint type 2 and no prior shutdown. Every user-mode candidate
I tried to reach it, CancelIoEx, WSAIoctl SIO_FLUSH, shutdown
SD_RECEIVE, non-blocking sends, either got absorbed at the AFD layer
or went through the WSK disconnect path instead. Every live hit on
AfUnixEndpointIoctl during testing came from AFD's own bind bookkeeping,
not from anything I called.

So I am not saying the flag can never clear. I am saying it did not
clear in any run I did and I could not find the path that would clear
it despite trying the obvious candidates. That is me not finding the
path, not me proving there is no path.

Same goes for ceiling. The corrupted field is a flag word, not a
pointer. What breaks is which branch gets taken, not where execution
jumps. DoS is what was demonstrated. Anything beyond that was not
tested and not claimed.