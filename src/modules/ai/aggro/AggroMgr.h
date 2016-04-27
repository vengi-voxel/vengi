#pragma once

#include <vector>
#include <memory>
#include "aggro/Entry.h"

namespace ai {

class AI;
typedef std::shared_ptr<AI> AIPtr;

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

	/**
	 * @brief Remove the entries from the list that have no aggro left.
	 * This list is ordered, so we will only remove the first X elements.
	 */
	void cleanupList();

	inline void sort() const;
public:
	AggroMgr(std::size_t expectedEntrySize = 0u);
	virtual ~AggroMgr();

	/**
	 * @brief this will update the aggro list according to the reduction type of an entry.
	 * @param[in] deltaMillis The current milliseconds to use to update the aggro value of the entries.
	 */
	void update(int64_t deltaMillis);

	/**
	 * @brief will increase the aggro
	 * @param[in,out] entity The entity to increase the aggro against
	 * @param[in] amount The amount to increase the aggro for
	 * @return The aggro @c Entry that was added or updated. Useful for changing the reduce type or amount.
	 */
	EntryPtr addAggro(const AIPtr& entity, float amount);

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
	EntryPtr getHighestEntry() const;
};

}
