/**
 * @file
 */

#pragma once

#include "tb_core.h"
#include "core/Assert.h"

namespace tb {

/** TBHashTable is a minimal hash table, for hashing anything using a uint32 key. */

class TBHashTable
{
public:
	TBHashTable();
	virtual ~TBHashTable();

	/** Remove all items without deleting the content. */
	void removeAll() { removeAll(false); }

	/** Remove all items and delete the content.
		This requires TBHashTable to be subclassed and implementing DeleteContent.
		You would typically do this by using TBHashTableOf or TBHashTableAutoDeleteOf. */
	void deleteAll() { removeAll(true); }

	/** Get the content for the given key, or nullptr if not found. */
	void *get(uint32_t key) const;

	/** Add content with the given key.
		Returns false if out of memory. */
	bool add(uint32_t key, void *content);

	/** Remove the content with the given key. */
	void *remove(uint32_t key);

	/** Delete the content with the given key. */
	void deleteKey(uint32_t key);

	/** Rehash the table so use the given number of buckets.
		Returns false if out of memory. */
	bool rehash(uint32_t num_buckets);

	/** Return true if the hashtable itself think it's time to rehash. */
	bool needRehash() const;

	/** Get the number of buckets the hashtable itself thinks is suitable for
		the current number of items. */
	uint32_t getSuitableBucketsCount() const;

	/** Get the number of items in the hash table. */
	uint32_t getNumItems() const { return m_num_items; }

#ifdef TB_RUNTIME_DEBUG_INFO
	/** Print out some debug info about the hash table. */
	void debug();
#endif

protected:
	/** Delete the content of a item. This is called if calling DeleteAll, and must be
		implemented in a subclass that knows about the content type. */
	virtual void deleteContent(void *content) { core_assert(!"You need to subclass and implement!"); }
private:
	friend class TBHashTableIterator;
	void removeAll(bool delete_content);
	struct ITEM {
		uint32_t key;
		ITEM *next;
		void *content;
	} **m_buckets;
	uint32_t m_num_buckets;
	uint32_t m_num_items;
};

/** TBHashTableIterator is a iterator for stepping through all content stored in a TBHashTable. */
//FIX: make it safe in case the current item is removed from the hashtable
class TBHashTableIterator
{
public:
	TBHashTableIterator(TBHashTable *hash_table);
	void *getNextContent();
private:
	TBHashTable *m_hash_table;
	uint32_t m_current_bucket;
	TBHashTable::ITEM *m_current_item;
};

/** TBHashTableIteratorOf is a TBHashTableIterator which auto cast to the class type. */
template<class T>
class TBHashTableIteratorOf : private TBHashTableIterator
{
public:
	TBHashTableIteratorOf(TBHashTable *hash_table) : TBHashTableIterator(hash_table) {}
	T *getNextContent() { return (T*) TBHashTableIterator::getNextContent(); }
};

/** TBHashTableOf is a TBHashTable with the given class type as content. */
template<class T>
class TBHashTableOf : public TBHashTable
{
// FIX: Don't do public inheritance! Either inherit privately and forward, or use a private member backend!
public:
	T *get(uint32_t key) const { return (T*) TBHashTable::get(key); }
	T *remove(uint32_t key) { return (T*) TBHashTable::remove(key); }
protected:
	virtual void deleteContent(void *content) { delete (T*) content; }
};

/** TBHashTableOf is a TBHashTable with the given class type as content.
	It will delete all content automaticallt on destruction. */
template<class T>
class TBHashTableAutoDeleteOf : public TBHashTable
{
public:
	~TBHashTableAutoDeleteOf() { deleteAll(); }

	T *get(uint32_t key) const { return (T*) TBHashTable::get(key); }

protected:
	virtual void deleteContent(void *content) { delete (T*) content; }
};

} // namespace tb
