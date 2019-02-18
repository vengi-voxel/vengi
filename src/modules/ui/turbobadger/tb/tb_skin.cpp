/**
 * @file
 */

#include "tb_skin.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "tb_node_tree.h"
#include "tb_system.h"
#include "tb_tempbuffer.h"

namespace tb {

// == Util functions ==========================================================

/*TB_TEXT_ALIGN StringToTextAlign(const char *align_str)
{
	TB_TEXT_ALIGN align = TB_TEXT_ALIGN_CENTER;
	if (SDL_strstr(state_str, "left"))		align = TB_TEXT_ALIGN_LEFT;
	if (SDL_strstr(state_str, "center"))	align = TB_TEXT_ALIGN_CENTER;
	if (SDL_strstr(state_str, "right"))		align = TB_TEXT_ALIGN_RIGHT;
	return state;
}*/

SKIN_STATE stringToState(const char *stateStr) {
	SKIN_STATE state = SKIN_STATE_NONE;
	if (SDL_strstr(stateStr, "all"))
		state |= SKIN_STATE_ALL;
	if (SDL_strstr(stateStr, "disabled"))
		state |= SKIN_STATE_DISABLED;
	if (SDL_strstr(stateStr, "focused"))
		state |= SKIN_STATE_FOCUSED;
	if (SDL_strstr(stateStr, "pressed"))
		state |= SKIN_STATE_PRESSED;
	if (SDL_strstr(stateStr, "selected"))
		state |= SKIN_STATE_SELECTED;
	if (SDL_strstr(stateStr, "hovered"))
		state |= SKIN_STATE_HOVERED;
	return state;
}

SKIN_ELEMENT_TYPE stringToType(const char *typeStr) {
	if (SDL_strcmp(typeStr, "StretchBox") == 0)
		return SKIN_ELEMENT_TYPE_STRETCH_BOX;
	else if (SDL_strcmp(typeStr, "Image") == 0)
		return SKIN_ELEMENT_TYPE_IMAGE;
	else if (SDL_strcmp(typeStr, "Stretch Image") == 0)
		return SKIN_ELEMENT_TYPE_STRETCH_IMAGE;
	else if (SDL_strcmp(typeStr, "Tile") == 0)
		return SKIN_ELEMENT_TYPE_TILE;
	else if (SDL_strcmp(typeStr, "StretchBorder") == 0)
		return SKIN_ELEMENT_TYPE_STRETCH_BORDER;
	Log::debug("Skin error: Unknown skin type!");
	return SKIN_ELEMENT_TYPE_STRETCH_BOX;
}

TBSkinCondition::TARGET stringToTarget(const char *targetStr) {
	if (SDL_strcmp(targetStr, "this") == 0)
		return TBSkinCondition::TARGET_THIS;
	else if (SDL_strcmp(targetStr, "parent") == 0)
		return TBSkinCondition::TARGET_PARENT;
	else if (SDL_strcmp(targetStr, "ancestors") == 0)
		return TBSkinCondition::TARGET_ANCESTORS;
	else if (SDL_strcmp(targetStr, "prev sibling") == 0)
		return TBSkinCondition::TARGET_PREV_SIBLING;
	else if (SDL_strcmp(targetStr, "next sibling") == 0)
		return TBSkinCondition::TARGET_NEXT_SIBLING;
	Log::debug("Skin error: Unknown target in condition!");
	return TBSkinCondition::TARGET_THIS;
}

TBSkinCondition::PROPERTY stringToProperty(const char *propStr) {
	if (SDL_strcmp(propStr, "skin") == 0)
		return TBSkinCondition::PROPERTY_SKIN;
	else if (SDL_strcmp(propStr, "window active") == 0)
		return TBSkinCondition::PROPERTY_WINDOW_ACTIVE;
	else if (SDL_strcmp(propStr, "axis") == 0)
		return TBSkinCondition::PROPERTY_AXIS;
	else if (SDL_strcmp(propStr, "align") == 0)
		return TBSkinCondition::PROPERTY_ALIGN;
	else if (SDL_strcmp(propStr, "id") == 0)
		return TBSkinCondition::PROPERTY_ID;
	else if (SDL_strcmp(propStr, "state") == 0)
		return TBSkinCondition::PROPERTY_STATE;
	else if (SDL_strcmp(propStr, "value") == 0)
		return TBSkinCondition::PROPERTY_VALUE;
	else if (SDL_strcmp(propStr, "hover") == 0)
		return TBSkinCondition::PROPERTY_HOVER;
	else if (SDL_strcmp(propStr, "capture") == 0)
		return TBSkinCondition::PROPERTY_CAPTURE;
	else if (SDL_strcmp(propStr, "focus") == 0)
		return TBSkinCondition::PROPERTY_FOCUS;
	return TBSkinCondition::PROPERTY_CUSTOM;
}

// == TBSkinCondition =======================================================

TBSkinCondition::TBSkinCondition(TARGET target, PROPERTY prop, const TBID &customProp, const TBID &value, TEST test)
	: m_target(target), m_test(test) {
	m_info.prop = prop;
	m_info.custom_prop = customProp;
	m_info.value = value;
}

bool TBSkinCondition::getCondition(TBSkinConditionContext &context) const {
	const bool equal = context.getCondition(m_target, m_info);
	return equal == (m_test == TEST_EQUAL);
}

// == TBSkin ================================================================

TBSkin::TBSkin()
	: m_listener(nullptr), m_color_frag(nullptr), m_default_disabled_opacity(0.3f), m_default_placeholder_opacity(0.2f),
	  m_default_spacing(0) {
	g_renderer->addListener(this);

	// Avoid filtering artifacts at edges when we draw fragments stretched.
	m_frag_manager.setAddBorder(true);
}

bool TBSkin::load(const char *skinFile, const char *overrideSkinFile) {
	if (!loadInternal(skinFile))
		return false;
	if (overrideSkinFile && !loadInternal(overrideSkinFile))
		return false;
	return reloadBitmaps();
}

bool TBSkin::loadInternal(const char *skinFile) {
	TBNode node;
	if (!node.readFile(skinFile))
		return false;

	TBTempBuffer skin_path;
	if (!skin_path.appendPath(skinFile))
		return false;

	if (node.getNode("description")) {
		// Check which DPI mode the dimension converter should use.
		// The base-dpi is the dpi in which the padding, spacing (and so on)
		// is specified in. If the skin supports a different DPI that is
		// closer to the screen DPI, all such dimensions will be scaled.
		int base_dpi = node.getValueInt("description>base-dpi", 96);
		int supported_dpi = base_dpi;
		if (TBNode *supported_dpi_node = node.getNode("description>supported-dpi")) {
			core_assert(supported_dpi_node->getValue().isArray() ||
						supported_dpi_node->getValue().getInt() == base_dpi);
			if (TBValueArray *arr = supported_dpi_node->getValue().getArray()) {
				int screen_dpi = TBSystem::getDPI();
				int best_supported_dpi = 0;
				for (int i = 0; i < arr->getLength(); i++) {
					int candidate_dpi = arr->getValue(i)->getInt();
					if (!best_supported_dpi || Abs(candidate_dpi - screen_dpi) < Abs(best_supported_dpi - screen_dpi))
						best_supported_dpi = candidate_dpi;
				}
				supported_dpi = best_supported_dpi;
			}
		}
		m_dim_conv.setDPI(base_dpi, supported_dpi);
	}

	// Read skin constants
	if (const char *color = node.getValueString("defaults>text-color", nullptr))
		m_default_text_color.setFromString(color, SDL_strlen(color));
	m_default_disabled_opacity = node.getValueFloat("defaults>disabled>opacity", m_default_disabled_opacity);
	m_default_placeholder_opacity = node.getValueFloat("defaults>placeholder>opacity", m_default_placeholder_opacity);
	m_default_spacing = getPxFromNode(node.getNode("defaults>spacing"), m_default_spacing);

	// Iterate through all elements nodes and add skin elements or patch already
	// existing elements.
	TBNode *elements = node.getNode("elements");
	if (elements) {
		TBNode *n = elements->getFirstChild();
		while (n) {
			// If we have a "clone" node, clone all children from that node
			// into this node.
			while (TBNode *clone = n->getNode("clone")) {
				n->remove(clone);

				TBNode *clone_source = elements->getNode(clone->getValue().getString());
				if (clone_source)
					n->cloneChildren(clone_source);

				delete clone;
			}

			// If the skin element already exist, we will call Load on it again.
			// This will patch the element with any new data from the node.
			TBID element_id(n->getName());
			TBSkinElement *e = getSkinElement(element_id);
			if (!e) {
				e = new TBSkinElement;
				if (!e)
					return false;
				m_elements.add(element_id, e);
			}

			e->load(n, this, skin_path.getData());
			if (m_listener)
				m_listener->onSkinElementLoaded(this, e, n);

			n = n->getNext();
		}
	}
	return true;
}

void TBSkin::unloadBitmaps() {
	// Unset all bitmap pointers.
	TBHashTableIteratorOf<TBSkinElement> it(&m_elements);
	while (TBSkinElement *element = it.getNextContent())
		element->bitmap = nullptr;

	// Clear all fragments and bitmaps.
	m_frag_manager.clear();
	m_color_frag = nullptr;
}

bool TBSkin::reloadBitmaps() {
	unloadBitmaps();
	bool success = reloadBitmapsInternal();
	// Create all bitmaps for the bitmap fragment maps
	if (success)
		success = m_frag_manager.validateBitmaps();

#ifdef TB_RUNTIME_DEBUG_INFO
	Log::debug("Skin loaded using %d bitmaps.", m_frag_manager.getNumMaps());
#endif
	return success;
}

bool TBSkin::reloadBitmapsInternal() {
	// Load all bitmap files into new bitmap fragments.
	TBTempBuffer filename_dst_DPI;
	bool success = true;
	TBHashTableIteratorOf<TBSkinElement> it(&m_elements);
	while (TBSkinElement *element = it.getNextContent()) {
		if (!element->bitmap_file.isEmpty()) {
			core_assert(!element->bitmap);

			// FIX: dedicated_map is not needed for all backends (only deprecated fixed function GL)
			bool dedicated_map = element->type == SKIN_ELEMENT_TYPE_TILE;

			// Try to load bitmap fragment in the destination DPI (F.ex "foo.png" becomes "foo@192.png")
			int bitmap_dpi = m_dim_conv.getSrcDPI();
			if (m_dim_conv.needConversion()) {
				m_dim_conv.getDstDPIFilename(element->bitmap_file, &filename_dst_DPI);
				element->bitmap = m_frag_manager.getFragmentFromFile(filename_dst_DPI.getData(), dedicated_map);
				if (element->bitmap)
					bitmap_dpi = m_dim_conv.getDstDPI();
			}
			element->setBitmapDPI(m_dim_conv, bitmap_dpi);

			// If we still have no bitmap fragment, load from default file.
			if (!element->bitmap)
				element->bitmap = m_frag_manager.getFragmentFromFile(element->bitmap_file, dedicated_map);

			if (!element->bitmap)
				success = false;
		}
	}
	// Create fragment used for color fills. Use 2x2px and inset source rect to center 0x0
	// to avoid filtering artifacts.
	uint32_t data[4] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
	m_color_frag = m_frag_manager.createNewFragment(TBID((uint32_t)0), false, 2, 2, 2, data);
	m_color_frag->m_rect = m_color_frag->m_rect.shrink(1, 1);
	return success;
}

TBSkin::~TBSkin() {
	g_renderer->removeListener(this);
}

TBSkinElement *TBSkin::getSkinElement(const TBID &skinId) const {
	if (!skinId)
		return nullptr;
	return m_elements.get(skinId);
}

TBSkinElement *TBSkin::getSkinElementStrongOverride(const TBID &skinId, SKIN_STATE state,
													TBSkinConditionContext &context) const {
	if (TBSkinElement *skin_element = getSkinElement(skinId)) {
		// Avoid eternal recursion when overrides refer to elements referring back.
		if (skin_element->is_getting)
			return nullptr;
		skin_element->is_getting = true;

		// Check if there's any strong overrides for this element with the given state.
		TBSkinElementState *override_state = skin_element->m_strong_override_elements.getStateElement(state, context);
		if (override_state) {
			if (TBSkinElement *override_element =
					getSkinElementStrongOverride(override_state->element_id, state, context)) {
				skin_element->is_getting = false;
				return override_element;
			}
		}

		skin_element->is_getting = false;
		return skin_element;
	}
	return nullptr;
}

TBSkinElement *TBSkin::paintSkin(const TBRect &dstRect, const TBID &skinId, SKIN_STATE state,
								 TBSkinConditionContext &context) {
	return paintSkin(dstRect, getSkinElement(skinId), state, context);
}

TBSkinElement *TBSkin::paintSkin(const TBRect &dstRect, TBSkinElement *element, SKIN_STATE state,
								 TBSkinConditionContext &context) {
	if (!element || element->is_painting)
		return nullptr;

	// Avoid potential endless recursion in evil skins
	element->is_painting = true;

	// Return the override if we have one.
	TBSkinElement *return_element = element;

#ifdef TB_RUNTIME_DEBUG_INFO
	bool paint_error_highlight = false;
#endif

	// If there's any override for this state, paint it.
	TBSkinElementState *override_state = element->m_override_elements.getStateElement(state, context);
	if (override_state) {
		if (TBSkinElement *used_override = paintSkin(dstRect, override_state->element_id, state, context))
			return_element = used_override;
		else {
#ifdef TB_RUNTIME_DEBUG_INFO
			paint_error_highlight = true;
#endif
			Log::debug("Skin error: The skin references a missing element, or has a reference loop!");
			// Fall back to the standard skin.
			override_state = nullptr;
		}
	}

	// If there was no override, paint the standard skin element.
	if (!override_state)
		paintElement(dstRect, element);

	// Paint all child elements that match the state (or should be painted for all states)
	if (element->m_child_elements.hasStateElements()) {
		const TBSkinElementState *state_element = element->m_child_elements.getFirstElement();
		while (state_element) {
			if (state_element->isMatch(state, context))
				paintSkin(dstRect, state_element->element_id, state_element->state & state, context);
			state_element = state_element->getNext();
		}
	}

#ifdef TB_RUNTIME_DEBUG_INFO
	// Paint ugly rectangles on invalid skin elements in debug builds.
	if (paint_error_highlight)
		g_tb_skin->paintRect(dstRect.expand(1, 1), TBColor(255, 205, 0), 1);
	if (paint_error_highlight)
		g_tb_skin->paintRect(dstRect.shrink(1, 1), TBColor(255, 0, 0), 1);
#endif

	element->is_painting = false;
	return return_element;
}

void TBSkin::paintSkinOverlay(const TBRect &dstRect, TBSkinElement *element, SKIN_STATE state,
							  TBSkinConditionContext &context) {
	if (!element || element->is_painting)
		return;

	// Avoid potential endless recursion in evil skins
	element->is_painting = true;

	// Paint all overlay elements that matches the state (or should be painted for all states)
	const TBSkinElementState *state_element = element->m_overlay_elements.getFirstElement();
	while (state_element) {
		if (state_element->isMatch(state, context))
			paintSkin(dstRect, state_element->element_id, state_element->state & state, context);
		state_element = state_element->getNext();
	}

	element->is_painting = false;
}

void TBSkin::paintElement(const TBRect &dstRect, TBSkinElement *element) {
	paintElementBGColor(dstRect, element);
	if (!element->bitmap)
		return;
	if (element->type == SKIN_ELEMENT_TYPE_IMAGE)
		paintElementImage(dstRect, element);
	else if (element->type == SKIN_ELEMENT_TYPE_TILE)
		paintElementTile(dstRect, element);
	else if (element->type == SKIN_ELEMENT_TYPE_STRETCH_IMAGE || element->cut == 0)
		paintElementStretchImage(dstRect, element);
	else if (element->type == SKIN_ELEMENT_TYPE_STRETCH_BORDER)
		paintElementStretchBox(dstRect, element, false);
	else
		paintElementStretchBox(dstRect, element, true);
}

TBRect TBSkin::getFlippedRect(const TBRect &srcRect, TBSkinElement *element) const {
	// Turning the source rect "inside out" will flip the result when rendered.
	TBRect tmp_rect = srcRect;
	if (element->flip_x) {
		tmp_rect.x += tmp_rect.w;
		tmp_rect.w = -tmp_rect.w;
	}
	if (element->flip_y) {
		tmp_rect.y += tmp_rect.h;
		tmp_rect.h = -tmp_rect.h;
	}
	return tmp_rect;
}

void TBSkin::paintRect(const TBRect &dstRect, const TBColor &color, int thickness) {
	if (dstRect.w < thickness * 2 || dstRect.h < thickness * 2) {
		paintRectFill(dstRect, color);
		return;
	}
	// Top
	paintRectFill(TBRect(dstRect.x, dstRect.y, dstRect.w, thickness), color);
	// Bottom
	paintRectFill(TBRect(dstRect.x, dstRect.y + dstRect.h - thickness, dstRect.w, thickness), color);
	// Left
	paintRectFill(TBRect(dstRect.x, dstRect.y + thickness, thickness, dstRect.h - thickness * 2), color);
	// Right
	paintRectFill(
		TBRect(dstRect.x + dstRect.w - thickness, dstRect.y + thickness, thickness, dstRect.h - thickness * 2), color);
}

void TBSkin::paintRectFill(const TBRect &dstRect, const TBColor &color) {
	if (!dstRect.isEmpty())
		g_renderer->drawBitmapColored(dstRect, TBRect(), color, m_color_frag);
}

void TBSkin::paintElementBGColor(const TBRect &dstRect, TBSkinElement *element) {
	if (element->bg_color == 0)
		return;
	paintRectFill(dstRect, element->bg_color);
}

void TBSkin::paintElementImage(const TBRect &dstRect, TBSkinElement *element) {
	const TBRect src_rect(0, 0, element->bitmap->width(), element->bitmap->height());
	TBRect rect = dstRect.expand(element->expand, element->expand);
	rect.set(rect.x + element->img_ofs_x + (rect.w - src_rect.w) * element->img_position_x / 100,
			 rect.y + element->img_ofs_y + (rect.h - src_rect.h) * element->img_position_y / 100, src_rect.w,
			 src_rect.h);
	g_renderer->drawBitmap(rect, getFlippedRect(src_rect, element), element->bitmap);
}

void TBSkin::paintElementTile(const TBRect &dstRect, TBSkinElement *element) {
	const TBRect &rect = dstRect.expand(element->expand, element->expand);
	g_renderer->drawBitmapTile(rect, element->bitmap->getBitmap());
}

void TBSkin::paintElementStretchImage(const TBRect &dstRect, TBSkinElement *element) {
	if (dstRect.isEmpty())
		return;
	const TBRect &rect = dstRect.expand(element->expand, element->expand);
	const TBRect &src_rect = getFlippedRect(TBRect(0, 0, element->bitmap->width(), element->bitmap->height()), element);
	g_renderer->drawBitmap(rect, src_rect, element->bitmap);
}

void TBSkin::paintElementStretchBox(const TBRect &dstRect, TBSkinElement *element, bool fillCenter) {
	if (dstRect.isEmpty())
		return;

	TBRect rect = dstRect.expand(element->expand, element->expand);

	// Stretch the dst_cut (if rect is smaller than the skin size)
	// FIX: the expand should also be stretched!
	const int cut = element->cut;
	int dst_cut_w = Min(cut, rect.w / 2);
	int dst_cut_h = Min(cut, rect.h / 2);
	const int bw = element->bitmap->width();
	const int bh = element->bitmap->height();

	const bool has_left_right_edges = rect.h > dst_cut_h * 2;
	const bool has_top_bottom_edges = rect.w > dst_cut_w * 2;

	rect = getFlippedRect(rect, element);
	if (element->flip_x)
		dst_cut_w = -dst_cut_w;
	if (element->flip_y)
		dst_cut_h = -dst_cut_h;

	// Corners
	g_renderer->drawBitmap(TBRect(rect.x, rect.y, dst_cut_w, dst_cut_h), TBRect(0, 0, cut, cut), element->bitmap);
	g_renderer->drawBitmap(TBRect(rect.x + rect.w - dst_cut_w, rect.y, dst_cut_w, dst_cut_h),
						   TBRect(bw - cut, 0, cut, cut), element->bitmap);
	g_renderer->drawBitmap(TBRect(rect.x, rect.y + rect.h - dst_cut_h, dst_cut_w, dst_cut_h),
						   TBRect(0, bh - cut, cut, cut), element->bitmap);
	g_renderer->drawBitmap(TBRect(rect.x + rect.w - dst_cut_w, rect.y + rect.h - dst_cut_h, dst_cut_w, dst_cut_h),
						   TBRect(bw - cut, bh - cut, cut, cut), element->bitmap);

	// Left & right edge
	if (has_left_right_edges) {
		g_renderer->drawBitmap(TBRect(rect.x, rect.y + dst_cut_h, dst_cut_w, rect.h - dst_cut_h * 2),
							   TBRect(0, cut, cut, bh - cut * 2), element->bitmap);
		g_renderer->drawBitmap(
			TBRect(rect.x + rect.w - dst_cut_w, rect.y + dst_cut_h, dst_cut_w, rect.h - dst_cut_h * 2),
			TBRect(bw - cut, cut, cut, bh - cut * 2), element->bitmap);
	}

	// Top & bottom edge
	if (has_top_bottom_edges) {
		g_renderer->drawBitmap(TBRect(rect.x + dst_cut_w, rect.y, rect.w - dst_cut_w * 2, dst_cut_h),
							   TBRect(cut, 0, bw - cut * 2, cut), element->bitmap);
		g_renderer->drawBitmap(
			TBRect(rect.x + dst_cut_w, rect.y + rect.h - dst_cut_h, rect.w - dst_cut_w * 2, dst_cut_h),
			TBRect(cut, bh - cut, bw - cut * 2, cut), element->bitmap);
	}

	// Center
	if (fillCenter && has_top_bottom_edges && has_left_right_edges)
		g_renderer->drawBitmap(
			TBRect(rect.x + dst_cut_w, rect.y + dst_cut_h, rect.w - dst_cut_w * 2, rect.h - dst_cut_h * 2),
			TBRect(cut, cut, bw - cut * 2, bh - cut * 2), element->bitmap);
}

#ifdef TB_RUNTIME_DEBUG_INFO
void TBSkin::debug() {
	m_frag_manager.debug();
}
#endif // TB_RUNTIME_DEBUG_INFO

void TBSkin::onContextLost() {
	// We could simply do: m_frag_manager.DeleteBitmaps() and then all bitmaps
	// would be recreated automatically when needed. But because it's easy,
	// we unload everything so we save some memory (by not keeping any image
	// data around).
	unloadBitmaps();
}

void TBSkin::onContextRestored() {
	// Reload bitmaps (since we unloaded everything in OnContextLost())
	reloadBitmaps();
}

int TBSkin::getPxFromNode(TBNode *node, int defValue) const {
	return node ? m_dim_conv.getPxFromValue(&node->getValue(), defValue) : defValue;
}

// == TBSkinElement =========================================================

TBSkinElement::TBSkinElement()
	: bitmap(nullptr), cut(0), expand(0), type(SKIN_ELEMENT_TYPE_STRETCH_BOX), is_painting(false), is_getting(false),
	  padding_left(0), padding_top(0), padding_right(0), padding_bottom(0), width(SKIN_VALUE_NOT_SPECIFIED),
	  height(SKIN_VALUE_NOT_SPECIFIED), pref_width(SKIN_VALUE_NOT_SPECIFIED), pref_height(SKIN_VALUE_NOT_SPECIFIED),
	  min_width(SKIN_VALUE_NOT_SPECIFIED), min_height(SKIN_VALUE_NOT_SPECIFIED), max_width(SKIN_VALUE_NOT_SPECIFIED),
	  max_height(SKIN_VALUE_NOT_SPECIFIED), spacing(SKIN_VALUE_NOT_SPECIFIED), content_ofs_x(0), content_ofs_y(0),
	  img_ofs_x(0), img_ofs_y(0), img_position_x(50), img_position_y(50), flip_x(0), flip_y(0), opacity(1.f),
	  text_color(0, 0, 0, 0), bg_color(0, 0, 0, 0), bitmap_dpi(0) {
}

TBSkinElement::~TBSkinElement() {
}

int TBSkinElement::getIntrinsicMinWidth() const {
	if (bitmap && type == SKIN_ELEMENT_TYPE_IMAGE)
		return bitmap->width() - expand * 2;
	// Sizes below the skin cut size would start to shrink the skin below pretty,
	// so assume that's the default minimum size if it's not specified (minus expansion)
	return cut * 2 - expand * 2;
}

int TBSkinElement::getIntrinsicMinHeight() const {
	if (bitmap && type == SKIN_ELEMENT_TYPE_IMAGE)
		return bitmap->height() - expand * 2;
	// Sizes below the skin cut size would start to shrink the skin below pretty,
	// so assume that's the default minimum size if it's not specified (minus expansion)
	return cut * 2 - expand * 2;
}

int TBSkinElement::getIntrinsicWidth() const {
	if (width != SKIN_VALUE_NOT_SPECIFIED)
		return width;
	if (bitmap)
		return bitmap->width() - expand * 2;
	// FIX: We may want to check child elements etc.
	return SKIN_VALUE_NOT_SPECIFIED;
}

int TBSkinElement::getIntrinsicHeight() const {
	if (height != SKIN_VALUE_NOT_SPECIFIED)
		return height;
	if (bitmap)
		return bitmap->height() - expand * 2;
	// FIX: We may want to check child elements etc.
	return SKIN_VALUE_NOT_SPECIFIED;
}

void TBSkinElement::setBitmapDPI(const TBDimensionConverter &dimConv, int bitmapDpi) {
	if (this->bitmap_dpi) {
		// We have already applied the modifications so abort. This may
		// happen when we reload bitmaps without reloading the skin.
		return;
	}
	if (dimConv.needConversion()) {
		if (bitmapDpi == dimConv.getDstDPI()) {
			// The bitmap was loaded in a different DPI than the base DPI so
			// we must scale the bitmap properties.
			expand = expand * dimConv.getDstDPI() / dimConv.getSrcDPI();
			cut = cut * dimConv.getDstDPI() / dimConv.getSrcDPI();
		} else {
			// The bitmap was loaded in the base DPI and we need to scale it.
			// Apply the DPI conversion to the skin element scale factor.
			// FIX: For this to work well, we would need to apply scale to both
			//      image and all the other types of drawing too.
			// scale_x = scale_x * dim_conv.getDstDPI() / dim_conv.getSrcDPI();
			// scale_y = scale_y * dim_conv.getDstDPI() / dim_conv.getSrcDPI();
		}
	}
	this->bitmap_dpi = bitmapDpi;
}

bool TBSkinElement::hasState(SKIN_STATE state, TBSkinConditionContext &context) {
	return m_override_elements.getStateElement(state, context, TBSkinElementState::MATCH_RULE_ONLY_SPECIFIC_STATE) ||
		   m_child_elements.getStateElement(state, context, TBSkinElementState::MATCH_RULE_ONLY_SPECIFIC_STATE) ||
		   m_overlay_elements.getStateElement(state, context, TBSkinElementState::MATCH_RULE_ONLY_SPECIFIC_STATE);
}

void TBSkinElement::load(TBNode *n, TBSkin *skin, const char *skinPath) {
	if (const char *bitmap = n->getValueString("bitmap", nullptr)) {
		bitmap_file.clear();
		bitmap_file.append(skinPath);
		bitmap_file.append(bitmap);
	}

	// Note: Always read cut and expand as pixels. These values might later be
	//       recalculated depending on the DPI the bitmaps are available in.
	cut = n->getValueInt("cut", cut);
	expand = n->getValueInt("expand", expand);

	name.set(n->getName());
	id.set(n->getName());

	const TBDimensionConverter *dim_conv = skin->getDimensionConverter();

	if (TBNode *padding_node = n->getNode("padding")) {
		TBValue &val = padding_node->getValue();
		if (val.getArrayLength() == 4) {
			padding_top = dim_conv->getPxFromValue(val.getArray()->getValue(0), 0);
			padding_right = dim_conv->getPxFromValue(val.getArray()->getValue(1), 0);
			padding_bottom = dim_conv->getPxFromValue(val.getArray()->getValue(2), 0);
			padding_left = dim_conv->getPxFromValue(val.getArray()->getValue(3), 0);
		} else if (val.getArrayLength() == 2) {
			padding_top = padding_bottom = dim_conv->getPxFromValue(val.getArray()->getValue(0), 0);
			padding_left = padding_right = dim_conv->getPxFromValue(val.getArray()->getValue(1), 0);
		} else {
			padding_top = padding_right = padding_bottom = padding_left = dim_conv->getPxFromValue(&val, 0);
		}
	}
	width = skin->getPxFromNode(n->getNode("width"), width);
	height = skin->getPxFromNode(n->getNode("height"), height);
	pref_width = skin->getPxFromNode(n->getNode("pref-width"), pref_width);
	pref_height = skin->getPxFromNode(n->getNode("pref-height"), pref_height);
	min_width = skin->getPxFromNode(n->getNode("min-width"), min_width);
	min_height = skin->getPxFromNode(n->getNode("min-height"), min_height);
	max_width = skin->getPxFromNode(n->getNode("max-width"), max_width);
	max_height = skin->getPxFromNode(n->getNode("max-height"), max_height);
	spacing = skin->getPxFromNode(n->getNode("spacing"), spacing);
	content_ofs_x = skin->getPxFromNode(n->getNode("content-ofs-x"), content_ofs_x);
	content_ofs_y = skin->getPxFromNode(n->getNode("content-ofs-y"), content_ofs_y);
	img_position_x = n->getValueInt("img-position-x", img_position_x);
	img_position_y = n->getValueInt("img-position-y", img_position_y);
	img_ofs_x = skin->getPxFromNode(n->getNode("img-ofs-x"), img_ofs_x);
	img_ofs_y = skin->getPxFromNode(n->getNode("img-ofs-y"), img_ofs_y);
	flip_x = n->getValueInt("flip-x", flip_x);
	flip_y = n->getValueInt("flip-y", flip_y);
	opacity = n->getValueFloat("opacity", opacity);

	if (const char *color = n->getValueString("text-color", nullptr))
		text_color.setFromString(color, SDL_strlen(color));

	if (const char *color = n->getValueString("background-color", nullptr))
		bg_color.setFromString(color, SDL_strlen(color));

	if (const char *type_str = n->getValueString("type", nullptr))
		type = stringToType(type_str);

	// Create all state elements
	m_override_elements.load(n->getNode("overrides"));
	m_strong_override_elements.load(n->getNode("strong-overrides"));
	m_child_elements.load(n->getNode("children"));
	m_overlay_elements.load(n->getNode("overlays"));
}

// == TBSkinElementState ====================================================

bool TBSkinElementState::isMatch(SKIN_STATE state, TBSkinConditionContext &context, MATCH_RULE rule) const {
	if (rule == MATCH_RULE_ONLY_SPECIFIC_STATE && this->state == SKIN_STATE_ALL)
		return false;
	if ((state & this->state) || this->state == SKIN_STATE_ALL) {
		for (TBSkinCondition *condition = conditions.getFirst(); condition; condition = condition->getNext())
			if (!condition->getCondition(context))
				return false;
		return true;
	}
	return false;
}

bool TBSkinElementState::isExactMatch(SKIN_STATE state, TBSkinConditionContext &context, MATCH_RULE rule) const {
	if (rule == MATCH_RULE_ONLY_SPECIFIC_STATE && this->state == SKIN_STATE_ALL)
		return false;
	if (state == this->state || this->state == SKIN_STATE_ALL) {
		for (TBSkinCondition *condition = conditions.getFirst(); condition; condition = condition->getNext())
			if (!condition->getCondition(context))
				return false;
		return true;
	}
	return false;
}

// == TBSkinElementStateList ==================================================

TBSkinElementStateList::~TBSkinElementStateList() {
	while (TBSkinElementState *state = m_state_elements.getFirst()) {
		m_state_elements.remove(state);
		delete state;
	}
}

TBSkinElementState *TBSkinElementStateList::getStateElement(SKIN_STATE state, TBSkinConditionContext &context,
															TBSkinElementState::MATCH_RULE rule) const {
	// First try to get a state element with a exact match to the current state
	if (TBSkinElementState *element_state = getStateElementExactMatch(state, context, rule))
		return element_state;
	// No exact state match. Get a state with a partly match if there is one.
	TBSkinElementState *state_element = m_state_elements.getFirst();
	while (state_element) {
		if (state_element->isMatch(state, context, rule))
			return state_element;
		state_element = state_element->getNext();
	}
	return nullptr;
}

TBSkinElementState *TBSkinElementStateList::getStateElementExactMatch(SKIN_STATE state, TBSkinConditionContext &context,
																	  TBSkinElementState::MATCH_RULE rule) const {
	TBSkinElementState *state_element = m_state_elements.getFirst();
	while (state_element) {
		if (state_element->isExactMatch(state, context, rule))
			return state_element;
		state_element = state_element->getNext();
	}
	return nullptr;
}

void TBSkinElementStateList::load(TBNode *n) {
	if (!n)
		return;

	// For each node, create a new state element.
	TBNode *element_node = n->getFirstChild();
	while (element_node) {
		TBSkinElementState *state = new TBSkinElementState;
		if (!state)
			return;

		// By default, a state element applies to all combinations of states
		state->state = SKIN_STATE_ALL;
		state->element_id.set(element_node->getValue().getString());

		// Loop through all nodes, read state and create all found conditions.
		for (TBNode *condition_node = element_node->getFirstChild(); condition_node;
			 condition_node = condition_node->getNext()) {
			if (SDL_strcmp(condition_node->getName(), "state") == 0)
				state->state = stringToState(condition_node->getValue().getString());
			else if (SDL_strcmp(condition_node->getName(), "condition") == 0) {
				TBSkinCondition::TARGET target = stringToTarget(condition_node->getValueString("target", ""));

				const char *prop_str = condition_node->getValueString("property", "");
				TBSkinCondition::PROPERTY prop = stringToProperty(prop_str);
				TBID custom_prop;
				if (prop == TBSkinCondition::PROPERTY_CUSTOM)
					custom_prop.set(prop_str);

				TBID value;
				if (TBNode *value_n = condition_node->getNode("value")) {
					// Set the it to number or string. If it's a state, we must first convert the
					// state string to the SKIN_STATE state combo.
					if (prop == TBSkinCondition::PROPERTY_STATE)
						value.set(stringToState(value_n->getValue().getString()));
					else if (value_n->getValue().isString())
						value.set(value_n->getValue().getString());
					else
						value.set(value_n->getValue().getInt());
				}

				TBSkinCondition::TEST test = TBSkinCondition::TEST_EQUAL;
				if (const char *test_str = condition_node->getValueString("test", nullptr)) {
					if (SDL_strcmp(test_str, "!=") == 0)
						test = TBSkinCondition::TEST_NOT_EQUAL;
				}

				if (TBSkinCondition *condition = new TBSkinCondition(target, prop, custom_prop, value, test))
					state->conditions.addLast(condition);
			}
		}

		// State is reado to add
		m_state_elements.addLast(state);
		element_node = element_node->getNext();
	}
}

} // namespace tb
