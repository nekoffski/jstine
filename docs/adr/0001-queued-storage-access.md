# Queued Storage Access

## Status

Accepted

## Context

Network request handling runs on Asio threads, while the in-memory database must
have one execution context. API-visible storage operations still require a
response before their client response can be produced.

## Decision

`StorageEngine` owns `Database` and a single `StorageExecutor`. Asio code sends
typed storage commands through `StorageCommandQueue`; `AsioStorageProxy` awaits
their completion on the originating Asio executor. Every proxy operation returns
`Result<T>`, including queue rejection, which uses `storageUnavailable`.

## Consequences

The database is not called directly by network threads. Queue closure is
observable as a storage availability error. Command input is currently borrowed
from the awaiting API request; command-owned input buffers remain a separate
follow-up before adding independent producers or cancellation.
