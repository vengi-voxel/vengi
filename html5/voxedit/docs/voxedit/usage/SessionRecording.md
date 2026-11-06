# Session recording

Use **File > Record** to save and replay changes to a scene in a `.vrec` file.
The recording contains the starting scene and subsequent voxel, node, palette,
property, keyframe and animation changes. Recording also works during a
[network session](Network.md).

Session recording stores editing changes. To capture rendered images as AVI,
MPEG2 or GIF, use [viewport video recording](Viewport.md#screenshots-and-video).
Camera navigation and the original timing of mouse movements are not a video
or part of the editing replay.

## Record an editing session

1. Open or create the scene you want to work on.
2. Choose **File > Record > Start recording** and select a `.vrec` output file.
3. Edit the scene normally. The recording starts with a complete scene snapshot,
   so playback does not require loading the original model separately.
4. Choose **File > Record > Stop recording** to finish and flush the file.

Save your model normally as well if you want a regular editable model file.
Recording and playback cannot run at the same time.

## Play a recording

Choose **File > Record > Playback** and open a `.vrec` file. Playback loads the
recording's starting scene into the editor, replacing the current scene, then
applies its recorded changes. Save work you want to keep before starting it.

The **File > Record** menu provides **Pause**, **Resume** and **Stop playback**.
Stopping leaves the scene at its current replayed state. Playback also pauses
when you modify the scene yourself.

**Speed (s)** is the delay between recorded change messages, from 0 to 2 seconds.
Smaller values play faster. A value of 0 processes at most one message per frame;
the default is 0.1 seconds. This setting does not reproduce the time spent on
the original editing gestures.

Recordings require a matching recording protocol version. Playback reports an
error if the file was written with an incompatible version.

## Console commands

These commands are available in the [console](Console.md):

| Command | Action |
| --- | --- |
| `record_start "session.vrec"` | Start recording; omit the filename to open the save dialog. |
| `record_stop` | Finish recording. |
| `record_playback "session.vrec"` | Load and play a recording; omit the filename to open the file dialog. |
| `record_playback_pause` | Pause playback. |
| `record_playback_resume` | Resume playback. |
| `record_playback_stop` | Stop playback at the current scene state. |
