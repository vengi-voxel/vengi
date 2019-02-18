/**
 * @file
 */

#pragma once

#include "tb_core.h"
#include "tb_geometry.h"
#include "tb_hashtable.h"
#include "tb_id.h"
#include "tb_linklist.h"
#include "tb_list.h"

namespace tb {

class TBBitmapFragment;
class TBBitmap;

/** Return the nearest power of two from val.
	F.ex 110 -> 128, 256->256, 257->512 etc. */
int TBGetNearestPowerOfTwo(int val);

/** Allocator of space out of a given available space. */
class TBSpaceAllocator {
public:
	/** A chunk of space */
	class Space : public TBLinkOf<Space> {
	public:
		int x, width;
	};

	TBSpaceAllocator(int availableSpace) : m_available_space(availableSpace) {
	}

	/** Return true if no allocations are currently live using this allocator. */
	bool isAllAvailable() const {
		return !m_used_space_list.hasLinks();
	}

	/** Return true if the given width is currently available. */
	bool hasSpace(int needed_w) const;

	/** Allocate the given space and return the Space, or nullptr on error. */
	Space *allocSpace(int needed_w);

	/** Free the given space so it is available for new allocations. */
	void freeSpace(Space *space);

private:
	Space *getSmallestAvailableSpace(int needed_w);
	int m_available_space;
	TBLinkListAutoDeleteOf<Space> m_free_space_list;
	TBLinkListAutoDeleteOf<Space> m_used_space_list;
};

/** Allocates space for TBBitmapFragment in a row (used in TBBitmapFragmentMap). */
class TBFragmentSpaceAllocator : public TBSpaceAllocator {
public:
	TBFragmentSpaceAllocator(int y, int width, int height) : TBSpaceAllocator(width), y(y), height(height) {
	}

	int y, height;
};

/** Specify when the bitmap should be validated when calling TBBitmapFragmentMap::getBitmap. */
enum TB_VALIDATE_TYPE {

	/** Always validate the bitmap (The bitmap is updated if needed) */
	TB_VALIDATE_ALWAYS,

	/** Only validate if the bitmap does not yet exist (Make sure there is
		a valid bitmap pointer, but the data is not necessarily updated) */
	TB_VALIDATE_FIRST_TIME
};

/** TBBitmapFragmentMap is used to pack multiple bitmaps into a single TBBitmap.
	When initialized (in a size suitable for a TBBitmap) it also creates a software buffer
	that will make up the TBBitmap when all fragments have been added. */
class TBBitmapFragmentMap {
public:
	TBBitmapFragmentMap();
	~TBBitmapFragmentMap();

	/** Initialize the map with the given size. The size should be a power of two since
		it will be used to create a TBBitmap (texture memory). */
	bool init(int bitmapW, int bitmapH);

	/** Create a new fragment with the given size and data in this map.
		Returns nullptr if there is not enough room in this map or on any other fail. */
	TBBitmapFragment *createNewFragment(int fragW, int fragH, int dataStride, uint32_t *fragData, bool addBorder);

	/** Free up the space used by the given fragment, so that other fragments can take its place. */
	void freeFragmentSpace(TBBitmapFragment *frag);

	/** Return the bitmap for this map.
		By default, the bitmap is validated if needed before returning (See TB_VALIDATE_TYPE) */
	TBBitmap *getBitmap(TB_VALIDATE_TYPE validate_type = TB_VALIDATE_ALWAYS);

private:
	friend class TBBitmapFragmentManager;
	bool validateBitmap();
	void deleteBitmap();
	void copyData(TBBitmapFragment *frag, int dataStride, uint32_t *fragData, int border);
	TBListAutoDeleteOf<TBFragmentSpaceAllocator> m_rows;
	int m_bitmap_w, m_bitmap_h;
	uint32_t *m_bitmap_data;
	TBBitmap *m_bitmap;
	bool m_need_update;
	int m_allocated_pixels;
};

/** TBBitmapFragment represents a sub part of a TBBitmap.
	It's owned by TBBitmapFragmentManager which pack multiple
	TBBitmapFragment within TBBitmaps to reduce texture switching. */
class TBBitmapFragment {
public:
	/** Return the width of the bitmap fragment. */
	int width() const {
		return m_rect.w;
	}

