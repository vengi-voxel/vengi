/**
 * @file
 */

#include "tb_hashtable.h"
#include "tb_system.h"
#include "tb_tempbuffer.h"

namespace tb {

// FIX: reduce memory (block allocation of ITEM)
// FIX: should shrink when deleting single items (but not when adding items!)
// FIX: should grow when about 70% full instead of 100%

TBHashTable::TBHashTable() : m_buckets(nullptr), m_num_buckets(0), m_num_items(0) {
}

TBHashTable::~TBHashTable() {
	removeAll();
}

void TBHashTable::removeAll(bool delContent) {
	for (uint32_t i = 0; i < m_num_buckets; i++) {
		ITEM *item = m_buckets[i];
		while (item != nullptr) {
			ITEM *item_next = item->next;
			if (delContent) {
				deleteContent(item->content);
			}
			delete item;
			item = item_next;
		}
	}
	delete[] m_buckets;
	m_buckets = nullptr;
	m_num_buckets = m_num_items = 0;
}

bool TBHashTable::rehash(uint32_t newNumBuckets) {
	if (newNumBuckets == m_num_buckets) {
		return true;
	}
	if (ITEM **new_buckets = new ITEM *[newNumBuckets]) {
		SDL_memset(new_buckets, 0, sizeof(ITEM *) * newNumBuckets);
		// Rehash all items into the new buckets
		for (uint32_t i = 0; i < m_num_buckets; i++) {
			ITEM *item = m_buckets[i];
			while (item != nullptr) {
				ITEM *item_next = item->next;
				// Add it to new_buckets
				uint32_t bucket = item->key & (newNumBuckets - 1);
				item->next = new_buckets[bucket];
				new_buckets[bucket] = item;
				item = item_next;
			}
		}
		// Delete old buckets and update
		delete[] m_buckets;
		m_buckets = new_buckets;
		m_num_buckets = newNumBuckets;
		return true;
	}
	return false;
}

uint32_t TBHashTable::getSuitableBucketsCount() const {
	// As long as we use FNV for TBID (in TBGetHash), power of two hash sizes are the best.
	if (m_num_items == 0U) {
		return 16;
	}
	return m_num_items * 2;
}

void *TBHashTable::get(uint32_t key) const {
	if (m_num_buckets == 0U) {
		return nullptr;
	}
	uint32_t bucket = key & (m_num_buckets - 1);
	ITEM *item = m_buckets[bucket];
	while (item != nullptr) {
		if (item->key == key) {
			return item->content;
		}
		item = item->next;
	}
	return nullptr;
}

bool TBHashTable::add(uint32_t key, void *content) {
	if (needRehash() && !rehash(getSuitableBucketsCount())) {
		return false;
	}
	core_assert(!get(key));
	if (ITEM *item = new ITEM) {
		uint32_t bucket = key & (m_num_buckets - 1);
		item->key = key;
		item->content = content;
		item->next = m_buckets[bucket];
		m_buckets[bucket] = item;
		m_num_items++;
		return true;
	}
	return false;
}

void *TBHashTable::remove(uint32_t key) {
	if (m_num_buckets == 0U) {
		return nullptr;
	}
	uint32_t bucket = key & (m_num_buckets - 1);
	ITEM *item = m_buckets[bucket];
	ITEM *prev_item = nullptr;
	while (item != nullptr) {
		if (item->key == key) {
			if (prev_item != nullptr) {
				prev_item->next = item->next;
			} else {
				m_buckets[bucket] = item->next;
			}
			m_num_items--;
			void *content = item->content;
			delete item;
			return content;
		}
		prev_item = item;
		item = item->next;
	}
	core_assert(!"This hash table didn't contain the given key!");
	return nullptr;
}

#ifdef TB_RUNTIME_DEBUG_INFO

void TBHashTable::debug() {
	Log::debug("Hash table: ");
	int total_count = 0;
	for (uint32_t i = 0; i < m_num_buckets; i++) {
		int count = 0;
		ITEM *item = m_buckets[i];
		while (item != nullptr) {
			count++;
			item = item->next;
		}
		Log::debug("%d ", count);
		total_count += count;
	}
	Log::debug(" (total: %d of %d buckets)", total_count, m_num_buckets);
}

#endif // TB_RUNTIME_DEBUG_INFO

TBHashTableIterator::TBHashTableIterator(TBHashTable *hashTable)
	: m_hash_table(hashTable), m_current_bucket(0), m_current_item(nullptr) {
}

void *TBHashTableIterator::getNextContent() {
	if (m_current_bucket == m_hash_table->m_num_buckets) {
		return nullptr;
	}
	if ((m_current_item != nullptr) && (m_current_item->next != nullptr)) {
		m_current_item = m_current_item->next;
	} else {
		if (m_current_item != nullptr) {
			m_current_bucket++;
		}
		if (m_current_bucket == m_hash_table->m_num_buckets) {
			return nullptr;
		}
		while (m_current_bucket < m_hash_table->m_num_buckets) {
			m_current_item = m_hash_table->m_buckets[m_current_bucket];
			if (m_current_item != nullptr) {
				break;
			}
			m_current_bucket++;
		}
	}
	return m_current_item != nullptr ? m_current_item->content : nullptr;
}

} // namespace tb
