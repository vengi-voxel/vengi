/**
 * @file
 */

#include "Demo.h"
#include "ListWindow.h"
#include "ResourceEditWindow.h"
#include <stdio.h>
#include <stdarg.h>
#include "tb_system.h"
#include "tb_language.h"
#include "tb_inline_select.h"
#include "tb_select.h"
#include "tb_menu_window.h"
#include "tb_editfield.h"
#include "tb_tab_container.h"
#include "tb_bitmap_fragment.h"
#include "animation/tb_widget_animation.h"
#include "tb_node_tree.h"
#include "tb_tempbuffer.h"
#include "tb_font_renderer.h"
#include "image/tb_image_manager.h"
#include "utf8/utf8.h"

AdvancedItemSource advanced_source;
TBGenericStringItemSource name_source;
TBGenericStringItemSource popup_menu_source;

#ifdef TB_SUPPORT_CONSTEXPR

void const_expr_test()
{
	// Some code here just to see if the compiler really did
	// implement constexpr (and not just ignored it)
	// Should obviosly only compile if it really works. If not,
	// disable TB_SUPPORT_CONSTEXPR in tb_hash.h for your compiler.
	TBID id("foo");
	switch(id)
	{
		case TBIDC("foo"):
			break;
		case TBIDC("baar"):
			break;
		default:
			break;
	}
}

#endif // TB_SUPPORT_CONSTEXPR

// == DemoWindow ==============================================================

DemoWindow::DemoWindow(TBWidget *root)
{
	root->addChild(this);
}

bool DemoWindow::loadResourceFile(const char *filename)
{
	// We could do g_widgets_reader->LoadFile(this, filename) but we want
	// some extra data we store under "WindowInfo", so read into node tree.
	TBNode node;
	if (!node.readFile(filename))
		return false;
	loadResource(node);
	return true;
}

void DemoWindow::loadResourceData(const char *data)
{
	// We could do g_widgets_reader->LoadData(this, filename) but we want
	// some extra data we store under "WindowInfo", so read into node tree.
	TBNode node;
	node.readData(data);
	loadResource(node);
}

void DemoWindow::loadResource(TBNode &node)
{
	g_widgets_reader->loadNodeTree(this, &node);

	// Get title from the WindowInfo section (or use "" if not specified)
	setText(node.getValueString("WindowInfo>title", ""));

	const TBRect parent_rect(0, 0, getParent()->getRect().w, getParent()->getRect().h);
	const TBDimensionConverter *dc = g_tb_skin->getDimensionConverter();
	TBRect window_rect = getResizeToFitContentRect();

	// Use specified size or adapt to the preferred content size.
	TBNode *tmp = node.getNode("WindowInfo>size");
	if (tmp && tmp->getValue().getArrayLength() == 2)
	{
		window_rect.w = dc->getPxFromString(tmp->getValue().getArray()->getValue(0)->getString(), window_rect.w);
		window_rect.h = dc->getPxFromString(tmp->getValue().getArray()->getValue(1)->getString(), window_rect.h);
	}

	// Use the specified position or center in parent.
	tmp = node.getNode("WindowInfo>position");
	if (tmp && tmp->getValue().getArrayLength() == 2)
	{
		window_rect.x = dc->getPxFromString(tmp->getValue().getArray()->getValue(0)->getString(), window_rect.x);
		window_rect.y = dc->getPxFromString(tmp->getValue().getArray()->getValue(1)->getString(), window_rect.y);
	}
	else
		window_rect = window_rect.centerIn(parent_rect);

	// Make sure the window is inside the parent, and not larger.
	window_rect = window_rect.moveIn(parent_rect).clip(parent_rect);

	setRect(window_rect);

	// Ensure we have focus - now that we've filled the window with possible focusable
	// widgets. EnsureFocus was automatically called when the window was activated (by
	// adding the window to the root), but then we had nothing to focus.
	// Alternatively, we could add the window after setting it up properly.
	ensureFocus();
}

bool DemoWindow::onEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_KEY_DOWN && ev.special_key == TB_KEY_ESC)
	{
		// We could call Die() to fade away and die, but click the close button instead.
		// That way the window has a chance of intercepting the close and f.ex ask if it really should be closed.
		TBWidgetEvent click_ev(EVENT_TYPE_CLICK);
		m_close_button.invokeEvent(click_ev);
		return true;
	}
	return TBWindow::onEvent(ev);
}

