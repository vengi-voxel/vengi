/**
 * @file
 */

#include "Attributes.h"
#include "core/Set.h"

namespace attrib {

Attributes::Attributes(Attributes* parent) :
		_dirty(false), _lock("Attributes"), _attribLock("Attributes2"), _parent(parent) {
}

bool Attributes::onFrame(long dt) {
	if (_parent != nullptr) {
		_parent->onFrame(dt);
	}
	if (!_dirty.exchange(false)) {
		return false;
	}

	Values max;
	calculateMax(max);

	core::ScopedReadLock scopedLock(_attribLock);
	//const std::unordered_set<Types>& diff = core::mapKeysDifference(_max, max);
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

void Attributes::calculateMax(Values& max) const {
	if (_parent != nullptr) {
		_parent->calculateMax(max);
	}

	Containers containers;
	{
		core::ScopedReadLock scopedLock(_lock);
		containers = _containers;
	}
	for (const Container& c : containers) {
		const Values& abs = c.absolute();
		for (ValuesConstIter i = abs.begin(); i != abs.end(); ++i) {
			max[i->first] += i->second;
		}
	}
	for (const Container& c : containers) {
		const Values& abs = c.percentage();
		for (ValuesConstIter i = abs.begin(); i != abs.end(); ++i) {
			max[i->first] *= 1.0 + (i->second * 0.01);
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

double Attributes::setCurrent(Types type, double value) {
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
