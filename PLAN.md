# Air-track project: first plan

24 November 2026

## Goal

The project is finished when a tracker on one machine holds a live, smoothed picture of every aircraft in range, fed over the network by a publisher on another, and that picture stays complete with 5% of packets deliberately dropped.

Version 1 delivers four things:

- A publisher that turns aircraft messages into a sequenced multicast feed, with a recovery service.
- A tracker that receives the feed, repairs gaps, and keeps one filtered track per aircraft.
- A query interface, plus one consumer that uses it: conflict detection between pairs of aircraft.
- A write-up with measured throughput, latency and recovery numbers.

Out of scope for version 1: locating aircraft from several receivers, a standby tracker, and any interface beyond a basic map.

## Architecture

Data flows one way: radio receiver, publisher, network, tracker, consumers. You write three programs (publisher, tracker, conflict detector) and one tool (a replayer for recorded data). The radio decoding is done by `dump1090`, an existing open-source program.

Each link between them uses a different protocol, chosen for what that link has to do.

| Link | Carries | Protocol | Why this one |
| --- | --- | --- | --- |
| Receiver to publisher | Decoded aircraft messages | TCP on the same machine | `dump1090` already serves them this way |
| Live feed | Every update, with a sequence number | UDP multicast | One send reaches all listeners, the publisher keeps no per-listener state, and a slow listener cannot delay the others |
| Recovery | Missed packets, and a full snapshot for late joiners | TCP | Rare, must arrive complete and in order, speed matters less |
| Queries and control | Questions about tracks, subscriptions, health | gRPC | Low rate and structured, so a defined schema and generated code beat hand-packed bytes |
| Inside the tracker | Packets from the network thread to the tracking thread | Lock-free ring buffer | The network thread must never wait, or the kernel drops packets |

The split between multicast and TCP is the central design idea. UDP can lose packets silently, so every packet carries a sequence number, and a receiver that sees a jump asks the recovery service for what it missed.

## gRPC and the case against RPC

gRPC is RPC. The criticism you have heard is aimed at RPC systems that hide the network, not at remote calls as such.

**The criticism.** Early RPC systems tried to make a call to another machine look identical to a local function call. That fails because a remote call differs in ways code cannot ignore:

- It can be thousands of times slower, and the delay varies.
- It can fail halfway. If no reply comes, you cannot tell whether the other side never got the request, or ran it and the reply was lost.
- The two sides share no memory, so everything must be copied and encoded.
- The other machine can crash while yours keeps running.

The classic statement of this is a 1994 Sun paper, *A Note on Distributed Computing*. Its point is that code must know which calls are remote.

**What gRPC does differently.** It keeps the convenience and drops the pretence:

- Every call returns a status you have to handle, including "deadline exceeded" and "unavailable".
- Every call can carry a deadline and can be cancelled.
- A call can be a stream of messages in either direction, which a function call cannot express.

**What it is, concretely.** You describe your messages and operations in a `.proto` file. A compiler generates C++ classes for both client and server. Messages travel as Protocol Buffers, a compact binary encoding, over HTTP/2. For this project the file would hold something like:

```proto
service AirPicture {
  rpc GetTracks(Area) returns (TrackList);
  rpc SubscribeTracks(Area) returns (stream Track);
}
```

The first is a single request and reply. The second is a server stream: one request, then updates until the client disconnects.

**Why not use it for the live feed.** Each client gets its own TCP connection, so the server tracks every client and a slow one creates back-pressure. Multicast avoids both.

**What still applies.** You still design for failure: set a deadline on every call, and make any call you might retry safe to run twice.

## Milestones

Build in this order. Each stage runs on its own and has a check that tells you it is done. Stages 1 to 3 are the networking core, and stage 5 can be prototyped in Python at any point.