// == EditWindow ==============================================================

EditWindow::EditWindow(TBWidget *root) : DemoWindow(root)
{
	loadResourceFile("demo01/ui_resources/test_textwindow.tb.txt");
}
void EditWindow::onProcessStates()
{
	// Update the disabled state of undo/redo buttons, and caret info.

	if (TBEditField *edit = getWidgetByIDAndType<TBEditField>(TBIDC("editfield")))
	{
		if (TBWidget *undo = getWidgetByID("undo"))
			undo->setState(WIDGET_STATE_DISABLED, !edit->getStyleEdit()->canUndo());
		if (TBWidget *redo = getWidgetByID("redo"))
			redo->setState(WIDGET_STATE_DISABLED, !edit->getStyleEdit()->canRedo());
		if (TBTextField *info = getWidgetByIDAndType<TBTextField>(TBIDC("info")))
		{
			TBStr text;
			text.setFormatted("Caret ofs: %d", edit->getStyleEdit()->caret.getGlobalOfs());
			info->setText(text);
		}
	}
}
bool EditWindow::onEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CLICK)
	{
		TBEditField *edit = getWidgetByIDAndType<TBEditField>(TBIDC("editfield"));
		if (!edit)
			return false;

		if (ev.target->getID() == TBIDC("clear"))
		{
			edit->setText("");
			return true;
		}
		else if (ev.target->getID() == TBIDC("undo"))
		{
			edit->getStyleEdit()->undo();
			return true;
		}
		else if (ev.target->getID() == TBIDC("redo"))
		{
			edit->getStyleEdit()->redo();
			return true;
		}
		else if (ev.target->getID() == TBIDC("menu"))
		{
			static TBGenericStringItemSource source;
			if (!source.getNumItems())
			{
				source.addItem(new TBGenericStringItem("Default font", TBIDC("default font")));
				source.addItem(new TBGenericStringItem("Default font (larger)", TBIDC("large font")));
				source.addItem(new TBGenericStringItem("RGB font (Neon)", TBIDC("rgb font Neon")));
				source.addItem(new TBGenericStringItem("RGB font (Orangutang)", TBIDC("rgb font Orangutang")));
				source.addItem(new TBGenericStringItem("RGB font (Orange)", TBIDC("rgb font Orange")));
				source.addItem(new TBGenericStringItem("-"));
				source.addItem(new TBGenericStringItem("Glyph cache stresstest (CJK)", TBIDC("CJK")));
				source.addItem(new TBGenericStringItem("-"));
				source.addItem(new TBGenericStringItem("Toggle wrapping", TBIDC("toggle wrapping")));
				source.addItem(new TBGenericStringItem("-"));
				source.addItem(new TBGenericStringItem("Align left", TBIDC("align left")));
				source.addItem(new TBGenericStringItem("Align center", TBIDC("align center")));
				source.addItem(new TBGenericStringItem("Align right", TBIDC("align right")));
			}

			if (TBMenuWindow *menu = new TBMenuWindow(ev.target, TBIDC("popup_menu")))
				menu->show(&source, TBPopupAlignment());
			return true;
		}
		else if (ev.target->getID() == TBIDC("popup_menu"))
		{
			if (ev.ref_id == TBIDC("default font"))
				edit->setFontDescription(TBFontDescription());
			else if (ev.ref_id == TBIDC("large font"))
			{
				TBFontDescription fd = g_font_manager->getDefaultFontDescription();
				fd.setSize(28);
				edit->setFontDescription(fd);
			}
			else if (ev.ref_id == TBIDC("rgb font Neon"))
			{
				TBFontDescription fd = edit->getCalculatedFontDescription();
				fd.setID(TBIDC("Neon"));
				edit->setFontDescription(fd);
			}
			else if (ev.ref_id == TBIDC("rgb font Orangutang"))
			{
				TBFontDescription fd = edit->getCalculatedFontDescription();
				fd.setID(TBIDC("Orangutang"));
				edit->setFontDescription(fd);
			}
			else if (ev.ref_id == TBIDC("rgb font Orange"))
			{
				TBFontDescription fd = edit->getCalculatedFontDescription();
				fd.setID(TBIDC("Orange"));
				edit->setFontDescription(fd);
			}
			else if (ev.ref_id == TBIDC("CJK"))
			{
				TBTempBuffer buf;
				for (int i = 0, cp = 0x4E00; cp <= 0x9FCC; cp++, i++)
				{
					char utf8[8];
					int len = utf8::encode(cp, utf8);
					buf.append(utf8, len);
					if (i % 64 == 63)
						buf.append("\n", 1);
				}
				edit->getStyleEdit()->setText(buf.getData(), buf.getAppendPos());
			}
			else if (ev.ref_id == TBIDC("toggle wrapping"))
				edit->setWrapping(!edit->getWrapping());
			else if (ev.ref_id == TBIDC("align left"))
				edit->setTextAlign(TB_TEXT_ALIGN_LEFT);
			else if (ev.ref_id == TBIDC("align center"))
				edit->setTextAlign(TB_TEXT_ALIGN_CENTER);
			else if (ev.ref_id == TBIDC("align right"))
				edit->setTextAlign(TB_TEXT_ALIGN_RIGHT);
			return true;
		}
	}
	return DemoWindow::onEvent(ev);
}

