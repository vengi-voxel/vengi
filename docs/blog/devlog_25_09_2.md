# Multiuser mode

I've started to implement a client/server mode for `vengi-voxedit` so people can collaborate while working on scenes. This feature was requested by a Ace of Spades community member. Initial state transfer might take some time - but after that the traffic is much lower.

![image voxedit client panel](https://raw.githubusercontent.com/wiki/vengi-voxel/vengi/images/voxedit-client_2025-09-11.png)
![image voxedit server panel](https://raw.githubusercontent.com/wiki/vengi-voxel/vengi/images/voxedit-server_2025-09-11.png)

One the road to support this I also had to tackle a long standing issue. The maximum region size of a volume for memento (undo/redo) state tracking. Why was that important for the network mode? Because I've decided to attach the network mode to the memento state handling - as every modification in the scene finds its route into the memento state handling. So I've wrapped up a listener interface and was surprised how good that worked out. After just a few days I had a working client/server mode. But there was a problem... the undo/redo also recorded a full volume state. Due to this memory wasting - but at the time of writing the memento state handling the easiest - way, it wasn't feasable to transmit the full Ace of Spades map just because you placed a single voxel. It took a few attempts to improve this, but I am quite happy with the outcome and even the non-network users will benefit from the less memory hungry memento states.

[youtube video](https://www.youtube.com/watch?v=LliCwLFiBEI)

# Let there be shadow

It's always a hop on hop off tour to get the feature requests right. There were users that wanted to see the real palette colors in a scene - no shading - nothing. I can totally understand that and it makes a lot of sense in a quite a lot of scenarios. But on the other hand there were also users that requested to get the shading back. In order to make this easier, `vengi-voxedit` got a new panel on the upper right side: The scene settings. You can use this panel to control the sun values like the azimuth and the elevation, or just pick between a few presets to put your scene into the right light (resp. shadow).

While implementing this and playing around with the shadows I've noticed a few issues with the shadows like artifacts and Peter panning. I've improved the bias calculation and made it dynamic as well as a few other things that drastically improved the shadows.

After doing this the shader got bigger and bigger - and .... there is a max string length in MSVC that this shader exceeded. It went unnoticed until I've - by coincidence - booted Windows for anther fix. `vengi-voxedit` wasn't even starting anymore and the shader error log looked quite weird. I was remembered that MSVC had a max string length of around 16000 characters - and because I've hit that limit already once in the past I did a smart splitting of those shader strings into smaller parts to please MSVC again. Turns out... I wasn't that smart. I've removed a newline too much - and everything exploded because `if (foo) // comment #if SOMEDEFINE /* comment */ #else someCode()` just doesn't make much sense in the same line. Anyway - glslangValidator didn't caught this because the final-shader-to-file wasn't hit by that bug, just the final-shader-to-string code path was. I've now ensured that every other platform is using the splitting code, too - even though this is afaik only needed on MSVC (better safe than sorry).

![image voxedit shadows](https://raw.githubusercontent.com/wiki/vengi-voxel/vengi/images/voxedit-shadow1_2025-09-15.png)
![image voxedit shadows](https://raw.githubusercontent.com/wiki/vengi-voxel/vengi/images/voxedit-shadow2_2025-09-15.png)

# Formats, formats, formats

There were a few memory leaks that are fixed now - and FBX got texture coordinate support on loading and other bug fixes. The vengi internal format got a version bump because I needed the UUIDs of the nodes inside the format (I use the `VengiFormat` to serialize the scene graph state for the network protocol initial message).

# Thumbnailer Windows

I was also working on the thumbnailer implementation for the Windows explorer - and finally found out why the DLL entrypoints are not called (see the ticket for further details) - but the DLL support needs a rewrite regarding window creation - as this is apparently not supported from DLLs for security reasons.
