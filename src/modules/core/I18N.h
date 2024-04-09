/**
 * @file
 */

#pragma once

#ifdef IMGUI_ENABLE_TEST_ENGINE
#define _(x) x
#else
// TODO: translate - see https://github.com/vengi-voxel/vengi/issues/430
#define _(x) x
#endif
