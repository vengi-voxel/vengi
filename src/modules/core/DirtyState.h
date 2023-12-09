/**
 * @file
 */

#pragma once

namespace core {

class DirtyState {
protected:
	bool _dirty = false;

public:
	virtual void markDirty() {
		_dirty = true;
	}

	virtual bool dirty() const {
		return _dirty;
	}

	virtual void markClean() {
		_dirty = false;
	}
};

} // namespace core
