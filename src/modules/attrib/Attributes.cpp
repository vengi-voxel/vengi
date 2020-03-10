/**
 * @file
 */

#include "Attributes.h"
#include "core/Common.h"
#include <unordered_set>

namespace attrib {

typedef std::unordered_set<Type, network::EnumHash<Type> > TypeSet;

TypeSet mapFindChangedValues(const Values& in1, const Values& in2) {
	TypeSet result;
	for (const auto& e : in1) {
		const Type& key = e->key;
		const auto& i = in2.find(key);
		if (i == in2.end()) {
			result.insert(key);
			continue;
		}
		const double oldValue = i->value;
		const double newValue = e->value;
		if (SDL_fabs(newValue - oldValue) > (double)0.000001) {
			result.insert(key);
		}
	}
	for (const auto& e : in2) {
		const Type& key = e->key;
		auto i = in1.find(key);
		if (i == in2.end()) {
			result.insert(key);
			continue;
		}
	}

	return result;
}

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
		double value;
		if (!max.get(p->key, value)) {
			continue;
		}
		value *= 1.0 + (p->value * 0.01);
		max.put(p->key, value);
	}

	core::ScopedReadLock scopedLock(_attribLock);
	if (!_listeners.empty()) {
		const TypeSet& diff = mapFindChangedValues(_max, max);
		for (const Type& e : diff) {
			double value;
			if (!max.get(e, value)) {
				continue;
			}
			const DirtyValue v{e, false, value};
			for (const auto& listener : _listeners) {
				listener(v);
			}
		}
	}
	_max = max;

	// cap your currents to the max allowed value
	for (ValuesIter i = _current.begin(); i != _current.end(); ++i) {
		ValuesIter mi = _max.find(i->key);
		if (mi == _max.end()) {
			continue;
		}
		double old = i->value;
		i->value = core_min(mi->value, i->value);
		if (SDL_fabs(old - i->value) > (double)0.000001) {
			const DirtyValue v{i->key, true, i->value};
			for (const auto& listener : _listeners) {
				listener(v);
			}
		}
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
		const Container& c = e->value;
		const double stackCount = c.stackCount();
		const Values& abs = c.absolute();
		for (ValuesConstIter i = abs.begin(); i != abs.end(); ++i) {
			auto v = absolutes.find(i->key);
			if (v == absolutes.end()) {
				absolutes.put(i->key, i->value * stackCount);
			} else {
				v->value += i->value * stackCount;
			}
		}
		const Values& rel = c.percentage();
		for (ValuesConstIter i = rel.begin(); i != rel.end(); ++i) {
			auto v = percentages.find(i->key);
			if (v == percentages.end()) {
				percentages.put(i->key, i->value * stackCount);
			} else {
				v->value += i->value * stackCount;
			}
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
	auto i = _max.find(type);
	if (i == _max.end()) {
		const DirtyValue v{type, true, value};
		_current.put(type, value);
		for (const auto& listener : _listeners) {
			listener(v);
		}
		return value;
	}
	const double max = core_min(i->second, value);
	_current.put(type, max);
	const DirtyValue v{type, true, max};
	for (const auto& listener : _listeners) {
		listener(v);
	}
	return max;
}

void Attributes::markAsDirty() {
	for (ValuesIter i = _current.begin(); i != _current.end(); ++i) {
		const DirtyValue v{i->first, true, i->second};
		for (const auto& listener : _listeners) {
			listener(v);
		}
	}
	for (ValuesIter i = _max.begin(); i != _max.end(); ++i) {
		const DirtyValue v{i->first, false, i->second};
		for (const auto& listener : _listeners) {
			listener(v);
		}
	}
}

}
