A more detailed changelog can be found at: https://github.com/mgerhardy/engine/commits/

#### 0.0.3 (XXXX-XX-XX)

 General:
   - Print stacktraces on asserts
   - Improved tree generation (mainly used in voxedit)
   - Fixed a few asserts in debug mode for the microsoft stl

#### 0.0.2 (2020-05-06)

 VoxEdit:
   - Static linked VC++ Runtime
   - Extract voxels by color into own layers
   - Updated tree and noise windows
   - Implemented `thicken` console command
   - Escape abort modifier action
   - Added L-System panel

 General:
   - Fixed binvox header parsing
   - Improved compilation speed
   - Fixed compile errors with locally installed glm 0.9.9
   - Fixed setup-documentation errors
   - Fixed shader pipeline rebuilds if included shader files were modified
   - Improved palm tree generator
   - Optimized mesh extraction for the world (streaming volumes)
   - Added new voxel models
   - (Re-)added Tracy profiler support and removed own imgui-based implementation
   - Fixed writing of key bindings
   - Improved compile speed and further removed the STL from a lot of places
   - Updated all dependencies to their latest version

 Server/Client:
   - Added DBChunkPersister
   - Built-in HTTP server to download the chunks
   - Replaced ui for the client

 Voxel rendering
   - Implemented reflection for water surfaces
   - Apply checkerboard pattern to voxel surfaces
   - Up-scaling effect for new voxel chunks while they pop in
   - Optimized rendering by not using one giant vbo


#### 0.0.1 "Initial Release" (2020-02-08)

 VoxEdit:
   - initial release
