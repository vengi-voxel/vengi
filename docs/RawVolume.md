# RawVolume storage

`RawVolume` is a bounded voxel volume. Regions below 64 MiB of uncompressed
voxel data use a contiguous array. Larger regions use an internal fixed directory
of lazily allocated chunks, with up to 32 voxels along each axis. The threshold
is an initial conservative choice and should be tuned using voxel benchmarks.

An untouched chunk contains copies of a default voxel without allocating its
payload. `fill()` also sets this default, so a uniformly filled volume need not
allocate every chunk. Writes preserve the same voxel assignment behavior as the
contiguous implementation, including normal preservation. Copy construction copies
only allocated chunks. Coordinates are relative to the volume's lower corner,
which allows negative origins and translation without moving stored data.

Reading a voxel does not allocate a chunk, acquire a volume mutex, or update a
shared cache. The fixed directory publishes new chunks atomically. Writes to
different voxels can create chunks concurrently, but access to the same voxel
must be synchronized by the caller. Bulk mutation, copying during mutation,
storage materialization and destruction require exclusive access. Chunks are
retained until destruction or materialization; there is no disk paging or LRU
eviction. `clear()` resets contents without invalidating allocated chunk addresses.

Use `voxel()`, `setVoxel()` and samplers for storage-independent access. A reference
returned for an unallocated cell refers to the default voxel; reacquire it after
writing to that cell. Samplers obtain current values without materializing storage.
For allocated chunks they cache the current pointer, strides and contiguous bounds;
neighbor reads crossing those bounds use the facade. Missing chunks remain live
lookups, so a sampler observes a subsequently allocated cell without being recreated.

Texture extraction, bounds computation, slice reads and memento serialization use
storage-independent reads. Binary greedy extraction uses a bounded, dense padded
copy of the extraction chunk rather than materializing the source volume.

The compatibility methods `voxels()` and `data()` require a contiguous array.
On a chunked volume they materialize all voxels and permanently switch that
instance to contiguous storage. This requires exclusive access, allocates the full
uncompressed volume, and invalidates existing voxel references and samplers.
Check for allocation failure before using the returned pointer. `copyVoxels()`
instead returns an independent contiguous copy without changing source storage.
`move()` currently also materializes storage before rotating voxels.

`allocatedBytes()` reports allocated voxel payload plus directory bytes, excluding
allocator and object overhead. It is not a process resident-memory measurement.

The read benchmarks compare default storage (`.../side/content/0`) with explicitly
materialized storage (`.../side/content/1`). Content 0 is empty, 1 is a shell,
and 2 is uniformly filled. The `allocated_bytes` counter reports payload and
directory allocation for each case. Run matching configurations on the same
machine when comparing results.