// == LayoutWindow ============================================================

LayoutWindow::LayoutWindow(TBWidget *root, const char *filename) : DemoWindow(root)
{
	loadResourceFile(filename);
}

bool LayoutWindow::onEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CHANGED && ev.target->getID() == TBIDC("select position"))
	{
		LAYOUT_POSITION pos = LAYOUT_POSITION_CENTER;
		if (TBSelectDropdown *select = getWidgetByIDAndType<TBSelectDropdown>(TBIDC("select position")))
			pos = static_cast<LAYOUT_POSITION>(select->getValue());
		for (int i = 0; i < 3; i++)
			if (TBLayout *layout = getWidgetByIDAndType<TBLayout>(i + 1))
				layout->setLayoutPosition(pos);
		return true;
	}
	else if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("toggle axis"))
	{
		static AXIS axis = AXIS_Y;
		for (int i = 0; i < 3; i++)
			if (TBLayout *layout = getWidgetByIDAndType<TBLayout>(i + 1))
				layout->setAxis(axis);
		axis = axis == AXIS_X ? AXIS_Y : AXIS_X;
		if (TBLayout *layout = getWidgetByIDAndType<TBLayout>(TBIDC("switch_layout")))
			layout->setAxis(axis);
		resizeToFitContent(RESIZE_FIT_CURRENT_OR_NEEDED);
		return true;
	}
	return DemoWindow::onEvent(ev);
}

// == TabContainerWindow ============================================================

TabContainerWindow::TabContainerWindow(TBWidget *root) : DemoWindow(root)
{
	loadResourceFile("demo01/ui_resources/test_tabcontainer01.tb.txt");
}

bool TabContainerWindow::onEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("set_align"))
	{
		if (TBTabContainer *tc = getWidgetByIDAndType<TBTabContainer>(TBIDC("tabcontainer")))
			tc->setAlignment(static_cast<TB_ALIGN>(ev.target->data.getInt()));
		resizeToFitContent(RESIZE_FIT_CURRENT_OR_NEEDED);
	}
	else if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("toggle_tab_axis"))
	{
		static AXIS axis = AXIS_X;
		axis = axis == AXIS_X ? AXIS_Y : AXIS_X;
		if (TBTabContainer *tc = getWidgetByIDAndType<TBTabContainer>(TBIDC("tabcontainer")))
		{
			for (TBWidget *child = tc->getTabLayout()->getFirstChild(); child; child = child->getNext())
			{
				if (TBButton *button = TBSafeCast<TBButton>(child))
					button->setAxis(axis);
			}
		}
		resizeToFitContent(RESIZE_FIT_CURRENT_OR_NEEDED);
	}
	else if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("start_spinner"))
	{
		if (TBProgressSpinner *spinner = getWidgetByIDAndType<TBProgressSpinner>(TBIDC("spinner")))
			spinner->setValue(1);
	}
	else if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("stop_spinner"))
	{
		if (TBProgressSpinner *spinner = getWidgetByIDAndType<TBProgressSpinner>(TBIDC("spinner")))
			spinner->setValue(0);
	}
	return DemoWindow::onEvent(ev);
}

// == ConnectionWindow =========================================================

ConnectionWindow::ConnectionWindow(TBWidget *root) : DemoWindow(root)
{
	loadResourceFile("demo01/ui_resources/test_connections.tb.txt");
}

