#include "AI.h"
#include "aggro/AggroMgr.h"
#include "ICharacter.h"
#include <algorithm>

namespace ai {

AggroMgr::AggroMgr(std::size_t expectedEntrySize) :
		_dirty(false) {
	if (expectedEntrySize > 0)
		_entries.reserve(expectedEntrySize);
}

AggroMgr::~AggroMgr() {
}

void AggroMgr::cleanupList() {
	EntriesIter::difference_type remove = 0;
	for (EntriesIter i = _entries.begin(); i != _entries.end(); ++i) {
		const float aggroValue = i->getAggro();
		if (aggroValue > 0.0f)
			break;

		++remove;
	}

	if (remove == 0)
		return;

	const int size = static_cast<int>(_entries.size());
	if (size == remove) {
		_entries.clear();
		return;
	}

	EntriesIter i = _entries.begin();
	std::advance(i, remove);
	_entries.erase(_entries.begin(), i);
}

void AggroMgr::update(int64_t deltaMillis) {
	for (EntriesIter i = _entries.begin(); i != _entries.end(); ++i) {
		_dirty |= i->reduceByTime(deltaMillis);
	}

	if (_dirty) {
		sort();
		cleanupList();
	}
}

class CharacterIdPredicate {
private:
	const CharacterId& _id;
public:
	CharacterIdPredicate(const CharacterId& id) :
			_id(id) {
	}

	bool operator()(const Entry &n1) {
		return n1.getCharacterId() == _id;
	}
};

static bool EntrySorter(const Entry& a, const Entry& b) {
	if (a.getAggro() > b.getAggro())
		return false;
	if (::fabs(a.getAggro() - b.getAggro()) < 0.0000001f)
		return a.getCharacterId() < b.getCharacterId();
	return true;
}

inline void AggroMgr::sort() const {
	if (!_dirty)
		return;
	std::sort(_entries.begin(), _entries.end(), EntrySorter);
	_dirty = false;
}

Entry* AggroMgr::addAggro(const AIPtr& entity, float amount) {
	const CharacterId id = entity->getId();
	const CharacterIdPredicate p(id);
	EntriesIter i = std::find_if(_entries.begin(), _entries.end(), p);
	if (i == _entries.end()) {
		const Entry newEntry(id, amount);
		_entries.push_back(newEntry);
		_dirty = true;
		return &_entries.back();
	}

	i->addAggro(amount);
	_dirty = true;
	return &*i;
}

EntryPtr AggroMgr::getHighestEntry() const {
	if (_entries.empty())
		return nullptr;

	sort();

	return &_entries.back();
}

}
