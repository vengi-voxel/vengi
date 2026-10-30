/**
 * @file
 */

#pragma once

#include "BrushDraft.h"
#include "brush/Brush.h"
#include "core/SharedPtr.h"
#include "core/collection/DynamicArray.h"
#include <type_traits>

namespace voxedit {

class SceneManager;

struct BrushPresentation {
	core::String label;
	core::String icon;
	core::String command;
	core::String panel;
	bool normalPaletteOnly = false;
};

/** Owns registered tools. Stable IDs are independent of toolbar order and enum values. */
class BrushRegistry {
private:
	struct Entry {
		core::String id;
		core::SharedPtr<Brush> tool;
		core::SharedPtr<BrushDraft> draft;
		core::Function<core::SharedPtr<Brush>(const Brush &)> clone;
		const void *type = nullptr;
		BrushPresentation presentation;
	};
	core::DynamicArray<Entry> _entries;
	core::Buffer<Brush *> _tools;

	bool insert(const core::String &id, const core::SharedPtr<Brush> &tool, const core::SharedPtr<BrushDraft> &draft,
				const core::Function<core::SharedPtr<Brush>(const Brush &)> &clone, const void *type);
	template<class T>
	static const void *typeKey() {
		static const char key = 0;
		return &key;
	}

public:
	template<class T>
	bool add(const core::String &id, const core::SharedPtr<T> &tool, const core::SharedPtr<BrushDraft> &draft = {},
			 const core::Function<core::SharedPtr<Brush>(const Brush &)> &factory = {}) {
		auto clone = factory;
		if (!clone) {
			if constexpr (std::is_copy_constructible<T>::value) {
				clone = [](const Brush &state) {
					auto copy = core::make_shared<T>((const T &)state);
					return core::SharedPtr<Brush>(copy);
				};
			}
		}
		return insert(id, core::SharedPtr<Brush>(tool), draft, clone, typeKey<T>());
	}
	bool remove(const Brush *tool);
	BrushPresentation &presentation(const core::String &id);
	const BrushPresentation &presentation(const core::String &id) const;
	core::SharedPtr<Brush> evaluation(const Brush *tool, const Brush *state = nullptr) const;
	Brush *find(const core::String &id) const;
	BrushDraft *draft(const Brush *tool) const;
	BrushDraft *activeDraft() const;
	Brush *find(BrushType type) const;
	const core::String &id(const Brush &tool) const;
	const core::Buffer<Brush *> &tools() const {
		return _tools;
	}

	template<class T>
	T &get(const core::String &id) const {
		for (const Entry &entry : _entries) {
			if (entry.id == id) {
				core_assert(entry.type == typeKey<T>());
				return *(T *)entry.tool.get();
			}
		}
		core_assert(false);
		return *(T *)nullptr;
	}
};

void registerBuiltinBrushes(BrushRegistry &registry, SceneManager *sceneManager);

} // namespace voxedit
