/**
 * @file
 */

#include "tb_core.h"
#include "tb_editfield.h"
#include "tb_font_renderer.h"
#include "tb_tempbuffer.h"
#include "tb_widgets_reader.h"
#include "tb_window.h"
#include <SDL.h>

namespace tb {

#ifdef TB_RUNTIME_DEBUG_INFO

TBDebugInfo g_tb_debug;

TBDebugInfo::TBDebugInfo() {
	SDL_memset(settings, 0, sizeof(int) * NUM_SETTINGS);
}

/** Window showing runtime debug settings. */
class DebugSettingsWindow : public TBWindow, public TBWidgetListener {
public:
	TBEditField *output;

	TBOBJECT_SUBCLASS(DebugSettingsWindow, TBWindow);

	DebugSettingsWindow(TBWidget *root) {
		setText("Debug settings");
		g_widgets_reader->loadData(this, "TBLayout: axis: y, distribution: available, position: left\n"
										 "	TBLayout: id: 'container', axis: y, size: available\n"
										 "	TBTextField: text: 'Event output:'\n"
										 "	TBEditField: id: 'output', gravity: all, multiline: 1, wrap: 0\n"
										 "		lp: pref-height: 100dp");

		addCheckbox(TBDebugInfo::LAYOUT_BOUNDS, "Layout bounds");
		addCheckbox(TBDebugInfo::LAYOUT_CLIPPING, "Layout clipping");
		addCheckbox(TBDebugInfo::LAYOUT_PS_DEBUGGING, "Layout size calculation");
		addCheckbox(TBDebugInfo::RENDER_BATCHES, "Render batches");
		addCheckbox(TBDebugInfo::RENDER_SKIN_BITMAP_FRAGMENTS, "Render skin bitmap fragments");
		addCheckbox(TBDebugInfo::RENDER_FONT_BITMAP_FRAGMENTS, "Render font bitmap fragments");

		output = getWidgetByIDAndType<TBEditField>(TBIDC("output"));

		TBRect bounds(0, 0, root->getRect().w, root->getRect().h);
		setRect(getResizeToFitContentRect().centerIn(bounds).moveIn(bounds).clip(bounds));

		root->addChild(this);

		TBWidgetListener::addGlobalListener(this);
	}

	~DebugSettingsWindow() {
		TBWidgetListener::removeGlobalListener(this);
	}

	void addCheckbox(TBDebugInfo::SETTING setting, const char *str) {
		TBCheckBox *check = new TBCheckBox();
		check->setValue(g_tb_debug.settings[setting]);
		check->data.setInt(setting);
		check->setID(TBIDC("check"));

		TBClickLabel *label = new TBClickLabel();
		label->setText(str);
		label->getContentRoot()->addChild(check, WIDGET_Z_BOTTOM);

		getWidgetByID(TBIDC("container"))->addChild(label);
	}

	virtual bool onEvent(const TBWidgetEvent &ev) {
		if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("check")) {
			// Update setting and invalidate
			g_tb_debug.settings[ev.target->data.getInt()] = ev.target->getValue();
			getParentRoot()->invalidate();
			return true;
		}
		return TBWindow::onEvent(ev);
	}

	virtual void onPaint(const PaintProps &paintProps) {
		// Draw stuff to the right of the debug window
		g_renderer->translate(getRect().w, 0);

		// Draw skin bitmap fragments
		if (TB_DEBUG_SETTING(RENDER_SKIN_BITMAP_FRAGMENTS)) {
			g_tb_skin->debug();
		}

		// Draw font glyph fragments (the font of the hovered widget)
		if (TB_DEBUG_SETTING(RENDER_FONT_BITMAP_FRAGMENTS)) {
			TBWidget *widget =
				TBWidget::hovered_widget != nullptr ? TBWidget::hovered_widget : TBWidget::focused_widget;
			g_font_manager
				->getFontFace(widget != nullptr ? widget->getCalculatedFontDescription()
												: g_font_manager->getDefaultFontDescription())
				->debug();
		}

		g_renderer->translate(-getRect().w, 0);
	}