bool ConnectionWindow::onEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("reset-master-volume"))
	{
		if (TBWidgetValue *val = g_value_group.getValue(TBIDC("master-volume")))
			val->setInt(50);
	}
	else if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("reset-user-name"))
	{
		if (TBWidgetValue *val = g_value_group.getValue(TBIDC("user-name")))
			val->setText("");
	}
	return DemoWindow::onEvent(ev);
}

// == ScrollContainerWindow ===================================================

ScrollContainerWindow::ScrollContainerWindow(TBWidget *root) : DemoWindow(root)
{
	loadResourceFile("demo01/ui_resources/test_scrollcontainer.tb.txt");

	if (TBSelectDropdown *select = getWidgetByIDAndType<TBSelectDropdown>(TBIDC("name dropdown")))
		select->setSource(&name_source);

	if (TBSelectDropdown *select = getWidgetByIDAndType<TBSelectDropdown>(TBIDC("advanced dropdown")))
		select->setSource(&advanced_source);
}

bool ScrollContainerWindow::onEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CLICK)
	{
		if (ev.target->getID() == TBIDC("add img"))
		{
			TBButton *button = TBSafeCast<TBButton>(ev.target);
			TBSkinImage *skin_image = new TBSkinImage;
			skin_image->setSkinBg(TBIDC("Icon16"));
			button->getContentRoot()->addChild(skin_image, WIDGET_Z_BOTTOM);
			return true;
		}
		else if (ev.target->getID() == TBIDC("new buttons"))
		{
			for(int i = 0; i < ev.target->data.getInt(); i++)
			{
				TBStr str;
				str.setFormatted("Remove %d", i);
				TBButton *button = new TBButton;
				button->setID(TBIDC("remove button"));
				button->setText(str);
				ev.target->getParent()->addChild(button);
			}
			return true;
		}
		else if (ev.target->getID() == TBIDC("new buttons delayed"))
		{
			for(int i = 0; i < ev.target->data.getInt(); i++)
			{
				TBMessageData *data = new TBMessageData();
				data->id1 = ev.target->getParent()->getID();
				data->v1.setInt(i);
				postMessageDelayed(TBIDC("new button"), data, 100 + i * 500);
			}
			return true;
		}
		else if (ev.target->getID() == TBIDC("remove button"))
		{
			ev.target->getParent()->removeChild(ev.target);
			delete ev.target;
			return true;
		}
		else if (ev.target->getID() == TBIDC("showpopupmenu1"))
		{
			if (TBMenuWindow *menu = new TBMenuWindow(ev.target, TBIDC("popupmenu1")))
				menu->show(&popup_menu_source, TBPopupAlignment());
			return true;
		}
		else if (ev.target->getID() == TBIDC("popupmenu1"))
		{
			TBStr str;
			str.setFormatted("Menu event received!\nref_id: %d", (int)ev.ref_id);
			TBMessageWindow *msg_win = new TBMessageWindow(this, TBIDC("popup_dialog"));
			msg_win->show("Info", str);
			return true;
		}
	}
	return DemoWindow::onEvent(ev);
}

void ScrollContainerWindow::onMessageReceived(TBMessage *msg)
{
	if (msg->message == TBIDC("new button") && msg->data)
	{
		if (TBWidget *target = getWidgetByID(msg->data->id1))
		{
			TBStr str;
			str.setFormatted("Remove %d", msg->data->v1.getInt());
			TBButton *button = new TBButton;
			button->setID(TBIDC("remove button"));
			button->setText(str);
			target->addChild(button);
		}
	}
}

// == ImageWindow =============================================================

ImageWindow::ImageWindow(TBWidget *root) : DemoWindow(root)
{
	loadResourceFile("demo01/ui_resources/test_image_widget.tb.txt");
}

bool ImageWindow::onEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("remove"))
	{
		TBWidget *image = ev.target->getParent();
		image->getParent()->removeChild(image);
		delete image;
		return true;
	}
	return DemoWindow::onEvent(ev);
}

// == PageWindow =============================================================

PageWindow::PageWindow(TBWidget *root) : DemoWindow(root)
{
	loadResourceFile("demo01/ui_resources/test_scroller_snap.tb.txt");

	// Listen to the pagers scroller
	if (TBWidget *pager = getWidgetByID(TBIDC("page-scroller")))
		pager->getScroller()->setSnapListener(this);
}

