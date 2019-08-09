/**
 * @file
 */

#include "Attributes.h"
#include "core/Common.h"
#include "collection/Set.h"
#include <unordered_set>

namespace attrib {

Attributes::Attributes(Attributes* parent) :
		_dirty(false), _lock("Attributes"), _attribLock("Attributes2"), _parent(parent) {
}

bool Attributes::update(long dt) {
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

	for (const auto& p : percentages) {
		max[p.first] *= 1.0 + (p.second * 0.01);
	}

	core::ScopedReadLock scopedLock(_attribLock);
	if (!_listeners.empty()) {
		const TypeSet& diff = core::mapFindChangedValues(_max, max);
		for (const auto& listener : _listeners) {
			for (const Type& e : diff) {
				listener(DirtyValue{e, false, max[e]});
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
		i->second = core_min(mi->second, i->second);
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
	for (const auto& e : containers) {
		const Container& c = e.second;
		const double stackCount = c.stackCount();
		const Values& abs = c.absolute();
		for (ValuesConstIter i = abs.begin(); i != abs.end(); ++i) {
			absolutes[i->first] += i->second * stackCount;
		}
		const Values& rel = c.percentage();
		for (ValuesConstIter i = rel.begin(); i != rel.end(); ++i) {
			percentages[i->first] += i->second * stackCount;
		}
	}
}

void Attributes::add(const Container& container) {
	core::ScopedWriteLock scopedLock(_lock);
	const auto& i = _containers.insert(std::make_pair(container.name(), container));
	if (i.second) {
		_dirty = true;
		return;
	}
	if (i.first->second.increaseStackCount()) {
		_dirty = true;
	}
}

void Attributes::add(Container&& container) {
	core::ScopedWriteLock scopedLock(_lock);
	const auto& i = _containers.insert(std::make_pair(container.name(), container));
	if (i.second) {
		_dirty = true;
		return;
	}
	if (i.first->second.increaseStackCount()) {
		_dirty = true;
	}
}

void Attributes::add(const ContainerPtr& container) {
	if (!container) {
		return;
	}
	core::ScopedWriteLock scopedLock(_lock);
	_containerPtrs.insert(std::make_pair(container->name(), container));
	const auto& i = _containers.insert(std::make_pair(container->name(), *container.get()));
	if (i.second) {
		_dirty = true;
		return;
	}
	if (i.first->second.increaseStackCount()) {
		_dirty = true;
	}
}

void Attributes::remove(const Container& container) {
	remove(container.name());
}

void Attributes::remove(const ContainerPtr& container) {
	if (!container) {
		return;
	}
	remove(container->name());
}

void Attributes::remove(const std::string& name) {
	core::ScopedWriteLock scopedLock(_lock);
	_containerPtrs.erase(name);
	const auto& i = _containers.find(name);
	if (i == _containers.end()) {
		return;
	}
	if (i->second.decreaseStackCount()) {
		_dirty = true;
		return;
	}
	const auto& erased = _containers.erase(i);
	if (erased != _containers.end()) {
		_dirty = true;
	}
}

double Attributes::setCurrent(Type type, double value) {
	core::ScopedWriteLock scopedLock(_attribLock);
	auto i = _max.find(type);
	if (i == _max.end()) {
		_current[type] = value;
		for (const auto& listener : _listeners) {
			listener(DirtyValue{type, true, value});
		}
		return value;
	}
	const double max = core_min(i->second, value);
	_current[type] = max;
	for (const auto& listener : _listeners) {
		listener(DirtyValue{type, true, max});
	}
	return max;
}

void Attributes::markAsDirty() {
	for (ValuesIter i = _current.begin(); i != _current.end(); ++i) {
		for (const auto& listener : _listeners) {
			listener(DirtyValue{i->first, true, i->second});
		}
	}
	for (ValuesIter i = _max.begin(); i != _max.end(); ++i) {
		for (const auto& listener : _listeners) {
			listener(DirtyValue{i->first, false, i->second});
		}
	}
}

}
