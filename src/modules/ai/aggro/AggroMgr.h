/**
 * @file
 *
 * @defgroup Aggro
 * @{
 * The aggro manager can be used to create interactions between AI controlled entities.
 */
#pragma once

#include <vector>
#include "ICharacter.h"
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

	/**
	 * @brief Remove the entries from the list that have no aggro left.
	 * This list is ordered, so we will only remove the first X elements.
	 */
	void cleanupList();

	inline void sort() const;
public:
	explicit AggroMgr(std::size_t expectedEntrySize = 0u) :
		_dirty(false) {
		if (expectedEntrySize > 0) {
			_entries.reserve(expectedEntrySize);
		}
	}

	virtual ~AggroMgr() {
	}

	void setReduceByRatio(float reduceRatioSecond, float minAggro);

	void setReduceByValue(float reduceValueSecond);

	void resetReduceValue();

	/**
	 * @brief this will update the aggro list according to the reduction type of an entry.
	 * @param[in] deltaMillis The current milliseconds to use to update the aggro value of the entries.
	 */
	void update(int64_t deltaMillis);

	/**
	 * @brief will increase the aggro
	 * @param[in] id The entity id to increase the aggro against
	 * @param[in] amount The amount to increase the aggro for
	 * @return The aggro @c Entry that was added or updated. Useful for changing the reduce type or amount.
	 */
	EntryPtr addAggro(CharacterId id, float amount);

	/**
	 * @return All the aggro entries
	 */
	const Entries& getEntries() const {
		return _entries;
	}

	inline size_t count() const {
		return _entries.size();
	}

	/**
	 * @brief Get the entry with the highest aggro value.
	 *
	 * @note Might execute a sort on the list if its dirty
	 */
	EntryPtr getHighestEntry() const;
};

}

/**
 * @}
 */
