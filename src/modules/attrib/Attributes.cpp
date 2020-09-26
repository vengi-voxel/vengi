/**
 * @file
 */

#include "Attributes.h"
#include "core/Common.h"
#include "core/Trace.h"
#include <glm/common.hpp>
#include <glm/ext/scalar_constants.hpp>

namespace attrib {

Attributes::Attributes(Attributes* parent) :
		_dirty(false), _lock("Attributes"), _attribLock("Attributes2"), _parent(parent) {
	_current.fill(0.0);
	_max.fill(0.0);
}

bool Attributes::update(long dt) {
	core_trace_scoped(AttributesUpdates);
	bool updated = false;
	if (_parent != nullptr) {
		updated = _parent->update(dt);
		if (updated) {
			_dirty = true;
		}
	}
	if (!_dirty.exchange(false)) {
		return updated;
	}

	Values max;
	Values percentages;
	calculateMax(max, percentages);

	for (size_t i = 0; i < percentages.size(); ++i) {
		if (max[i] <= glm::epsilon<double>()) {
			continue;
		}
		max[i] *= 1.0 + (percentages[i] * 0.01);
	}

	core::ScopedReadLock scopedLock(_attribLock);
	if (!_listeners.empty()) {
		for (size_t i = 0; i < max.size(); ++i) {
			const double oldValue = _max[i];
			const double newValue = max[i];
			if (glm::abs(newValue - oldValue) <= glm::epsilon<double>()) {
				continue;
			}
			const DirtyValue v{(Type)i, false, max[i]};
			for (const auto& listener : _listeners) {
				listener(v);
			}
		}
	}
	_max = max;

	// cap your currents to the max allowed value
	for (size_t i = 0; i < _current.size(); ++i) {
		const double old = _current[i];
		_current[i] = core_min(_max[i], old);
		if (glm::abs(old - _current[i]) > glm::epsilon<double>()) {
			const DirtyValue v{(Type)i, true, _current[i]};
			for (const auto& listener : _listeners) {
				listener(v);
			}
		}
	}
	return true;
}

void Attributes::calculateMax(Values& absolutes, Values& percentages) const {
	absolutes.fill(0.0);
	percentages.fill(0.0);

	if (_parent != nullptr) {
		_parent->calculateMax(absolutes, percentages);
	}

	Containers containers;
	{
		core::ScopedReadLock scopedLock(_lock);
		containers = _containers;
	}

	for (const auto& e : containers) {
		const Container& c = e->value;
		const double stackCount = c.stackCount();
		const Values& abs = c.absolute();
		for (size_t i = 0; i < abs.size(); ++i) {
			absolutes[i] += abs[i] * stackCount;
		}
		const Values& rel = c.percentage();
		for (size_t i = 0; i < rel.size(); ++i) {
			percentages[i] += rel[i] * stackCount;
		}
	}
}

bool Attributes::add(const Container& container) {
	core::ScopedWriteLock scopedLock(_lock);
	auto i = _containers.find(container.name());
	if (i == _containers.end()) {
		_containers.put(container.name(), container);
		_dirty = true;
		return true;
	}
	if (i->value.increaseStackCount()) {
		_dirty = true;
	}
	return false;
}

bool Attributes::add(const ContainerPtr& container) {
	if (!container) {
		return false;
	}
	if (add(*container.get())) {
		_containerPtrs.put(container->name(), container);
		return true;
	}
	return false;
}

void Attributes::remove(const Container& container) {
	remove(container.name());
}

void Attributes::remove(const ContainerPtr& container) {
	if (!container) {
		return;
	}
	remove(container->name());
	_containerPtrs.remove(container->name());
}

void Attributes::remove(const core::String& name) {
	core::ScopedWriteLock scopedLock(_lock);
	auto i = _containers.find(name);
	if (i == _containers.end()) {
		return;
	}
	_dirty = true;
	if (i->value.decreaseStackCount()) {
		return;
	}
	_containers.erase(i);
}

double Attributes::setCurrent(Type type, double value) {
	core::ScopedWriteLock scopedLock(_attribLock);
	const auto idx = core::enumVal(type);
	const double max = _max[idx] <= glm::epsilon<double>() ? value : core_min(_max[idx], value);
	_current[idx] = max;
	const DirtyValue v{type, true, max};
	for (const auto& listener : _listeners) {
		listener(v);
	}
	return max;
}

void Attributes::markAsDirty() {
	for (size_t i = 0; i < _current.size(); ++i) {
		const DirtyValue v{(Type)i, true, _current[i]};
		for (const auto& listener : _listeners) {
			listener(v);
		}
	}
	for (size_t i = 0; i < _max.size(); ++i) {
		const DirtyValue v{(Type)i, false, _max[i]};
		for (const auto& listener : _listeners) {
			listener(v);
		}
	}
}

}
