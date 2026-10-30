/**
 * @file
 */

#include "BrushRegistry.h"
#include "BrushDraft.h"

namespace voxedit {

bool BrushRegistry::insert(const core::String &id, const core::SharedPtr<Brush> &tool,
						   const core::SharedPtr<BrushDraft> &draft,
						   const core::Function<core::SharedPtr<Brush>(const Brush &)> &clone, const void *type) {
	if (id.empty() || !tool || find(id)) {
		return false;
	}
	BrushPresentation presentation;
	presentation.label = tool->name();
	presentation.panel = id;
	_entries.push_back({id, tool, draft, clone, type, presentation});
	_tools.push_back(tool.get());
	return true;
}

BrushPresentation &BrushRegistry::presentation(const core::String &id) {
	for (auto &entry : _entries) {
		if (entry.id == id) {
			return entry.presentation;
		}
	}
	core_assert(false);
	return _entries.front().presentation;
}

const BrushPresentation &BrushRegistry::presentation(const core::String &id) const {
	for (const auto &entry : _entries) {
		if (entry.id == id) {
			return entry.presentation;
		}
	}
	core_assert(false);
	return _entries.front().presentation;
}

bool BrushRegistry::remove(const Brush *tool) {
	for (size_t i = 0; i < _entries.size(); ++i) {
		if (_entries[i].tool.get() == tool) {
			_entries.erase(i);
			_tools.erase(i);
			return true;
		}
	}
	return false;
}

core::SharedPtr<Brush> BrushRegistry::evaluation(const Brush *tool, const Brush *state) const {
	for (const Entry &entry : _entries) {
		if (entry.tool.get() == tool && entry.clone) {
			return entry.clone(state ? *state : *tool);
		}
	}
	return {};
}

BrushDraft *BrushRegistry::draft(const Brush *tool) const {
	for (const Entry &entry : _entries) {
		if (entry.tool.get() == tool && entry.draft && entry.draft->enabled()) {
			return entry.draft.get();
		}
	}
	return nullptr;
}

BrushDraft *BrushRegistry::activeDraft() const {
	for (const Entry &entry : _entries) {
		if (entry.draft && entry.draft->active()) {
			return entry.draft.get();
		}
	}
	return nullptr;
}

Brush *BrushRegistry::find(const core::String &id) const {
	for (const Entry &entry : _entries) {
		if (entry.id == id) {
			return entry.tool.get();
		}
	}
	return nullptr;
}

Brush *BrushRegistry::find(BrushType type) const {
	for (Brush *tool : _tools) {
		if (tool->type() == type) {
			return tool;
		}
	}
	return nullptr;
}

const core::String &BrushRegistry::id(const Brush &tool) const {
	for (const Entry &entry : _entries) {
		if (entry.tool.get() == &tool) {
			return entry.id;
		}
	}
	return core::String::Empty;
}

} // namespace voxedit
