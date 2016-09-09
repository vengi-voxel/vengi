#pragma once

#include <vector>
#include <memory>
#include "ICharacter.h"
#include <algorithm>
#include "aggro/Entry.h"

namespace ai {

/**
 * @brief Manages the aggro values for one @c AI instance. There are several ways to degrade the aggro values.
 */
class AggroMgr {
public:
	typedef std::vector<Entry> Entries;
	typedef Entries::iterator EntriesIter;
protected:
	mutable Entries _entries;

	mutable bool _dirty;

	float _minAggro = 0.0f;
	float _reduceRatioSecond = 0.0f;
	float _reduceValueSecond = 0.0f;
	ReductionType _reduceType = DISABLED;

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
	void cleanupList() {
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

	inline void sort() const {
		if (!_dirty) {
			return;
		}
		std::sort(_entries.begin(), _entries.end(), EntrySorter);
		_dirty = false;
	}
public:
	explicit AggroMgr(std::size_t expectedEntrySize = 0u) :
		_dirty(false) {
		if (expectedEntrySize > 0) {
			_entries.reserve(expectedEntrySize);
		}
	}

	virtual ~AggroMgr() {
	}

	inline void setReduceByRatio(float reduceRatioSecond, float minAggro) {
		_reduceType = RATIO;
		_reduceValueSecond = 0.0f;
		_reduceRatioSecond = reduceRatioSecond;
		_minAggro = minAggro;
	}

	inline void setReduceByValue(float reduceValueSecond) {
		_reduceType = VALUE;
		_reduceValueSecond = reduceValueSecond;
		_reduceRatioSecond = 0.0f;
		_minAggro = 0.0f;
	}

	inline void resetReduceValue() {
		_reduceType = DISABLED;
		_reduceValueSecond = 0.0f;
		_reduceRatioSecond = 0.0f;
		_minAggro = 0.0f;
	}

	/**
	 * @brief this will update the aggro list according to the reduction type of an entry.
	 * @param[in] deltaMillis The current milliseconds to use to update the aggro value of the entries.
	 */
	void update(int64_t deltaMillis) {
		for (EntriesIter i = _entries.begin(); i != _entries.end(); ++i) {
			_dirty |= i->reduceByTime(deltaMillis);
		}

		if (_dirty) {
			sort();
			cleanupList();
		}
	}

	/**
	 * @brief will increase the aggro
	 * @param[in] id The entity id to increase the aggro against
	 * @param[in] amount The amount to increase the aggro for
	 * @return The aggro @c Entry that was added or updated. Useful for changing the reduce type or amount.
	 */
	EntryPtr addAggro(CharacterId id, float amount) {
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

	/**
	 * @return All the aggro entries
	 */
	const Entries& getEntries() const {
		return _entries;
	}

	/**
	 * @brief Get the entry with the highest aggro value.
	 *
	 * @note Might execute a sort on the list if its dirty
	 */
	EntryPtr getHighestEntry() const {
		if (_entries.empty()) {
			return nullptr;
		}

		sort();

		return &_entries.back();
	}
};

}
