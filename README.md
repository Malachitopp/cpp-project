# cpp-project

A real-time air-track picture built from live aircraft broadcasts (ADS-B), written in C++ on Linux.

A publisher turns aircraft messages into a sequenced UDP multicast feed with a TCP recovery service. A tracker receives the feed, repairs gaps, and keeps one Kalman-filtered track per aircraft. Consumers query the tracker over gRPC.

Status: planning. See [PLAN.md](PLAN.md).