	/** Return the height of the bitmap fragment. */
	int height() const {
		return m_rect.h;
	}

	/** Return the bitmap for this fragment.
		By default, the bitmap is validated if needed before returning (See TB_VALIDATE_TYPE) */
	TBBitmap *getBitmap(TB_VALIDATE_TYPE validateType = TB_VALIDATE_ALWAYS) {
		return m_map->getBitmap(validateType);
	}

	/** Return the height allocated to this fragment. This may be larger than Height() depending
		of the internal allocation of fragments in a map. It should rarely be used. */
	int getAllocatedHeight() const {
		return m_row_height;
	}

public:
	TBBitmapFragmentMap *m_map;
	TBRect m_rect;
	TBFragmentSpaceAllocator *m_row;
	TBFragmentSpaceAllocator::Space *m_space;
	TBID m_id;
	int m_row_height;

	/** This uint32 is reserved for batching renderer backends. It's not used
		internally, but always initialized to 0xffffffff for all new fragments. */
	uint32_t m_batch_id;
};

/** TBBitmapFragmentManager manages loading bitmaps of arbitrary size,
	pack as many of them into as few TBBitmap as possible.

	It also makes sure that only one instance of each file is loaded,
	so f.ex loading "foo.png" many times still load and allocate one
	TBBitmapFragment. */
class TBBitmapFragmentManager {
public:
	TBBitmapFragmentManager();
	~TBBitmapFragmentManager();

	/** Set to true if a 1px border should be added to new fragments so stretched
		drawing won't get filtering artifacts at the edges (default is disabled). */
	void setAddBorder(bool addBorder) {
		m_add_border = addBorder;
	}
	bool getAddBorder() const {
		return m_add_border;
	}

	/** Get the fragment with the given image filename. If it's not already loaded,
		it will be loaded into a new fragment with the filename as id.
		returns nullptr on fail. */
	TBBitmapFragment *getFragmentFromFile(const char *filename, bool dedicated_map);

	/** Get the fragment with the given id, or nullptr if it doesn't exist. */
	TBBitmapFragment *getFragment(const TBID &id) const;

	/** Create a new fragment from the given data.
		@param id The id that should be used to identify the fragment.
		@param dedicated_map if true, it will get a dedicated map.
		@param data_w the width of the data.
		@param data_h the height of the data.
		@param data_stride the number of pixels in a row of the input data.
		@param data pointer to the data in BGRA32 format. */
	TBBitmapFragment *createNewFragment(const TBID &id, bool dedicatedMap, int dataW, int dataH, int dataStride,
										uint32_t *data);

	/** Delete the given fragment and free the space it used in its map,
		so that other fragments can take its place. */
	void freeFragment(TBBitmapFragment *frag);

	/** Clear all loaded bitmaps and all created bitmap fragments and maps.
		After this call, do not keep any pointers to any TBBitmapFragment created
		by this fragment manager. */
	void clear();

	/** Validate bitmaps on fragment maps that has changed. */
	bool validateBitmaps();

	/** Delete all bitmaps in all fragment maps in this manager.
		The bitmaps will be recreated automatically when needed, or when
		calling ValidateBitmaps. You do not need to call this, except when
		the context is lost and all bitmaps must be forgotten. */
	void deleteBitmaps();

	/** Get number of fragment maps that is currently used. */
	int getNumMaps() const {
		return m_fragment_maps.getNumItems();
	}

	/** Set the number of maps (TBBitmaps) this manager should be allowed to create.
		If a new fragment can't fit into any existing bitmap and the limit is reached,
		the fragment creation will fail. Set to 0 for unlimited (default). */
	void setNumMapsLimit(int num_maps_limit);

	/** Set the default size of new fragment maps. These must be power of two. */
	void setDefaultMapSize(int w, int h);

	/** Get the amount (in percent) of space that is currently occupied by all maps
		in this fragment manager. */
	int getUseRatio() const;
#ifdef TB_RUNTIME_DEBUG_INFO
	/** Render the maps on screen, to analyze fragment positioning. */
	void debug();
#endif
private:
	TBListOf<TBBitmapFragmentMap> m_fragment_maps;
	TBHashTableOf<TBBitmapFragment> m_fragments;
	int m_num_maps_limit;
	bool m_add_border;
	int m_default_map_w;
	int m_default_map_h;
};

} // namespace tb
