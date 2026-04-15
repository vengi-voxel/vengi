/**
 * @file
 */

#pragma once

#include "brush/LUABrush.h"
#include "brush/LUASelectionMode.h"
#include "brush/SelectBrush.h"
#include "core/IComponent.h"
#include "core/collection/DynamicArray.h"

namespace voxedit {

/**
 * @brief Manages discovery, loading, and lifecycle of Lua brush and selection mode scripts
 */
class ScriptManager : public core::IComponent {
private:
	core::DynamicArray<LUABrush *> _luaBrushes;
	int _activeLuaBrushIndex = -1;

	core::DynamicArray<LUASelectionMode *> _luaSelectionModes;

	/**
	 * @brief SelectBrush needs to be notified when selection mode scripts are reloaded
	 * to avoid dangling pointers
	 */
	SelectBrush *_selectBrush = nullptr;

	void discoverBrushScripts();
	void clearBrushScripts();
	void discoverSelectionModeScripts();
	void clearSelectionModeScripts();

public:
	void construct() override;
	bool init() override;
	void shutdown() override;

	void setSelectBrush(SelectBrush *selectBrush);

	const core::DynamicArray<LUABrush *> &luaBrushes() const;
	int activeLuaBrushIndex() const;
	void setActiveLuaBrushIndex(int index);
	LUABrush *activeLuaBrush();
	const LUABrush *activeLuaBrush() const;
	void reloadBrushScripts();

	const core::DynamicArray<LUASelectionMode *> &luaSelectionModes() const;
	void reloadSelectionModeScripts();
};

inline const core::DynamicArray<LUABrush *> &ScriptManager::luaBrushes() const {
	return _luaBrushes;
}

inline int ScriptManager::activeLuaBrushIndex() const {
	return _activeLuaBrushIndex;
}

inline const core::DynamicArray<LUASelectionMode *> &ScriptManager::luaSelectionModes() const {
	return _luaSelectionModes;
}

} // namespace voxedit
