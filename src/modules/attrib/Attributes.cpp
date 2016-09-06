/**
 * @file
 */

#include "Attributes.h"
#include "core/Set.h"
#include <unordered_set>

namespace attrib {

Attributes::Attributes(Attributes* parent) :
		_dirty(false), _lock("Attributes"), _attribLock("Attributes2"), _parent(parent) {
}

bool Attributes::onFrame(long dt) {
	bool updated = false;
	if (_parent != nullptr) {
		updated = _parent->onFrame(dt);
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

	for (const auto& p : percentages) {
		max[p.first] *= 1.0 + (p.second * 0.01);
	}

	core::ScopedReadLock scopedLock(_attribLock);
	if (!_listeners.empty()) {
		const std::unordered_set<Type>& diff = core::mapFindChangedValues(_max, max);
		for (const auto& listener : _listeners) {
			for (const Type& e : diff) {
				listener(e);
			}
		}
	}
	_max = max;

	// cap your currents to the max allowed value
	for (ValuesIter i = _current.begin(); i != _current.end(); ++i) {
		ValuesIter mi = _max.find(i->first);
		if (mi == _max.end()) {
			continue;
		}
		i->second = std::min(mi->second, i->second);
	}
	return true;
}

void Attributes::calculateMax(Values& absolutes, Values& percentages) const {
	if (_parent != nullptr) {
		_parent->calculateMax(absolutes, percentages);
	}

	Containers containers;
	{
		core::ScopedReadLock scopedLock(_lock);
		containers = _containers;
	}
	for (const Container& c : containers) {
		const Values& abs = c.absolute();
		for (ValuesConstIter i = abs.begin(); i != abs.end(); ++i) {
			absolutes[i->first] += i->second;
		}
	}
	for (const Container& c : containers) {
		const Values& rel = c.percentage();
		for (ValuesConstIter i = rel.begin(); i != rel.end(); ++i) {
			percentages[i->first] += i->second;
		}
	}
}

void Attributes::add(const Container& container) {
	core::ScopedWriteLock scopedLock(_lock);
	_containers.insert(container);
	_dirty = true;
}

void Attributes::remove(const Container& container) {
	core::ScopedWriteLock scopedLock(_lock);
	_containers.erase(container);
	_dirty = true;
}

void Attributes::add(Container&& container) {
	core::ScopedWriteLock scopedLock(_lock);
	_containers.insert(container);
	_dirty = true;
}

void Attributes::add(const ContainerPtr& container) {
	if (!container)
		return;
	core::ScopedWriteLock scopedLock(_lock);
	_containerPtrs.insert(container);
	_containers.insert(*container.get());
	_dirty = true;
}

void Attributes::remove(const ContainerPtr& container) {
	if (!container) {
		return;
	}
	core::ScopedWriteLock scopedLock(_lock);
	_containers.erase(*container.get());
	_containerPtrs.erase(container);
	_dirty = true;
}

void Attributes::remove(Container&& container) {
	core::ScopedWriteLock scopedLock(_lock);
	_containers.erase(container);
	_dirty = true;
}

double Attributes::setCurrent(Type type, double value) {
	core::ScopedWriteLock scopedLock(_attribLock);
	auto i = _max.find(type);
	if (i == _max.end()) {
		_current[type] = value;
		return value;
	}
	const double max = std::min(i->second, value);
	_current[type] = max;
	return max;
}

}
