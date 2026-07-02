# Security Policy

`microconf` is a data-integrity library, not a security boundary.

- CRC32 is for accidental corruption detection only.
- Persistence records are not authenticated or encrypted.
- Shared contexts, data buffers, and storage backends must be externally serialized.
- Callback implementations must not recursively call back into the same `mconf_t`
  during save/load operations.

Report issues through the repository issue tracker until a dedicated security
contact is established.