	TBStr getIdString(const TBID &id) {
		TBStr str;
		str.setFormatted("%u", (uint32_t)id);
		return str;
	}

	// TBWidgetListener
	virtual bool onWidgetInvokeEvent(TBWidget *widget, const TBWidgetEvent &ev) {
		// Skip these events for now
		if (ev.isPointerEvent()) {
			return false;
		}

		// Always ignore activity in this window (or we might get endless recursion)
		if (TBWindow *window = widget->getParentWindow()) {
			if (TBSafeCast<DebugSettingsWindow>(window) != nullptr) {
				return false;
			}
		}

		TBTempBuffer buf;
		buf.appendString(getEventTypeStr(ev.type));
		buf.appendString(" (");
		buf.appendString(widget->getClassName());
		buf.appendString(")");

		buf.appendString(" id: ");
		buf.appendString(getIdString(ev.target->getID()));

		if (ev.ref_id != 0U) {
			buf.appendString(", ref_id: ");
			buf.appendString(getIdString(ev.ref_id));
		}

		if (ev.type == EVENT_TYPE_CHANGED) {
			TBStr extra;
			TBStr text;
			if (ev.target->getText(text) && text.length() > 24) {
				sprintf(text.c_str() + 20, "...");
			}
			extra.setFormatted(", value: %.2f (\"%s\")", ev.target->getValueDouble(), text.c_str());
			buf.appendString(extra);
		}
		buf.appendString("\n");

		// Append the line to the output textfield
		TBStyleEdit *se = output->getStyleEdit();
		se->selection.selectNothing();
		se->appendText(buf.getData(), TB_ALL_TO_TERMINATION, true);
		se->scrollIfNeeded(false, true);

		// Remove lines from the top if we exceed the height limit.
		const int height_limit = 2000;
		int current_height = se->getContentHeight();
		if (current_height > height_limit) {
			se->caret.place(TBPoint(0, current_height - height_limit));
			se->selection.selectToCaret(se->blocks.getFirst(), 0);
			se->del();
		}
		return false;
	}

	const char *getEventTypeStr(EVENT_TYPE type) const {
		switch (type) {
		case EVENT_TYPE_CLICK:
			return "CLICK";
		case EVENT_TYPE_LONG_CLICK:
			return "LONG_CLICK";
		case EVENT_TYPE_POINTER_DOWN:
			return "POINTER_DOWN";
		case EVENT_TYPE_POINTER_UP:
			return "POINTER_UP";
		case EVENT_TYPE_POINTER_MOVE:
			return "POINTER_MOVE";
		case EVENT_TYPE_TOUCH_DOWN:
			return "TOUCH_DOWN";
		case EVENT_TYPE_TOUCH_UP:
			return "TOUCH_UP";
		case EVENT_TYPE_TOUCH_MOVE:
			return "TOUCH_MOVE";
		case EVENT_TYPE_TOUCH_CANCEL:
			return "TOUCH_CANCEL";
		case EVENT_TYPE_WHEEL:
			return "WHEEL";
		case EVENT_TYPE_CHANGED:
			return "CHANGED";
		case EVENT_TYPE_KEY_DOWN:
			return "KEY_DOWN";
		case EVENT_TYPE_KEY_UP:
			return "KEY_UP";
		case EVENT_TYPE_SHORTCUT:
			return "SHORT_CUT";
		case EVENT_TYPE_CONTEXT_MENU:
			return "CONTEXT_MENU";
		default:
			return "[UNKNOWN]";
		}
	}
};

void ShowDebugInfoSettingsWindow(TBWidget *root) {
	new DebugSettingsWindow(root);
}

#endif // TB_RUNTIME_DEBUG_INFO

} // namespace tb