bool PageWindow::onEvent(const TBWidgetEvent &ev)
{
	return DemoWindow::onEvent(ev);
}

void PageWindow::onScrollSnap(TBWidget *targetWidget, int &targetX, int &targetY)
{
	int page_w = targetWidget->getPaddingRect().w;
	int target_page = (targetX + page_w / 2) / page_w;
	targetX = target_page * page_w;
}

// == AnimationsWindow ========================================================

AnimationsWindow::AnimationsWindow(TBWidget *root) : DemoWindow(root)
{
	loadResourceFile("demo01/ui_resources/test_animations.tb.txt");
	animate();
}

void AnimationsWindow::animate()
{
	// Abort any still unfinished animations.
	TBWidgetsAnimationManager::abortAnimations(this);

	ANIMATION_CURVE curve = ANIMATION_CURVE_SLOW_DOWN;
	double duration = 500;
	bool fade = true;

	if (TBSelectList *curve_select = getWidgetByIDAndType<TBSelectList>("curve"))
		curve = static_cast<ANIMATION_CURVE>(curve_select->getValue());
	if (TBInlineSelect *duration_select = getWidgetByIDAndType<TBInlineSelect>("duration"))
		duration = duration_select->getValueDouble();
	if (TBCheckBox *fade_check = getWidgetByIDAndType<TBCheckBox>("fade"))
		fade = fade_check->getValue() ? true : false;

	// Start move animation
	if (TBAnimationObject *anim = new TBWidgetAnimationRect(this, getRect().offset(-getRect().x - getRect().w, 0), getRect()))
		TBAnimationManager::startAnimation(anim, curve, duration);
	// Start fade animation
	if (fade)
	{
		if (TBAnimationObject *anim = new TBWidgetAnimationOpacity(this, TB_ALMOST_ZERO_OPACITY, 1, false))
			TBAnimationManager::startAnimation(anim, ANIMATION_CURVE_SLOW_DOWN, duration);
	}
}

bool AnimationsWindow::onEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("Animate!"))
		animate();
	return DemoWindow::onEvent(ev);
}

// == MainWindow ==============================================================

MainWindow::MainWindow(TBWidget *root) : DemoWindow(root)
{
	loadResourceFile("demo01/ui_resources/test_ui.tb.txt");

	setOpacity(0.97f);
}

void MainWindow::onMessageReceived(TBMessage *msg)
{
	if (msg->message == TBIDC("instantmsg"))
	{
		TBMessageWindow *msg_win = new TBMessageWindow(this, TBIDC("test_dialog"));
		msg_win->show("Message window", "Instant message received!");
	}
	else if (msg->message == TBIDC("busy"))
	{
		// Keep the message queue busy by posting another "busy" message.
		postMessage(TBIDC("busy"), nullptr);
	}
	else if (msg->message == TBIDC("delayedmsg"))
	{
		TBStr text;
		text.setFormatted("Delayed message received!\n\n"
							"It was received %d ms after its intended fire time.",
							(int)(TBSystem::getTimeMS() - msg->getFireTime()));
		TBMessageWindow *msg_win = new TBMessageWindow(this, TBIDC(""));
		msg_win->show("Message window", text);
	}
}

