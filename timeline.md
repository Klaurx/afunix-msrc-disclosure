# MSRC Submission Timeline

Full record of the submission and response. Documented publicly because
MSRC closed a technically complete report without reading it, which is
consistent wit a well documented pattern of how they treat researchers
they do not recognize.

## What was submitted

Report filed through the MSRC researcher portal on October 5, 2026.

Included in the original submission:
- Full vulnerability description wit static analysis, locking audit,
  lost-update scenario, and reasoning for why this cannot be intentional
- Steps to reproduce wit exact WinDbg commands and breakpoint offsets
- Working PoC source code (repro6.c)
- Live callstack captured at the vulnerable instruction
- Hardware watchpoint output showing the race firing across 3 CPU cores
- Lock state measured at the exact moment of the write
- Binary SHA256, PDB GUID, function offsets, affected field offsets
- Honest disclosure of what was and was not demonstrated

## MSRC response

First response, boilerplate asking for a proof of concept:

> We require additional information, including a proof of concept,
> to continue our assessment. Because we have not received the required
> information, we are closing this report.

The PoC was in the original submission. The callstack was in the original
submission. The live debugger output was in the original submission.
This response was sent without reading what was there.

Second response after pushback, case status in the portal:

> Closed as non MSRC case

No elaboration. No technical justification. No indication anyone read
the static analysis or the live debugger output.

## What "non MSRC case" means in practice

It means they decided the issue is not a security vulnerability at all,
not even worth evaluating for severity. A classification that gets
applied automatically when triage does not recognize the submitter or
the report does not match a pattern the flowchart expects.

This is not unique to this report. The same classification gets applied
routinely to reports from researchers without established MSRC reputation,
regardless of technical quality.

## The pattern this fits into

This is not a one off. It is a documented systemic issue:

- Tenable, a professional security company, sent four follow up requests
  over six weeks wit zero response. MSRC patched their bug, said it was
  not security related, then reversed that after Tenable escalated to
  Twitter and a community manager directly.

- Nightmare Eclipse had multiple reports ignored or silently patched wit
  no credit or bounty across an extended period. MSRC eventually deleted
  their account and threatened legal action after they went public.
  Microsoft later issued a public statement acknowledging that some past
  interactions had fallen short.

- Multiple researchers have documented submitting reports that were closed
  as not meeting the bar for servicing, only to see the exact issue
  patched in a subsequent Windows update wit no credit assigned.

- Katie Moussouris, the person who built Microsoft's bug bounty program,
  publicly stated the bugs are Microsoft's, describing how MSRC treats
  researcher findings as their own intellectual property.

- Former MSRC employees have stated publicly that qualified specialists
  were replaced wit flowchart followers, and that the S in MSRC stopped
  standing for security.

The two-tier system is real. Researchers wit established MSRC reputation
or institutional affiliation get human review. Unknown handles get
auto-triage and a boilerplate close. The same report submitted by a
known researcher wit prior accepted CVEs routes differently than one
submitted by someone outside the system.

## Why this repo exists

The bug is real. The live evidence is real. The PoC works.

MSRC closing this as a non security issue does not change any of that.
This repo exists so the finding is documented publicly and permanently,
independent of what MSRC decides to do wit it.

If Microsoft silently fixes this in a future patch Tuesday, that fix
will exist alongside this documentation. The work was done. The evidence
is here. Draw your own conclusions.

## Resources on MSRC researcher treatment

- Tenable TRA-2022-20, documented six weeks of silence followed by
  silent patch and denial of security relevance
- Nightmare Eclipse public disclosures, March-July 2026, and the
  subsequent MSRC backlash documented across security media
- Microsoft public statement May 2026 acknowledging interactions had
  fallen short
- Katie Moussouris public comments on researcher treatment
- r/netsec threads on MSRC treatment patterns