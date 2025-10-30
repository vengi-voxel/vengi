/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace voxedit {

enum class ViewMode : uint8_t { Default, Simple, All, CommandAndConquer, MinecraftSkin, AceOfSpades, Max };

#define VIEWMODE_FLAG_PALFORMAT6BIT (1 << 0)
#define VIEWMODE_FLAG_ALL_VIEWPORTS (1 << 1)
#define VIEWMODE_FLAG_NORMALPALETTE (1 << 2)
#define VIEWMODE_FLAG_MEMENTOPANEL  (1 << 3)
#define VIEWMODE_FLAG_CAMERAPANEL   (1 << 4)
#define VIEWMODE_FLAG_TREEPANEL     (1 << 5)
#define VIEWMODE_FLAG_LSYSTEMPANEL  (1 << 6)
#define VIEWMODE_FLAG_SCRIPTPANEL   (1 << 7)
#define VIEWMODE_FLAG_ASSETPANEL    (1 << 8)
#define VIEWMODE_FLAG_RENDERPANEL   (1 << 9)
#define VIEWMODE_FLAG_ANIMATIONS    (1 << 10)
#define VIEWMODE_FLAG_NOSPLIT       (1 << 11)
#define VIEWMODE_FLAG_NETWORKPANEL  (1 << 12)
#define VIEWMODE_FLAG_GAMEMODEPANEL (1 << 13)

uint64_t viewModeFlags(ViewMode viewMode);

template<typename T>
inline bool viewModeAllViewports(T viewMode) {
	return (viewModeFlags((ViewMode)viewMode) & VIEWMODE_FLAG_ALL_VIEWPORTS) != 0u;
}

template<typename T>
inline bool viewModeNormalPalette(T viewMode) {
	return (viewModeFlags((ViewMode)viewMode) & VIEWMODE_FLAG_NORMALPALETTE) != 0u;
}

template<typename T>
inline bool viewModeMementoPanel(T viewMode) {
	return (viewModeFlags((ViewMode)viewMode) & VIEWMODE_FLAG_MEMENTOPANEL) != 0u;
}

template<typename T>
inline bool viewModeCameraPanel(T viewMode) {
	return (viewModeFlags((ViewMode)viewMode) & VIEWMODE_FLAG_CAMERAPANEL) != 0u;
}

template<typename T>
inline bool viewModeTreePanel(T viewMode) {
	return (viewModeFlags((ViewMode)viewMode) & VIEWMODE_FLAG_TREEPANEL) != 0u;
}

template<typename T>
inline bool viewModeLSystemPanel(T viewMode) {
	return (viewModeFlags((ViewMode)viewMode) & VIEWMODE_FLAG_LSYSTEMPANEL) != 0u;
}

template<typename T>
inline bool viewModeScriptPanel(T viewMode) {
	return (viewModeFlags((ViewMode)viewMode) & VIEWMODE_FLAG_SCRIPTPANEL) != 0u;
}

template<typename T>
inline bool viewModeNetworkPanel(T viewMode) {
	return (viewModeFlags((ViewMode)viewMode) & VIEWMODE_FLAG_NETWORKPANEL) != 0u;
}

template<typename T>
inline bool viewModeGameModePanel(T viewMode) {
	return (viewModeFlags((ViewMode)viewMode) & VIEWMODE_FLAG_GAMEMODEPANEL) != 0u;
}

template<typename T>
inline bool viewModePaletteFormat6Bit(T viewMode) {
	return (viewModeFlags((ViewMode)viewMode) & VIEWMODE_FLAG_PALFORMAT6BIT) != 0u;
}

template<typename T>
inline bool viewModeAssetPanel(T viewMode) {
	return (viewModeFlags((ViewMode)viewMode) & VIEWMODE_FLAG_ASSETPANEL) != 0u;
}

template<typename T>
inline bool viewModeRenderPanel(T viewMode) {
	return (viewModeFlags((ViewMode)viewMode) & VIEWMODE_FLAG_RENDERPANEL) != 0u;
}

template<typename T>
inline bool viewModeAnimations(T viewMode) {
	return (viewModeFlags((ViewMode)viewMode) & VIEWMODE_FLAG_ANIMATIONS) != 0u;
}

template<typename T>
inline bool viewModeNoSplit(T viewMode) {
	return (viewModeFlags((ViewMode)viewMode) & VIEWMODE_FLAG_NOSPLIT) != 0u;
}

const char *getViewModeString(ViewMode viewMode);

} // namespace voxedit
