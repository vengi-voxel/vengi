/**
 * @file
 */

#include "AggroMgr.h"
#include <algorithm>

namespace ai {

class CharacterIdPredicate {
private:
	const CharacterId& _id;
public:
	explicit CharacterIdPredicate(const CharacterId& id) :
		_id(id) {
	}

	inline bool operator()(const Entry &n1) {
		return n1.getCharacterId() == _id;
	}
};

static bool EntrySorter(const Entry& a, const Entry& b) {
	if (a.getAggro() > b.getAggro()) {
		return false;
	}
	if (::fabs(a.getAggro() - b.getAggro()) < 0.0000001f) {
		return a.getCharacterId() < b.getCharacterId();
	}
	return true;
}

/**
 * @brief Remove the entries from the list that have no aggro left.
 * This list is ordered, so we will only remove the first X elements.
 */
void AggroMgr::cleanupList() {
	EntriesIter::difference_type remove = 0;
	for (EntriesIter i = _entries.begin(); i != _entries.end(); ++i) {
		const float aggroValue = i->getAggro();
		if (aggroValue > 0.0f) {
			break;
		}

		++remove;
	}

	if (remove == 0) {
		return;
	}

	const int size = static_cast<int>(_entries.size());
	if (size == remove) {
		_entries.clear();
		return;
	}

	EntriesIter i = _entries.begin();
	std::advance(i, remove);
	_entries.erase(_entries.begin(), i);
}

void AggroMgr::sort() const {
	if (!_dirty) {
		return;
	}
	std::sort(_entries.begin(), _entries.end(), EntrySorter);
	_dirty = false;
}

void AggroMgr::setReduceByRatio(float reduceRatioSecond, float minAggro) {
	_reduceType = RATIO;
	_reduceValueSecond = 0.0f;
	_reduceRatioSecond = reduceRatioSecond;
	_minAggro = minAggro;
}

void AggroMgr::setReduceByValue(float reduceValueSecond) {
	_reduceType = VALUE;
	_reduceValueSecond = reduceValueSecond;
	_reduceRatioSecond = 0.0f;
	_minAggro = 0.0f;
}

void AggroMgr::resetReduceValue() {
	_reduceType = DISABLED;
	_reduceValueSecond = 0.0f;
	_reduceRatioSecond = 0.0f;
	_minAggro = 0.0f;
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

EntryPtr AggroMgr::addAggro(CharacterId id, float amount) {
	const CharacterIdPredicate p(id);
	EntriesIter i = std::find_if(_entries.begin(), _entries.end(), p);
	if (i == _entries.end()) {
		Entry newEntry(id, amount);
		switch (_reduceType) {
		case RATIO:
			newEntry.setReduceByRatio(_reduceRatioSecond, _minAggro);
			break;
		case VALUE:
			newEntry.setReduceByValue(_reduceValueSecond);
			break;
		default:
			break;
		}
		_entries.push_back(newEntry);
		_dirty = true;
		return &_entries.back();
	}

	i->addAggro(amount);
	_dirty = true;
	return &*i;
}

EntryPtr AggroMgr::getHighestEntry() const {
	if (_entries.empty()) {
		return nullptr;
	}

	sort();

	return &_entries.back();
}

}
