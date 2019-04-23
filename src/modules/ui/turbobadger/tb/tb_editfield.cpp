/**
 * @file
 */

#include "tb_editfield.h"
#include "tb_font_renderer.h"
#include "tb_language.h"
#include "tb_menu_window.h"
#include "tb_select.h"
#include "tb_skin_util.h"
#include "tb_style_edit_content.h"
#include "tb_system.h"
#include "tb_widget_skin_condition_context.h"
#include "tb_widgets_reader.h"

namespace tb {

const int CARET_BLINK_TIME = 500;
const int SELECTION_SCROLL_DELAY = 1000 / 30;

/** Get the delta that should be scrolled if dragging the pointer outside the range min-max */
int getSelectionScrollSpeed(int pointerpos, int min, int max) {
	int d = 0;
	if (pointerpos < min) {
		d = pointerpos - min;
	} else if (pointerpos > max) {
		d = pointerpos - max;
	}
	d *= d;
	d /= 40;
	return (pointerpos < min) ? -d : d;
}

TBEditField::TBEditField() : m_edit_type(EDIT_TYPE_TEXT), m_adapt_to_content_size(false), m_virtual_width(250) {
	setIsFocusable(true);
	setWantLongClick(true);
	addChild(&m_scrollbar_x);
	addChild(&m_scrollbar_y);
	addChild(&m_root);
	m_root.setGravity(WIDGET_GRAVITY_ALL);
	m_scrollbar_x.setGravity(WIDGET_GRAVITY_BOTTOM | WIDGET_GRAVITY_LEFT_RIGHT);
	m_scrollbar_y.setGravity(WIDGET_GRAVITY_RIGHT | WIDGET_GRAVITY_TOP_BOTTOM);
	m_scrollbar_y.setAxis(AXIS_Y);
	int scrollbar_y_w = m_scrollbar_y.getPreferredSize().pref_w;
	int scrollbar_x_h = m_scrollbar_x.getPreferredSize().pref_h;
	m_scrollbar_x.setRect(TBRect(0, -scrollbar_x_h, -scrollbar_y_w, scrollbar_x_h));
	m_scrollbar_y.setRect(TBRect(-scrollbar_y_w, 0, scrollbar_y_w, 0));
	m_scrollbar_x.setOpacity(0);
	m_scrollbar_y.setOpacity(0);

	setSkinBg(TBIDC("TBEditField"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
	m_style_edit.setListener(this);

	m_root.setRect(getVisibleRect());

	m_placeholder.setTextAlign(TB_TEXT_ALIGN_LEFT);

	m_content_factory.editfield = this;
	m_style_edit.setContentFactory(&m_content_factory);
}

TBEditField::~TBEditField() {
	removeChild(&m_root);
	removeChild(&m_scrollbar_y);
	removeChild(&m_scrollbar_x);
}

TBRect TBEditField::getVisibleRect() {
	TBRect rect = getPaddingRect();
	if (m_scrollbar_y.getOpacity() != 0.0F) {
		rect.w -= m_scrollbar_y.getRect().w;
	}
	if (m_scrollbar_x.getOpacity() != 0.0F) {
		rect.h -= m_scrollbar_x.getRect().h;
	}
	return rect;
}

void TBEditField::updateScrollbarVisibility(bool multiline) {
	bool enable_vertical = multiline && !m_adapt_to_content_size;
	m_scrollbar_y.setOpacity(enable_vertical ? 1.F : 0.F);
	m_root.setRect(getVisibleRect());
}

void TBEditField::setAdaptToContentSize(bool adapt) {
	if (m_adapt_to_content_size == adapt) {
		return;
	}
	m_adapt_to_content_size = adapt;
	updateScrollbarVisibility(getMultiline());
}

void TBEditField::setVirtualWidth(int virtualWidth) {
	if (m_virtual_width == virtualWidth) {
		return;
	}
	m_virtual_width = virtualWidth;

	if (m_adapt_to_content_size && m_style_edit.packed.wrapping) {
		invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
	}
}

void TBEditField::setMultiline(bool multiline) {
	if (multiline == getMultiline()) {
		return;
	}
	updateScrollbarVisibility(multiline);
	m_style_edit.setMultiline(multiline);
	setWrapping(multiline);
	invalidateSkinStates();
	TBWidget::invalidate();
}

void TBEditField::setStyling(bool styling) {
	m_style_edit.setStyling(styling);
}

void TBEditField::setReadOnly(bool readonly) {
	if (readonly == getReadOnly()) {
		return;
	}
	m_style_edit.setReadOnly(readonly);
	invalidateSkinStates();
	TBWidget::invalidate();
}

void TBEditField::setWrapping(bool wrapping) {
	if (wrapping == getWrapping()) {
		return;
	}

	m_style_edit.setWrapping(wrapping);

	// Invalidate the layout when the wrap mode change and we should adapt our size to it
	if (m_adapt_to_content_size) {
		invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
	}
}

void TBEditField::setEditType(EDIT_TYPE type) {
	if (m_edit_type == type) {
		return;
	}
	m_edit_type = type;
	m_style_edit.setPassword(type == EDIT_TYPE_PASSWORD);
	invalidateSkinStates();
	TBWidget::invalidate();
}

bool TBEditField::getCustomSkinCondition(const TBSkinCondition::CONDITION_INFO &info) {
	if (info.custom_prop == TBIDC("edit-type")) {
		switch (m_edit_type) {
		case EDIT_TYPE_TEXT:
			return info.value == TBIDC("text");
		case EDIT_TYPE_SEARCH:
			return info.value == TBIDC("search");
		case EDIT_TYPE_PASSWORD:
			return info.value == TBIDC("password");
		case EDIT_TYPE_EMAIL:
			return info.value == TBIDC("email");
		case EDIT_TYPE_PHONE:
			return info.value == TBIDC("phone");
		case EDIT_TYPE_URL:
			return info.value == TBIDC("url");
		case EDIT_TYPE_NUMBER:
			return info.value == TBIDC("number");
		}
	} else if (info.custom_prop == TBIDC("multiline")) {
		const bool ml = ((uint32_t)info.value) == 0U;
		return ml == !getMultiline();
	} else if (info.custom_prop == TBIDC("readonly")) {
		const bool ro = ((uint32_t)info.value) == 0U;
		return ro == !getReadOnly();
	}
	return false;
}

void TBEditField::scrollTo(int x, int y) {
	int old_x = m_scrollbar_x.getValue();
	int old_y = m_scrollbar_y.getValue();
	m_style_edit.setScrollPos(x, y);
	if (old_x != m_scrollbar_x.getValue() || old_y != m_scrollbar_y.getValue()) {
		TBWidget::invalidate();
	}
}

TBWidget::ScrollInfo TBEditField::getScrollInfo() {
	ScrollInfo info;
	info.min_x = static_cast<int>(m_scrollbar_x.getMinValue());
	info.min_y = static_cast<int>(m_scrollbar_y.getMinValue());
	info.max_x = static_cast<int>(m_scrollbar_x.getMaxValue());
	info.max_y = static_cast<int>(m_scrollbar_y.getMaxValue());
	info.x = m_scrollbar_x.getValue();
	info.y = m_scrollbar_y.getValue();
	return info;
}

bool TBEditField::onEvent(const TBWidgetEvent &ev) {
	if (ev.type == EVENT_TYPE_CHANGED && ev.target == &m_scrollbar_x) {
		m_style_edit.setScrollPos(m_scrollbar_x.getValue(), m_style_edit.scroll_y);
		onScroll(m_scrollbar_x.getValue(), m_style_edit.scroll_y);
		return true;
	}
	if (ev.type == EVENT_TYPE_CHANGED && ev.target == &m_scrollbar_y) {
		m_style_edit.setScrollPos(m_style_edit.scroll_x, m_scrollbar_y.getValue());
		onScroll(m_style_edit.scroll_x, m_scrollbar_y.getValue());
		return true;
	}
	if (ev.type == EVENT_TYPE_WHEEL && ev.modifierkeys == TB_MODIFIER_NONE) {
		int old_val = m_scrollbar_y.getValue();
		m_scrollbar_y.setValue(old_val + ev.delta_y * TBSystem::getPixelsPerLine());
		return m_scrollbar_y.getValue() != old_val;
	}
	if (ev.type == EVENT_TYPE_POINTER_DOWN && ev.target == this) {
		TBRect padding_rect = getPaddingRect();
		if (m_style_edit.mouseDown(TBPoint(ev.target_x - padding_rect.x, ev.target_y - padding_rect.y), 1, ev.count,
								   TB_MODIFIER_NONE, ev.button_type == TB_TOUCH)) {
			// Post a message to start selection scroll
			postMessageDelayed(TBIDC("selscroll"), nullptr, SELECTION_SCROLL_DELAY);
			return true;
		}
	} else if (ev.type == EVENT_TYPE_POINTER_MOVE && ev.target == this) {
		TBRect padding_rect = getPaddingRect();
		return m_style_edit.mouseMove(TBPoint(ev.target_x - padding_rect.x, ev.target_y - padding_rect.y));
	} else if (ev.type == EVENT_TYPE_POINTER_UP && ev.target == this) {
		TBRect padding_rect = getPaddingRect();
		return m_style_edit.mouseUp(TBPoint(ev.target_x - padding_rect.x, ev.target_y - padding_rect.y), 1,
									TB_MODIFIER_NONE, ev.button_type == TB_TOUCH);
	} else if (ev.type == EVENT_TYPE_KEY_DOWN) {
		if (m_style_edit.keyDown(ev.key, ev.special_key, ev.modifierkeys)) {
			if (_var) {
				_var->setVal(getText().c_str());
				_var->markClean();
			}
			return true;
		}
		return false;
	} else if (ev.type == EVENT_TYPE_KEY_UP) {
		return true;
	} else if ((ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("popupmenu")) ||
			   (ev.type == EVENT_TYPE_SHORTCUT)) {
		if (ev.ref_id == TBIDC("cut") && !m_style_edit.packed.read_only) {
			m_style_edit.cut();
			if (_var) {
				_var->setVal(getText().c_str());
				_var->markClean();
			}
		} else if (ev.ref_id == TBIDC("copy")) {
			m_style_edit.copy();
		} else if (ev.ref_id == TBIDC("paste") && !m_style_edit.packed.read_only) {
			m_style_edit.paste();
			if (_var) {
				_var->setVal(getText().c_str());
				_var->markClean();
			}
		} else if (ev.ref_id == TBIDC("delete") && !m_style_edit.packed.read_only) {
			m_style_edit.del();
			if (_var) {
				_var->setVal(getText().c_str());
				_var->markClean();
			}
		} else if (ev.ref_id == TBIDC("undo") && !m_style_edit.packed.read_only) {
			m_style_edit.undo();
			if (_var) {
				_var->setVal(getText().c_str());
				_var->markClean();
			}
		} else if (ev.ref_id == TBIDC("redo") && !m_style_edit.packed.read_only) {
			m_style_edit.redo();
			if (_var) {
				_var->setVal(getText().c_str());
				_var->markClean();
			}
		} else if (ev.ref_id == TBIDC("selectall")) {
			m_style_edit.selection.selectAll();
		} else {
			return false;
		}
		return true;
	} else if (ev.type == EVENT_TYPE_CONTEXT_MENU && ev.target == this) {
		TBPoint pos_in_root(ev.target_x, ev.target_y);
		ev.target->convertToRoot(pos_in_root.x, pos_in_root.y);

		if (TBMenuWindow *menu = new TBMenuWindow(ev.target, TBIDC("popupmenu"))) {
			TBGenericStringItemSource *source = menu->getList()->getDefaultSource();
			source->addItem(new TBGenericStringItem(g_tb_lng->getString(TBIDC("cut")), TBIDC("cut")));
			source->addItem(new TBGenericStringItem(g_tb_lng->getString(TBIDC("copy")), TBIDC("copy")));
			source->addItem(new TBGenericStringItem(g_tb_lng->getString(TBIDC("paste")), TBIDC("paste")));
			source->addItem(new TBGenericStringItem(g_tb_lng->getString(TBIDC("delete")), TBIDC("delete")));
			source->addItem(new TBGenericStringItem("-"));
			source->addItem(new TBGenericStringItem(g_tb_lng->getString(TBIDC("selectall")), TBIDC("selectall")));
			menu->show(source, TBPopupAlignment(pos_in_root), -1);
		}
		return true;
	}
	return false;
}

void TBEditField::onPaint(const PaintProps &paintProps) {
	TBRect visible_rect = getVisibleRect();

	bool clip = m_scrollbar_x.canScroll() || m_scrollbar_y.canScroll();
	TBRect old_clip;
	if (clip) {
		old_clip = g_renderer->setClipRect(visible_rect, true);
	}

	int trans_x = visible_rect.x;
	int trans_y = visible_rect.y;
	g_renderer->translate(trans_x, trans_y);

	// Draw text content, caret etc.
	visible_rect.x = visible_rect.y = 0;
	m_style_edit.paint(visible_rect, getCalculatedFontDescription(), paintProps.text_color);

	// If empty, draw placeholder text with some opacity.
	if (m_style_edit.isEmpty()) {
		float old_opacity = g_renderer->getOpacity();
		g_renderer->setOpacity(old_opacity * g_tb_skin->getDefaultPlaceholderOpacity());
		TBRect placeholder_rect(visible_rect.x, visible_rect.y, visible_rect.w, getFont()->getHeight());
		m_placeholder.paint(this, placeholder_rect, paintProps.text_color);
		g_renderer->setOpacity(old_opacity);
	}
	g_renderer->translate(-trans_x, -trans_y);

	if (clip) {
		g_renderer->setClipRect(old_clip, false);
	}
}

void TBEditField::setTextFormatted(const char *format, ...) {
	va_list args;
	va_start(args, format);
	char buf[1024];
	SDL_vsnprintf(buf, sizeof(buf), format, args);
	buf[sizeof(buf) - 1] = '\0';
	setText(buf);
	va_end(args);
}

void TBEditField::onPaintChildren(const PaintProps &paintProps) {
	TBWidget::onPaintChildren(paintProps);

	// Draw fadeout skin at the needed edges.
	drawEdgeFadeout(getVisibleRect(), TBIDC("TBEditField.fadeout_x"), TBIDC("TBEditField.fadeout_y"),
					m_scrollbar_x.getValue(), m_scrollbar_y.getValue(),
					(int)(m_scrollbar_x.getMaxValue() - m_scrollbar_x.getValueDouble()),
					(int)(m_scrollbar_y.getMaxValue() - m_scrollbar_y.getValueDouble()));
}

void TBEditField::onAdded() {
	m_style_edit.setFont(getCalculatedFontDescription());
}

void TBEditField::onFontChanged() {
	m_style_edit.setFont(getCalculatedFontDescription());
}

void TBEditField::onFocusChanged(bool focused) {
	m_style_edit.focus(focused);
}

void TBEditField::onResized(int oldW, int oldH) {
	// Make the scrollbars move
	TBWidget::onResized(oldW, oldH);

	TBRect visible_rect = getVisibleRect();
	m_style_edit.setLayoutSize(visible_rect.w, visible_rect.h, false);

	updateScrollbars();
}

PreferredSize TBEditField::onCalculatePreferredContentSize(const SizeConstraints &constraints) {
	int font_height = getFont()->getHeight();
	PreferredSize ps;
	if (m_adapt_to_content_size) {
		int old_layout_width = m_style_edit.layout_width;
		int old_layout_height = m_style_edit.layout_height;
		if (m_style_edit.packed.wrapping) {
			// If we have wrapping enabled, we have to set a virtual width and format the text
			// so we can get the actual content width with a constant result every time.
			// If the layouter does not respect our size constraints in the end, we may
			// get a completly different content height due to different wrapping.
			// To fix that, we need to layout in 2 passes.

			// A hacky fix is to do something we probably shouldn't: use the old layout width
			// as virtual width for the new.
			// int layout_width = old_layout_width > 0 ? Max(old_layout_width, m_virtual_width) : m_virtual_width;
			int layout_width = m_virtual_width;
			if (constraints.available_w != SizeConstraints::NO_RESTRICTION) {
				layout_width = constraints.available_w;
				if (TBSkinElement *bg_skin = getSkinBgElement()) {
					layout_width -= bg_skin->padding_left + bg_skin->padding_right;
				}
			}

			m_style_edit.setLayoutSize(layout_width, old_layout_height, true);
			ps.size_dependency = SIZE_DEP_HEIGHT_DEPEND_ON_WIDTH;
		}
		int width = m_style_edit.getContentWidth();
		int height = m_style_edit.getContentHeight();
		if (m_style_edit.packed.wrapping) {
			m_style_edit.setLayoutSize(old_layout_width, old_layout_height, true);
		}
		height = Max(height, font_height);

		ps.min_w = ps.pref_w /*= ps.max_w*/ = width; // should go with the hack above.
		// ps.min_w = ps.pref_w = ps.max_w = width;
		ps.min_h = ps.pref_h = ps.max_h = height;
	} else {
		ps.pref_h = ps.min_h = font_height;
		if (m_style_edit.packed.multiline_on) {
			ps.pref_w = font_height * 10;
			ps.pref_h = font_height * 5;
		} else {
			ps.max_h = ps.pref_h;
		}
	}
	return ps;
}

void TBEditField::onMessageReceived(TBMessage *msg) {
	if (msg->message == TBIDC("blink")) {
		m_style_edit.caret.on = !m_style_edit.caret.on;
		m_style_edit.caret.invalidate();

		// Post another blink message so we blink again.
		postMessageDelayed(TBIDC("blink"), nullptr, CARET_BLINK_TIME);
	} else if (msg->message == TBIDC("selscroll") && captured_widget == this) {
		// Get scroll speed from where mouse is relative to the padding rect.
		TBRect padding_rect = getVisibleRect().shrink(2, 2);
		int dx = getSelectionScrollSpeed(pointer_move_widget_x, padding_rect.x, padding_rect.x + padding_rect.w);
		int dy = getSelectionScrollSpeed(pointer_move_widget_y, padding_rect.y, padding_rect.y + padding_rect.h);
		m_scrollbar_x.setValue(m_scrollbar_x.getValue() + dx);
		m_scrollbar_y.setValue(m_scrollbar_y.getValue() + dy);

		// Handle mouse move at the new scroll position, so selection is updated
		if ((dx != 0) || (dy != 0)) {
			m_style_edit.mouseMove(TBPoint(pointer_move_widget_x, pointer_move_widget_y));
		}

		// Post another setscroll message so we continue scrolling if we still should.
		if (m_style_edit.select_state != 0) {
			postMessageDelayed(TBIDC("selscroll"), nullptr, SELECTION_SCROLL_DELAY);
		}
	}
}

void TBEditField::onChange() {
	// Invalidate the layout when the content change and we should adapt our size to it
	if (m_adapt_to_content_size) {
		invalidateLayout(INVALIDATE_LAYOUT_RECURSIVE);
	}

	TBWidgetEvent ev(EVENT_TYPE_CHANGED);
	invokeEvent(ev);
}

bool TBEditField::onEnter() {
	return false;
}

void TBEditField::invalidate(const TBRect &rect) {
	TBWidget::invalidate();
}

void TBEditField::drawString(int32_t x, int32_t y, TBFontFace *font, const TBColor &color, const char *str,
							 int32_t len) {
	font->drawString(x, y, color, str, len);
}

void TBEditField::drawRect(const TBRect &rect, const TBColor &color) {
	g_tb_skin->paintRect(rect, color, 1);
}

void TBEditField::drawRectFill(const TBRect &rect, const TBColor &color) {
	g_tb_skin->paintRectFill(rect, color);
}

void TBEditField::drawTextSelectionBg(const TBRect &rect) {
	TBWidgetSkinConditionContext context(this);
	g_tb_skin->paintSkin(rect, TBIDC("TBEditField.selection"), static_cast<SKIN_STATE>(getAutoState()), context);
}

void TBEditField::drawContentSelectionFg(const TBRect &rect) {
	TBWidgetSkinConditionContext context(this);
	g_tb_skin->paintSkin(rect, TBIDC("TBEditField.selection"), static_cast<SKIN_STATE>(getAutoState()), context);
}

void TBEditField::drawCaret(const TBRect &rect) {
	if (getIsFocused() && !m_style_edit.packed.read_only) {
		drawTextSelectionBg(rect);
	}
}

void TBEditField::scroll(int32_t dx, int32_t dy) {
	TBWidget::invalidate();
	m_scrollbar_x.setValue(m_style_edit.scroll_x);
	m_scrollbar_y.setValue(m_style_edit.scroll_y);
}

void TBEditField::updateScrollbars() {
	int32_t w = m_style_edit.layout_width;
	int32_t h = m_style_edit.layout_height;
	m_scrollbar_x.setLimits(0, m_style_edit.getContentWidth() - w, w);
	m_scrollbar_y.setLimits(0, m_style_edit.getContentHeight() - h, h);
}

void TBEditField::caretBlinkStart() {
	// Post the delayed blink message if we don't already have one
	if (getMessageByID(TBIDC("blink")) == nullptr) {
		postMessageDelayed(TBIDC("blink"), nullptr, CARET_BLINK_TIME);
	}
}

void TBEditField::caretBlinkStop() {
	// Remove the blink message if we have one
	if (TBMessage *msg = getMessageByID(TBIDC("blink"))) {
		deleteMessage(msg);
	}
}

// == TBEditFieldScrollRoot =======================================================================

void TBEditFieldScrollRoot::onPaintChildren(const PaintProps &paintProps) {
	// Avoid setting clipping (can be expensive) if we have no children to paint anyway.
	if (getFirstChild() == nullptr) {
		return;
	}
	// Clip children
	TBRect old_clip_rect = g_renderer->setClipRect(getPaddingRect(), true);
	TBWidget::onPaintChildren(paintProps);
	g_renderer->setClipRect(old_clip_rect, false);
}

void TBEditFieldScrollRoot::getChildTranslation(int &x, int &y) const {
	TBEditField *edit_field = static_cast<TBEditField *>(getParent());
	x = (int)-edit_field->getStyleEdit()->scroll_x;
	y = (int)-edit_field->getStyleEdit()->scroll_y;
}

WIDGET_HIT_STATUS TBEditFieldScrollRoot::getHitStatus(int x, int y) {
	// Return no hit on this widget, but maybe on any of the children.
	if ((TBWidget::getHitStatus(x, y) != 0U) && (getWidgetAt(x, y, false) != nullptr)) {
		return WIDGET_HIT_STATUS_HIT;
	}
	return WIDGET_HIT_STATUS_NO_HIT;
}

// == TBTextFragmentContentWidget =================================================================

class TBTextFragmentContentWidget : public TBTextFragmentContent {
public:
	TBTextFragmentContentWidget(TBWidget *parent, TBWidget *widget);
	virtual ~TBTextFragmentContentWidget();

	virtual void updatePos(const TBBlock *block, int x, int y);
	virtual int32_t getWidth(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment);
	virtual int32_t getHeight(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment);
	virtual int32_t getBaseline(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment);

private:
	TBWidget *m_widget;
};

TBTextFragmentContentWidget::TBTextFragmentContentWidget(TBWidget *parent, TBWidget *widget) : m_widget(widget) {
	parent->getContentRoot()->addChild(widget);
}

TBTextFragmentContentWidget::~TBTextFragmentContentWidget() {
	m_widget->removeFromParent();
	delete m_widget;
}

void TBTextFragmentContentWidget::updatePos(const TBBlock *block, int x, int y) {
	m_widget->setRect(TBRect(x, y, getWidth(block, nullptr, nullptr), getHeight(block, nullptr, nullptr)));
}

int32_t TBTextFragmentContentWidget::getWidth(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment) {
	return m_widget->getRect().w != 0 ? m_widget->getRect().w : m_widget->getPreferredSize().pref_w;
}

int32_t TBTextFragmentContentWidget::getHeight(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment) {
	return m_widget->getRect().h != 0 ? m_widget->getRect().h : m_widget->getPreferredSize().pref_h;
}

int32_t TBTextFragmentContentWidget::getBaseline(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment) {
	int height = getHeight(block, font, fragment);
	return (height + block->calculateBaseline(font)) / 2;
}

// == TBEditFieldContentFactory ===================================================================

int TBEditFieldContentFactory::getContent(const char *text) {
	return TBTextFragmentContentFactory::getContent(text);
}

TBTextFragmentContent *TBEditFieldContentFactory::createFragmentContent(const char *text, int textLen) {
	if (strncmp(text, "<widget ", Min(textLen, 8)) == 0) {
		// Create a wrapper for the generated widget.
		// Its size will adapt to the content.
		if (TBWidget *widget = new TBWidget()) {
			if (TBTextFragmentContentWidget *cw = new TBTextFragmentContentWidget(editfield, widget)) {
				g_widgets_reader->loadData(widget, text + 8, textLen - 9);
				return cw;
			}
			delete widget;
		}
	}

	return TBTextFragmentContentFactory::createFragmentContent(text, textLen);
}

} // namespace tb