bool MainWindow::onEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CLICK)
	{
		if (ev.target->getID() == TBIDC("new"))
		{
			new MainWindow(getParentRoot());
			return true;
		}
		if (ev.target->getID() == TBIDC("msg"))
		{
			postMessage(TBIDC("instantmsg"), nullptr);
			return true;
		}
		else if (ev.target->getID() == TBIDC("busymsg"))
		{
			if (ev.target->getValue() == 1)
			{
				// Post the first "busy" message when we check the checkbox.
				core_assert(!getMessageByID(TBIDC("busy")));
				if (!getMessageByID(TBIDC("busy")))
				{
					postMessage(TBIDC("busy"), nullptr);
					TBMessageWindow *msg_win = new TBMessageWindow(this, TBIDC("test_dialog"));
					msg_win->show("Message window", "The message loop is now constantly busy with messages to process.\n\n"
								"The main thread should be working hard, but input & animations should still be running smoothly.");
				}
			}
			else
			{
				// Remove any pending "busy" message when we uncheck the checkbox.
				core_assert(getMessageByID(TBIDC("busy")));
				if (TBMessage *busymsg = getMessageByID(TBIDC("busy")))
					deleteMessage(busymsg);
			}
			return true;
		}
		else if (ev.target->getID() == TBIDC("delayedmsg"))
		{
			postMessageDelayed(TBIDC("delayedmsg"), nullptr, 2000);
			return true;
		}
		else if (ev.target->getID() == TBIDC("TBWindow.close"))
		{
			// Intercept the TBWindow.close message and stop it from bubbling
			// to TBWindow (prevent the window from closing)
			TBMessageWindow *msg_win = new TBMessageWindow(this, TBIDC("confirm_close_dialog"));
			TBMessageWindowSettings settings(TB_MSG_YES_NO, TBIDC("Icon48"));
			settings.dimmer = true;
			settings.styling = true;
			msg_win->show("Are you sure?", "Really <color #0794f8>close</color> the window?", &settings);
			return true;
		}
		else if (ev.target->getID() == TBIDC("confirm_close_dialog"))
		{
			if (ev.ref_id == TBIDC("TBMessageWindow.yes"))
				close();
			return true;
		}
		else if (ev.target->getID() == TBIDC("reload skin bitmaps"))
		{
			int reload_count = 10;
			double t1 = TBSystem::getTimeMS();
			for (int i = 0; i < reload_count; i++)
				g_tb_skin->reloadBitmaps();
			double t2 = TBSystem::getTimeMS();

			TBStr message;
			message.setFormatted("Reloading the skin graphics %d times took %dms", reload_count, (int)(t2 - t1));
			TBMessageWindow *msg_win = new TBMessageWindow(ev.target, TBID());
			msg_win->show("GFX load performance", message);
			return true;
		}
		else if (ev.target->getID() == TBIDC("test context lost"))
		{
			g_renderer->invokeContextLost();
			g_renderer->invokeContextRestored();
			TBMessageWindow *msg_win = new TBMessageWindow(ev.target, TBID());
			msg_win->show("Context lost & restore",
							"Called InvokeContextLost and InvokeContextRestored.\n\n"
							"Does everything look fine?");
			return true;
		}
		else if (ev.target->getID() == TBIDC("test-layout"))
		{
			TBStr resource_file("demo01/ui_resources/");
			resource_file.append(ev.target->data.getString());
			new LayoutWindow(getParentRoot(), resource_file);
			return true;
		}
		else if (ev.target->getID() == TBIDC("test-connections"))
		{
			new ConnectionWindow(getParentRoot());
			return true;
		}
		else if (ev.target->getID() == TBIDC("test-list"))
		{
			new AdvancedListWindow(getParentRoot(), &advanced_source);
			return true;
		}
		else if (ev.target->getID() == TBIDC("test-image"))
		{
			new ImageWindow(getParentRoot());
			return true;
		}
		else if (ev.target->getID() == TBIDC("test-page"))
		{
			new PageWindow(getParentRoot());
			return true;
		}
		else if (ev.target->getID() == TBIDC("test-animations"))
		{
			new AnimationsWindow(getParentRoot());
			return true;
		}
		else if (ev.target->getID() == TBIDC("test-scroll-container"))
		{
			new ScrollContainerWindow(getParentRoot());
			return true;
		}
		else if (ev.target->getID() == TBIDC("test-skin-conditions"))
		{
			(new DemoWindow(getParentRoot()))->loadResourceFile("demo01/ui_resources/test_skin_conditions01.tb.txt");
			(new DemoWindow(getParentRoot()))->loadResourceFile("demo01/ui_resources/test_skin_conditions02.tb.txt");
			return true;
		}
		else if (ev.target->getID() == TBIDC("test-resource-edit"))
		{
			ResourceEditWindow *res_edit_win = new ResourceEditWindow();
			res_edit_win->load("demo01/ui_resources/resource_edit_test.tb.txt");
			getParent()->addChild(res_edit_win);
			return true;
		}
		else if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("debug settings"))
		{
#ifdef TB_RUNTIME_DEBUG_INFO
			ShowDebugInfoSettingsWindow(getParentRoot());
#else
			TBMessageWindow *msg_win = new TBMessageWindow(ev.target, TBID());
			msg_win->show("Debug settings",
							"Debug settings is only available in builds "
							"compiled with TB_RUNTIME_DEBUG_INFO defined.\n\n"
							"Debug builds enable this by default.");
#endif
			return true;
		}
	}
	return DemoWindow::onEvent(ev);
}
