/**
 * @file
 */

#include "tb_bitmap_fragment.h"
#include "tb_renderer.h"
#include "tb_system.h"
#include "image/Image.h"

namespace tb {

int TBGetNearestPowerOfTwo(int val)
{
	int i;
	for(i = 31; i >= 0; i--)
		if ((val - 1) & (1<<i))
			break;
	return (1<<(i + 1));
}

// == TBSpaceAllocator ======================================================================================

bool TBSpaceAllocator::hasSpace(int neededW) const
{
	if (neededW > m_available_space)
		return false;
	if (isAllAvailable())
		return true;
	for (Space *fs = m_free_space_list.getFirst(); fs; fs = fs->getNext())
	{
		if (neededW <= fs->width)
			return true;
	}
	return false;
}

TBSpaceAllocator::Space *TBSpaceAllocator::allocSpace(int neededW)
{
	if (Space *available_space = getSmallestAvailableSpace(neededW))
	{
		if (Space *new_space = new Space)
		{
			new_space->x = available_space->x;
			new_space->width = neededW;
			m_used_space_list.addLast(new_space);

			// Consume the used space from the available space
			available_space->x += neededW;
			available_space->width -= neededW;
			m_available_space -= neededW;

			// Remove it if empty
			if (!available_space->width)
				m_free_space_list.doDelete(available_space);
			return new_space;
		}
	}
	return nullptr;
}

TBSpaceAllocator::Space *TBSpaceAllocator::getSmallestAvailableSpace(int neededW)
{
	core_assert(neededW > 0);

	// Add free space covering all available space if empty.
	if (!m_free_space_list.hasLinks() && isAllAvailable())
	{
		if (Space *fs = new Space)
		{
			fs->x = 0;
			fs->width = m_available_space;
			m_free_space_list.addLast(fs);
		}
	}

	// Check for the smallest space where we fit
	Space *best_fs = nullptr;
	for (Space *fs = m_free_space_list.getFirst(); fs; fs = fs->getNext())
	{
		if (neededW == fs->width)
			return fs; // It can't be better than a perfect match!
		if (neededW < fs->width)
			if (!best_fs || fs->width < best_fs->width)
				best_fs = fs;
	}
	return best_fs;
}

void TBSpaceAllocator::freeSpace(Space *space)
{
	m_used_space_list.remove(space);
	m_available_space += space->width;

	// Find where in m_free_space_list we should insert the space,
	// or which existing space we can extend.
	Space *preceeding = nullptr;
	Space *succeeding = nullptr;
	for (Space *fs = m_free_space_list.getFirst(); fs; fs = fs->getNext())
	{
		if (fs->x < space->x)
			preceeding = fs;
		if (fs->x > space->x)
		{
			succeeding = fs;
			break;
		}
	}
	if (preceeding && preceeding->x + preceeding->width == space->x)
	{
		preceeding->width += space->width;
		delete space;
	}
	else if (succeeding && succeeding->x == space->x + space->width)
	{
		succeeding->x -= space->width;
		succeeding->width += space->width;
		delete space;
	}
	else
	{
		if (preceeding)
			m_free_space_list.addAfter(space, preceeding);
		else if (succeeding)
			m_free_space_list.addBefore(space, succeeding);
		else
		{
			core_assert(!m_free_space_list.hasLinks());
			m_free_space_list.addLast(space);
		}
	}
	// Merge free spaces
	Space *fs = m_free_space_list.getFirst();
	while (fs)
	{
		Space *next_fs = fs->getNext();
		if (!next_fs)
			break;
		if (fs->x + fs->width == next_fs->x)
		{
			fs->width += next_fs->width;
			m_free_space_list.doDelete(next_fs);
			continue;
		}
		fs = next_fs;
	}

#ifdef TB_RUNTIME_DEBUG_INFO
	// Check that free space is in order
	Space *tmp = m_free_space_list.getFirst();
	int x = 0;
	while (tmp)
	{
		core_assert(tmp->x >= x);
		x = tmp->x + tmp->width;
		tmp = tmp->getNext();
	}
#endif // TB_RUNTIME_DEBUG_INFO
}

// == TBBitmapFragmentMap ===================================================================================

TBBitmapFragmentMap::TBBitmapFragmentMap()
	: m_bitmap_w(0)
	, m_bitmap_h(0)
	, m_bitmap_data(nullptr)
	, m_bitmap(nullptr)
	, m_need_update(false)
	, m_allocated_pixels(0)
{
}

bool TBBitmapFragmentMap::init(int bitmapW, int bitmapH)
{
	m_bitmap_data = new uint32_t[bitmapW * bitmapH];
	m_bitmap_w = bitmapW;
	m_bitmap_h = bitmapH;
#ifdef TB_RUNTIME_DEBUG_INFO
	if (m_bitmap_data)
		memset(m_bitmap_data, 0x88, bitmapW * bitmapH * sizeof(uint32_t));
#endif
	return m_bitmap_data ? true : false;
}

TBBitmapFragmentMap::~TBBitmapFragmentMap()
{
	delete m_bitmap;
	delete [] m_bitmap_data;
}

TBBitmapFragment *TBBitmapFragmentMap::createNewFragment(int fragW, int fragH, int dataStride, uint32_t *fragData, bool addBorder)
{
	// Finding available space works like this:
	// The map size is sliced up horizontally in rows (initially just one row covering
	// the entire map). When adding a new fragment, put it in the row with smallest height.
	// If the smallest row is empty, it may slice the row to make a even smaller row.

	// When a image is stretched up to a larger size, the filtering will read
	// pixels closest (but outside) of the src_rect. When we pack images together
	// those pixels would be read from neighbour images, so we must add border space
	// around each image to avoid artifacts. We must also fill in that border with
	// the "clamp" of the image itself so we don't get any filtering artifacts at all.
	// Allways add border except when we're using the entire map for one fragment.
	int border = 0;
	int needed_w = fragW;
	int needed_h = fragH;
	if (addBorder)
	{
		if (needed_w != m_bitmap_w || needed_h != m_bitmap_h)
		{
			border = 1;
			needed_w += 2;
			needed_h += 2;
		}
	}

	// Snap the fragments to a certain granularity. This could maybe ease the stress
	// on the space allocator when allocating & deallocating lots of small fragments.
	// I'm not sure there is any performance issue though and it would be better to
	// optimize the algorithm instead (so disabled it for now).
	//const int granularity = 8;
	//needed_w = (needed_w + granularity - 1) / granularity * granularity;
	//needed_h = (needed_h + granularity - 1) / granularity * granularity;

	if (!m_rows.getNumItems())
	{
		// Create a row covering the entire bitmap.
		TBFragmentSpaceAllocator *row;
		if (!m_rows.growIfNeeded() || !(row = new TBFragmentSpaceAllocator(0, m_bitmap_w, m_bitmap_h)))
			return nullptr;
		m_rows.add(row);
	}
	// Get the smallest row where we fit
	int best_row_index = -1;
	TBFragmentSpaceAllocator *best_row = nullptr;
	for (int i = 0; i < m_rows.getNumItems(); i++)
	{
		TBFragmentSpaceAllocator *row = m_rows[i];
		if (!best_row || row->height < best_row->height)
		{
			// This is the best row so far, if we fit
			if (needed_h <= row->height && row->hasSpace(needed_w))
			{
				best_row = row;
				best_row_index = i;
				if (needed_h == row->height)
					break; // We can't find a smaller line, so we're done
			}
		}
	}
	// Return if we're full
	if (!best_row)
		return nullptr;
	// If the row is unused, create a smaller row to only consume needed height for fragment
	if (best_row->isAllAvailable() && needed_h < best_row->height)
	{
		TBFragmentSpaceAllocator *row;
		if (!m_rows.growIfNeeded() || !(row = new TBFragmentSpaceAllocator(best_row->y + needed_h, m_bitmap_w, best_row->height - needed_h)))
			return nullptr;
		// Keep the rows sorted from top to bottom
		m_rows.add(row, best_row_index + 1);
		best_row->height = needed_h;
	}
	// Allocate the fragment and copy the fragment data into the map data.
	if (TBFragmentSpaceAllocator::Space *space = best_row->allocSpace(needed_w))
	{
		if (TBBitmapFragment *frag = new TBBitmapFragment)
		{
			frag->m_map = this;
			frag->m_row = best_row;
			frag->m_space = space;
			frag->m_rect.set(space->x + border, best_row->y + border, fragW, fragH);
			frag->m_row_height = best_row->height;
			frag->m_batch_id = 0xffffffff;
			copyData(frag, dataStride, fragData, border);
			m_need_update = true;
			m_allocated_pixels += frag->m_space->width * frag->m_row->height;
			return frag;
		}
		else
			best_row->freeSpace(space);
	}
	return nullptr;
}

void TBBitmapFragmentMap::freeFragmentSpace(TBBitmapFragment *frag)
{
	if (!frag)
		return;
	core_assert(frag->m_map == this);

#ifdef TB_RUNTIME_DEBUG_INFO
	// Debug code to clear the area in debug builds so it's easier to
	// see & debug the allocation & deallocation of fragments in maps.
	if (uint32_t *data32 = new uint32_t[frag->m_space->width * frag->m_row->height])
	{
		static int c = 0;
		memset(data32, (c++) * 32, sizeof(uint32_t) * frag->m_space->width * frag->m_row->height);
		copyData(frag, frag->m_space->width, data32, false);
		m_need_update = true;
		delete [] data32;
	}
#endif // TB_RUNTIME_DEBUG_INFO

	m_allocated_pixels -= frag->m_space->width * frag->m_row->height;
	frag->m_row->freeSpace(frag->m_space);
	frag->m_space = nullptr;
	frag->m_row_height = 0;

	// If the row is now empty, merge empty rows so larger fragments
	// have a chance of allocating the space.
	if (frag->m_row->isAllAvailable())
	{
		for (int i = 0; i < m_rows.getNumItems() - 1; i++)
		{
			core_assert(i >= 0);
			core_assert(i < m_rows.getNumItems() - 1);
			TBFragmentSpaceAllocator *row = m_rows.get(i);
			TBFragmentSpaceAllocator *next_row = m_rows.get(i + 1);
			if (row->isAllAvailable() && next_row->isAllAvailable())
			{
				row->height += next_row->height;
				m_rows.doDelete(i + 1);
				i--;
			}
		}
	}
}

void TBBitmapFragmentMap::copyData(TBBitmapFragment *frag, int dataStride, uint32_t *fragData, int border)
{
	// Copy the bitmap data
	uint32_t *dst = m_bitmap_data + frag->m_rect.x + frag->m_rect.y * m_bitmap_w;
	uint32_t *src = fragData;
	for (int i = 0; i < frag->m_rect.h; i++)
	{
		memcpy(dst, src, frag->m_rect.w * sizeof(uint32_t));
		dst += m_bitmap_w;
		src += dataStride;
	}
	// Copy the bitmap data to the border around the fragment
	if (border)
	{
		TBRect rect = frag->m_rect.expand(border, border);
		// Copy vertical edges
		dst = m_bitmap_data + rect.x + (rect.y + 1) * m_bitmap_w;
		src = fragData;
		for (int i = 0; i < frag->m_rect.h; i++)
		{
			dst[0] = src[0] & 0x00ffffff;
			dst[rect.w - 1] = src[frag->m_rect.w - 1] & 0x00ffffff;
			dst += m_bitmap_w;
			src += dataStride;
		}
		// Copy horizontal edges
		dst = m_bitmap_data + rect.x + 1 + rect.y * m_bitmap_w;
		src = fragData;
		for (int i = 0; i < frag->m_rect.w; i++)
			dst[i] = src[i] & 0x00ffffff;
		dst = m_bitmap_data + rect.x + 1 + (rect.y + rect.h - 1) * m_bitmap_w;
		src = fragData + (frag->m_rect.h - 1) * dataStride;
		for (int i = 0; i < frag->m_rect.w; i++)
			dst[i] = src[i] & 0x00ffffff;
	}
}

TBBitmap *TBBitmapFragmentMap::getBitmap(TB_VALIDATE_TYPE validateType)
{
	if (m_bitmap && validateType == TB_VALIDATE_FIRST_TIME)
		return m_bitmap;
	validateBitmap();
	return m_bitmap;
}

bool TBBitmapFragmentMap::validateBitmap()
{
	if (m_need_update)
	{
		if (m_bitmap)
			m_bitmap->setData(m_bitmap_data);
		else
			m_bitmap = g_renderer->createBitmap(m_bitmap_w, m_bitmap_h, m_bitmap_data);
		m_need_update = false;
	}
	return m_bitmap ? true : false;
}

void TBBitmapFragmentMap::deleteBitmap()
{
	delete m_bitmap;
	m_bitmap = nullptr;
	m_need_update = true;
}

// == TBBitmapFragmentManager =============================================================================

TBBitmapFragmentManager::TBBitmapFragmentManager()
	: m_num_maps_limit(0)
	, m_add_border(false)
	, m_default_map_w(512)
	, m_default_map_h(512)
{
}

TBBitmapFragmentManager::~TBBitmapFragmentManager()
{
	clear();
}

TBBitmapFragment *TBBitmapFragmentManager::getFragmentFromFile(const char *filename, bool dedicatedMap)
{
	TBID id(filename);

	// If we already have a fragment for this filename, return that
	TBBitmapFragment *frag = m_fragments.get(id);
	if (frag)
		return frag;

	// Load the file
	const image::ImagePtr& img = image::loadImage(filename, false);
	return createNewFragment(id, dedicatedMap, img->width(), img->height(), img->width(), (uint32_t*)img->data());
}

TBBitmapFragment *TBBitmapFragmentManager::createNewFragment(const TBID &id, bool dedicatedMap,
															 int dataW, int dataH, int dataStride,
															 uint32_t *data)
{
	core_assert(!getFragment(id));

	TBBitmapFragment *frag = nullptr;

	// Create a fragment in any of the fragment maps. Doing it in the reverse order
	// would be faster since it's most likely to succeed, but we want to maximize
	// the amount of fragments per map, so do it in the creation order.
	if (!dedicatedMap)
	{
		for (int i = 0; i < m_fragment_maps.getNumItems(); i++)
		{
			if ((frag = m_fragment_maps[i]->createNewFragment(dataW, dataH, dataStride, data, m_add_border)))
				break;
		}
	}
	// If we couldn't create the fragment in any map, create a new map where we know it will fit.
	bool allow_another_map = (m_num_maps_limit == 0 || m_fragment_maps.getNumItems() < m_num_maps_limit);
	if (!frag && allow_another_map && m_fragment_maps.growIfNeeded())
	{
		int po2w = TBGetNearestPowerOfTwo(Max(dataW, m_default_map_w));
		int po2h = TBGetNearestPowerOfTwo(Max(dataH, m_default_map_h));
		if (dedicatedMap)
		{
			po2w = TBGetNearestPowerOfTwo(dataW);
			po2h = TBGetNearestPowerOfTwo(dataH);
		}
		TBBitmapFragmentMap *fm = new TBBitmapFragmentMap();
		if (fm && fm->init(po2w, po2h))
		{
			m_fragment_maps.add(fm);
			frag = fm->createNewFragment(dataW, dataH, dataStride, data, m_add_border);
		}
		else
			delete fm;
	}
	// Finally, add the new fragment to the hash.
	if (frag && m_fragments.add(id, frag))
	{
		frag->m_id = id;
		return frag;
	}
	delete frag;
	return nullptr;
}

void TBBitmapFragmentManager::freeFragment(TBBitmapFragment *frag)
{
	if (frag)
	{
		g_renderer->flushBitmapFragment(frag);

		TBBitmapFragmentMap *map = frag->m_map;
		frag->m_map->freeFragmentSpace(frag);
		m_fragments.deleteKey(frag->m_id);

		// If the map is now empty, delete it.
		if (map->m_allocated_pixels == 0)
			m_fragment_maps.doDelete(m_fragment_maps.find(map));
	}
}

TBBitmapFragment *TBBitmapFragmentManager::getFragment(const TBID &id) const
{
	return m_fragments.get(id);
}

void TBBitmapFragmentManager::clear()
{
	m_fragment_maps.deleteAll();
	m_fragments.deleteAll();
}

bool TBBitmapFragmentManager::validateBitmaps()
{
	bool success = true;
	for (int i = 0; i < m_fragment_maps.getNumItems(); i++)
		if (!m_fragment_maps[i]->validateBitmap())
			success = false;
	return success;
}

void TBBitmapFragmentManager::deleteBitmaps()
{
	for (int i = 0; i < m_fragment_maps.getNumItems(); i++)
		m_fragment_maps[i]->deleteBitmap();
}

void TBBitmapFragmentManager::setNumMapsLimit(int numMapsLimit)
{
	m_num_maps_limit = numMapsLimit;
}

void TBBitmapFragmentManager::setDefaultMapSize(int w, int h)
{
	core_assert(TBGetNearestPowerOfTwo(w) == w);
	core_assert(TBGetNearestPowerOfTwo(h) == h);
	m_default_map_w = w;
	m_default_map_h = h;
}

int TBBitmapFragmentManager::getUseRatio() const
{
	int used = 0;
	int total = 0;
	for (int i = 0; i < m_fragment_maps.getNumItems(); i++)
	{
		used += m_fragment_maps[i]->m_allocated_pixels;
		total += m_fragment_maps[i]->m_bitmap_w * m_fragment_maps[i]->m_bitmap_h;
	}
	return total ? (used * 100) / total : 0;
}

#ifdef TB_RUNTIME_DEBUG_INFO
void TBBitmapFragmentManager::debug()
{
	int x = 0;
	for (int i = 0; i < m_fragment_maps.getNumItems(); i++)
	{
		TBBitmapFragmentMap *fm = m_fragment_maps[i];
		if (TBBitmap *bitmap = fm->getBitmap())
			g_renderer->drawBitmap(TBRect(x, 0, fm->m_bitmap_w, fm->m_bitmap_h), TBRect(0, 0, fm->m_bitmap_w, fm->m_bitmap_h), bitmap);
		x += fm->m_bitmap_w + 5;
	}
}
#endif // TB_RUNTIME_DEBUG_INFO

} // namespace tb
