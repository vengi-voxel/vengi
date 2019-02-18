/**
 * @file
 */

#include "tb_geometry.h"
#include "core/Assert.h"

namespace tb {

bool TBRect::intersects(const TBRect &rect) const {
	if (isEmpty() || rect.isEmpty())
		return false;
	if (x + w > rect.x && x < rect.x + rect.w && y + h > rect.y && y < rect.y + rect.h)
		return true;
	return false;
}

TBRect TBRect::moveIn(const TBRect &boundingRect) const {
	return TBRect(ClampClipMax(x, boundingRect.x, boundingRect.x + boundingRect.w - w),
				  ClampClipMax(y, boundingRect.y, boundingRect.y + boundingRect.h - h), w, h);
}

TBRect TBRect::centerIn(const TBRect &boundingRect) const {
	return TBRect(boundingRect.x + (boundingRect.w - w) / 2, boundingRect.y + (boundingRect.h - h) / 2, w, h);
}

TBRect TBRect::join(const TBRect &rect) const {
	core_assert(!isInsideOut());
	core_assert(!rect.isInsideOut());

	if (isEmpty())
		return rect;
	if (rect.isEmpty())
		return *this;

	int minx = Min(x, rect.x);
	int miny = Min(y, rect.y);
	int maxx = x + w > rect.x + rect.w ? x + w : rect.x + rect.w;
	int maxy = y + h > rect.y + rect.h ? y + h : rect.y + rect.h;
	return TBRect(minx, miny, maxx - minx, maxy - miny);
}

TBRect TBRect::clip(const TBRect &clipRect) const {
	core_assert(!clipRect.isInsideOut());
	TBRect tmp;
	if (!intersects(clipRect))
		return tmp;
	tmp.x = Max(x, clipRect.x);
	tmp.y = Max(y, clipRect.y);
	tmp.w = Min(x + w, clipRect.x + clipRect.w) - tmp.x;
	tmp.h = Min(y + h, clipRect.y + clipRect.h) - tmp.y;
	return tmp;
}

// == TBRegion ==========================================================================

TBRegion::TBRegion() : m_rects(nullptr), m_num_rects(0), m_capacity(0) {
}

TBRegion::~TBRegion() {
	removeAll(true);
}

void TBRegion::removeRect(int index) {
	core_assert(index >= 0 && index < m_num_rects);
	for (int i = index; i < m_num_rects - 1; i++)
		m_rects[i] = m_rects[i + 1];
	m_num_rects--;
}

void TBRegion::removeRectFast(int index) {
	core_assert(index >= 0 && index < m_num_rects);
	m_rects[index] = m_rects[--m_num_rects];
}

void TBRegion::removeAll(bool freeMemory) {
	m_num_rects = 0;
	if (freeMemory) {
		delete[] m_rects;
		m_rects = nullptr;
		m_capacity = 0;
	}
}

bool TBRegion::set(const TBRect &rect) {
	removeAll();
	return addRect(rect, false);
}

bool TBRegion::growIfNeeded() {
	if (m_num_rects == m_capacity) {
		int new_m_capacity = Clamp(4, m_capacity * 2, 1024);
		TBRect *new_rects = new TBRect[new_m_capacity];
		if (!new_rects)
			return false;
		if (m_rects)
			memmove(new_rects, m_rects, sizeof(TBRect) * m_capacity);
		delete[] m_rects;
		m_rects = new_rects;
		m_capacity = new_m_capacity;
	}
	return true;
}

bool TBRegion::addRect(const TBRect &rect, bool coalesce) {
	if (coalesce) {
		// If the rect can coalesce with any existing rect,
		// just replace it with the union of both, doing coalesce
		// check again recursively.
		// Searching backwards is most likely to give a hit quicker
		// in many usage scenarios.
		for (int i = m_num_rects - 1; i >= 0; i--) {
			if ( // Can coalesce vertically
				(rect.x == m_rects[i].x && rect.w == m_rects[i].w &&
				 (rect.y == m_rects[i].y + m_rects[i].h ||
				  rect.y + rect.h == m_rects[i].y)) || // Can coalesce horizontally
				(rect.y == m_rects[i].y && rect.h == m_rects[i].h &&
				 (rect.x == m_rects[i].x + m_rects[i].w || rect.x + rect.w == m_rects[i].x))) {
				TBRect union_rect = m_rects[i].join(rect);
				removeRectFast(i);
				return addRect(union_rect, true);
			}
		}
	}

	if (!growIfNeeded())
		return false;
	m_rects[m_num_rects++] = rect;
	return true;
}

bool TBRegion::includeRect(const TBRect &rect) {
	for (int i = 0; i < m_num_rects; i++) {
		if (rect.intersects(m_rects[i])) {
			// Make a region containing the non intersecting parts and then include
			// those recursively (they might still intersect some other part of the region).
			TBRegion inclusion_region;
			if (!inclusion_region.addExcludingRects(rect, m_rects[i], false))
				return false;
			for (int j = 0; j < inclusion_region.m_num_rects; j++) {
				if (!includeRect(inclusion_region.m_rects[j]))
					return false;
			}
			return true;
		}
	}
	// Now we know that the rect can be added without overlap.
	// Add it with coalesce checking to keep the number of rects down.
	return addRect(rect, true);
}

bool TBRegion::excludeRect(const TBRect &excludeRect) {
	int num_rects_to_check = m_num_rects;
	for (int i = 0; i < num_rects_to_check; i++) {
		if (m_rects[i].intersects(excludeRect)) {
			// Remove the existing rectangle we found we intersect
			// and add the pieces we don't intersect. New rects
			// will be added at the end of the list, so we can decrease
			// num_rects_to_check.
			TBRect rect = m_rects[i];
			removeRect(i);
			num_rects_to_check--;
			i--;

			if (!addExcludingRects(rect, excludeRect, true))
				return false;
		}
	}
	return true;
}

bool TBRegion::addExcludingRects(const TBRect &rect, const TBRect &excludeRect, bool coalesce) {
	core_assert(rect.intersects(excludeRect));
	TBRect remove = excludeRect.clip(rect);

	if (remove.y > rect.y)
		if (!addRect(TBRect(rect.x, rect.y, rect.w, remove.y - rect.y), coalesce))
			return false;
	if (remove.x > rect.x)
		if (!addRect(TBRect(rect.x, remove.y, remove.x - rect.x, remove.h), coalesce))
			return false;
	if (remove.x + remove.w < rect.x + rect.w)
		if (!addRect(TBRect(remove.x + remove.w, remove.y, rect.x + rect.w - (remove.x + remove.w), remove.h),
					 coalesce))
			return false;
	if (remove.y + remove.h < rect.y + rect.h)
		if (!addRect(TBRect(rect.x, remove.y + remove.h, rect.w, rect.y + rect.h - (remove.y + remove.h)), coalesce))
			return false;
	return true;
}

const TBRect &TBRegion::getRect(int index) const {
	core_assert(index >= 0 && index < m_num_rects);
	return m_rects[index];
}

} // namespace tb
