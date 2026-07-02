# Contributing

Contributions must preserve the project scope:

- C99 library core
- caller-owned objects and buffers
- zero heap allocation
- no hidden mutable global state
- no framework, parser, database, or runtime expansion

Rules for changes:

- Do not weaken validation or fault tests.
- Keep public contracts explicit and documented.
- Treat CRC as corruption detection only, never authentication.
- Document ownership, callback recursion limits, and verification gaps.
- Do not tag or publish releases except through an explicit requested release flow.
