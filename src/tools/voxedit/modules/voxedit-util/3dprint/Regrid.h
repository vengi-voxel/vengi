/**
 * @file
 */

#pragma once

namespace voxedit {
class SceneManager;

namespace printing {

// Rebucket every model node into axis-aligned world-space cells of size cellSize.
// New nodes are parented to the root (flat layout). Source nodes are deleted as
// they are fully consumed so peak memory does not double.
void runRegrid(SceneManager *sceneMgr, int cellSize);

} // namespace printing
} // namespace voxedit