| # | Stage | What you build | Main thing it teaches | Done when |
| --- | --- | --- | --- | --- |
| 0 | Setup | CMake project, tests, two processes exchanging one UDP packet | Toolchain, sockets basics | A clean checkout builds and tests with one command |
| 1 | Decode and replay | Parser from `dump1090` text lines into a struct, and a replayer that serves a recorded capture at 1x or 100x | Parsing, file and socket I/O | Parser tests pass and the replayer feeds a client at both speeds |
| 2 | Multicast feed | A binary message format with a header (sequence number, timestamp), a publisher, a bare receiver | Wire formats, multicast, byte layout | A receiver in another network namespace counts every packet and reports any sequence gap |
| 3 | Recovery | A history buffer and TCP resend service in the publisher, gap repair and startup snapshot in the receiver | TCP, `epoll`, state machines | With 5% injected loss, the receiver's output matches the publisher's input exactly |
| 4 | Tracker threads | A network thread, a single-producer single-consumer ring buffer, a tracking thread with a table of aircraft | Concurrency, atomics, cache effects | Ring buffer stress test is clean under ThreadSanitizer, and 100x replay drops nothing |
| 5 | Kalman filter | One-dimensional filter, then the four-state version, coordinate conversion, offline tuning | Estimation, numerical code | Error on a synthetic track is below the raw noise, and the consistency statistic averages near 2 on real data |
| 6 | gRPC interface | The `.proto` file, `GetTracks`, `SubscribeTracks` | Schemas, generated code, streaming calls | A short Python script lists aircraft within a radius of a point |
| 7 | Conflict detector | A consumer that extrapolates pairs of tracks and finds their closest approach, plus a basic map | Using the system as a client | Alerts appear for pairs that pass under a chosen separation |
| 8 | Measure and write up | Benchmarks, plots, README | Performance analysis | Every number in the Measurement section is filled in |

Stages 0 to 3 are the minimum that makes a worthwhile project on their own. If time runs short, stop there and write it up.

## Setup

Everything except the optional radio dongle is free.

| Area | Choice | Note |
| --- | --- | --- |
| Data | A recorded capture first, live radio later | A dongle with a 1090 MHz antenna costs roughly £25 to £40 (approximate, not checked) |
| Machine | The Arch laptop for all multicast work | WSL2 handles multicast badly |
| Second node | Linux network namespaces first, a real second machine later | Namespaces let one laptop act as several hosts |
| Language | C++20 with GCC or Clang, built with CMake | |
| Networking | Plain POSIX sockets and `epoll`, no library | Writing this layer is the point |
| Libraries | Eigen for matrices, gRPC and Protobuf, GoogleTest or Catch2 | |
| Fault injection | `tc netem` | Adds loss, delay and reordering to an interface |
| Debugging | ThreadSanitizer, AddressSanitizer, `tcpdump` or Wireshark, `perf` | |
| Python | Filter tuning, plots, a test client | Keeps the C++ focused on networking and threads |

## Measurement

These numbers are what turn the project from "it works" into evidence. Decide on them now so each stage is built to be measurable.

| What | How to measure | Stage |
| --- | --- | --- |
| Throughput | Highest replay speed the publisher and tracker sustain with no drops, in messages per second | 2, 4 |
| End-to-end latency | Time from publisher send to track updated, as median, 99th and 99.9th percentile | 4 |
| Gap recovery time | Time from detecting a gap to having it filled, at 1%, 5% and 10% loss | 3 |
| Late-join time | Time for a newly started tracker to match a running one | 3 |
| Ring buffer speed | Operations per second against a mutex-protected queue | 4 |
| Filter accuracy | Position error against the raw data on a synthetic track with known truth | 5 |
| Filter consistency | Average of the consistency statistic on real data, which should be near 2 | 5 |

Measure latency with both programs on one machine first. Across two machines the clocks disagree, so the figure includes clock error unless you synchronise them.

## Open decisions

Each has a suggested default so none of them blocks starting.

- [ ] **Buy a dongle now, or start from recorded data?** Default: recorded data, so stages 0 to 3 need no hardware. Where to get a good capture is still to be found.
- [ ] **Which `dump1090` output to read?** It offers a text format and a binary one. Default: text, because it is easy to inspect.
- [ ] **Fixed-size or variable-size messages on the feed?** Default: fixed-size structs, one per message type.
- [ ] **Track in two dimensions or three?** Default: two, with altitude carried as a plain field and not filtered.
- [ ] **How does a late joiner get the snapshot?** Default: over the same TCP recovery connection.
- [ ] **How many hours a week can this take alongside the degree?** This sets whether version 1 stops at stage 3 or stage 7.

## After version 1

In rough order of value:

1. **Multilateration.** Locate aircraft that do not broadcast a position, from arrival-time differences at three or more receivers. This needs clock synchronisation between machines and makes the system properly distributed.
2. **Standby tracker.** A second tracker takes over when the first dies, which brings in failover and replicated state.
3. **Multiple-model filter.** Run a cruising model and a manoeuvring model side by side so tracks stop lagging in turns.
4. **Ship tracking.** Add vessel broadcasts as a second feed type and fuse both into one picture.
5. **Faster I/O.** Compare `epoll` against `io_uring`, and pin threads to cores, with before and after latency figures.
