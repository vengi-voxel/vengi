# New formats and fixes

The next version of vengi will again support a few new formats - the Goxel `txt` format and the Veloren server chunk `*.dat` files.

`vengi-voxconvert --input veloren_server/directory/of/chunk_X_X_files --output veloren_chunks.vengi -f`

This will convert all the `*.dat` files in the chunk directory into a `vengi` file (or any other supported format).

The Goxel `txt` format is a text-based format that can be used to store voxel data. It's not very efficient, but it's human-readable and supported by the Goxel editor. I added support after reading a post on Discord from someone who used AI to generate voxel art via Python. They prompted the AI to produce Python code which generated voxel data, and the output of those scripts was saved as `txt` files. The resulting voxel models weren't particularly impressive, but the idea of a simple, text-based interchange format (we already support other text-like formats such as `qef` from Qubicle or `csv`) makes a lot of sense. So here we are with another easy-to-parse and easy-to-produce text-based format.

# Shadows

Every now and then I come back to rendering-related work and I always relearn how much I still have to learn about rendering. The improvements I mentioned in the last blog post still suffer from peter‑panning and other artifacts. I've spent quite some time trying to fix those, but unfortunately without success. I need to read a few more articles about shadow mapping, shadow acne, and peter‑panning before I give this another try.

# Multiuser mode

Some indirect work went into reducing network traffic for multiuser mode. I've reduced the number of bytes transmitted over the wire and, as a side effect, reduced the memory consumption of the memento states (they're tightly tied to each other).

# Optimizations

The Sierpinski triangle algorithm used to split large triangles into smaller ones for voxelization was quite slow - not because of the algorithm itself, but due to how I used it. The splits changed the size of the collection that stores the triangles, causing many reallocations and making an otherwise fast algorithm slow.

# voxconvert and the web

`vengi-voxconvert` is now also supported to be executed from the web via Emscripten. Previously, no console tools could be executed using the shell code I had, so this is a step forward. It still needs improvements for passing parameters to the tools, but the basic setup is in place.
