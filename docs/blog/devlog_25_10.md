# New formats and fixes

The next version of vengi will again support a few new formats - the goxel `txt` format and the veloren server chunk `dat` files.

`vengi-voxconvert --input veloren_server/directory/of/chunk_X_X_files --output veloren_chunks.vengi -f`

This will convert all the `*.dat` files in the chunk directory into a `vengi` file (or whatever format you want).

The goxel `txt` format is a text based format that can be used to store voxel data. It is not very efficient but human readable. It is supported by the goxel editor - I've basically added support for it because I read a post on discord about a guys that used AI to generate voxel art via python. They prompted the AI to generate python code to generate voxel data. The output of those scripts was then saved as `txt` files. The resulting voxel models were not really impressive - but the approach to use a text based format (another one, we already also support e.g. `qef` from qubicle, or `csv`) in this context makes a lot of sense. So here we are with another easy to parse and to produce text based format.

# Shadows

Every now and then I come back to render related stuff and each time I learn again that I don't know much about rendering. The improvements that I've mentioned in the last blog post are still full of peter panning and other artifacts. I've spent quite some time to fix those - but unfortunately without success. I guess I need to read a few more articles about shadow mapping and shadow acne and peter panning before I should give this another try.

# Multiuser mode

Some more indirect work went into the reduction of the network traffic for the multiuser mode. I've reduced the amount of bytes that is transmitted over the wire and as a side effect also reduced the memory consumption of the memento states, as they are directly tied to each other.

# Optimizations

The sirpinski triangle algoritgm that is used to split large triangles into smaller triangles for voxelization was quite slow - but not the algorithm itself, but the way how I used it - the splits changed the size of the collection that was used to store the triangles and a lot of reallocations made the otherwise fast algorithm quite slow.

# voxconvert and the web

`vengi-voxconvert` is now also supported to be executed from the web via emscripten. Previously no console tools could get executed with the shell code I've used. This still needs to be improved in terms of parameters that can get passed to the tools - but the basic setup is there.
