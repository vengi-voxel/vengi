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
	root->AddChild(this);
}

bool DemoWindow::LoadResourceFile(const char *filename)
{
	// We could do g_widgets_reader->LoadFile(this, filename) but we want
	// some extra data we store under "WindowInfo", so read into node tree.
	TBNode node;
	if (!node.ReadFile(filename))
		return false;
	LoadResource(node);
	return true;
}

void DemoWindow::LoadResourceData(const char *data)
{
	// We could do g_widgets_reader->LoadData(this, filename) but we want
	// some extra data we store under "WindowInfo", so read into node tree.
	TBNode node;
	node.ReadData(data);
	LoadResource(node);
}

void DemoWindow::LoadResource(TBNode &node)
{
	g_widgets_reader->LoadNodeTree(this, &node);

	// Get title from the WindowInfo section (or use "" if not specified)
	SetText(node.GetValueString("WindowInfo>title", ""));

	const TBRect parent_rect(0, 0, GetParent()->GetRect().w, GetParent()->GetRect().h);
	const TBDimensionConverter *dc = g_tb_skin->GetDimensionConverter();
	TBRect window_rect = GetResizeToFitContentRect();

	// Use specified size or adapt to the preferred content size.
	TBNode *tmp = node.GetNode("WindowInfo>size");
	if (tmp && tmp->GetValue().GetArrayLength() == 2)
	{
		window_rect.w = dc->GetPxFromString(tmp->GetValue().GetArray()->GetValue(0)->GetString(), window_rect.w);
		window_rect.h = dc->GetPxFromString(tmp->GetValue().GetArray()->GetValue(1)->GetString(), window_rect.h);
	}

	// Use the specified position or center in parent.
	tmp = node.GetNode("WindowInfo>position");
	if (tmp && tmp->GetValue().GetArrayLength() == 2)
	{
		window_rect.x = dc->GetPxFromString(tmp->GetValue().GetArray()->GetValue(0)->GetString(), window_rect.x);
		window_rect.y = dc->GetPxFromString(tmp->GetValue().GetArray()->GetValue(1)->GetString(), window_rect.y);
	}
	else
		window_rect = window_rect.CenterIn(parent_rect);

	// Make sure the window is inside the parent, and not larger.
	window_rect = window_rect.MoveIn(parent_rect).Clip(parent_rect);

	SetRect(window_rect);

	// Ensure we have focus - now that we've filled the window with possible focusable
	// widgets. EnsureFocus was automatically called when the window was activated (by
	// adding the window to the root), but then we had nothing to focus.
	// Alternatively, we could add the window after setting it up properly.
	EnsureFocus();
}

bool DemoWindow::OnEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_KEY_DOWN && ev.special_key == TB_KEY_ESC)
	{
		// We could call Die() to fade away and die, but click the close button instead.
		// That way the window has a chance of intercepting the close and f.ex ask if it really should be closed.
		TBWidgetEvent click_ev(EVENT_TYPE_CLICK);
		m_close_button.InvokeEvent(click_ev);
		return true;
	}
	return TBWindow::OnEvent(ev);
}

// == EditWindow ==============================================================

EditWindow::EditWindow(TBWidget *root) : DemoWindow(root)
{
	LoadResourceFile("demo01/ui_resources/test_textwindow.tb.txt");
}
void EditWindow::OnProcessStates()
{
	// Update the disabled state of undo/redo buttons, and caret info.

	if (TBEditField *edit = GetWidgetByIDAndType<TBEditField>(TBIDC("editfield")))
	{
		if (TBWidget *undo = GetWidgetByID("undo"))
			undo->SetState(WIDGET_STATE_DISABLED, !edit->GetStyleEdit()->CanUndo());
		if (TBWidget *redo = GetWidgetByID("redo"))
			redo->SetState(WIDGET_STATE_DISABLED, !edit->GetStyleEdit()->CanRedo());
		if (TBTextField *info = GetWidgetByIDAndType<TBTextField>(TBIDC("info")))
		{
			TBStr text;
			text.SetFormatted("Caret ofs: %d", edit->GetStyleEdit()->caret.GetGlobalOfs());
			info->SetText(text);
		}
	}
}
bool EditWindow::OnEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CLICK)
	{
		TBEditField *edit = GetWidgetByIDAndType<TBEditField>(TBIDC("editfield"));
		if (!edit)
			return false;

		if (ev.target->GetID() == TBIDC("clear"))
		{
			edit->SetText("");
			return true;
		}
		else if (ev.target->GetID() == TBIDC("undo"))
		{
			edit->GetStyleEdit()->Undo();
			return true;
		}
		else if (ev.target->GetID() == TBIDC("redo"))
		{
			edit->GetStyleEdit()->Redo();
			return true;
		}
		else if (ev.target->GetID() == TBIDC("menu"))
		{
			static TBGenericStringItemSource source;
			if (!source.GetNumItems())
			{
				source.AddItem(new TBGenericStringItem("Default font", TBIDC("default font")));
				source.AddItem(new TBGenericStringItem("Default font (larger)", TBIDC("large font")));
				source.AddItem(new TBGenericStringItem("RGB font (Neon)", TBIDC("rgb font Neon")));
				source.AddItem(new TBGenericStringItem("RGB font (Orangutang)", TBIDC("rgb font Orangutang")));
				source.AddItem(new TBGenericStringItem("RGB font (Orange)", TBIDC("rgb font Orange")));
				source.AddItem(new TBGenericStringItem("-"));
				source.AddItem(new TBGenericStringItem("Glyph cache stresstest (CJK)", TBIDC("CJK")));
				source.AddItem(new TBGenericStringItem("-"));
				source.AddItem(new TBGenericStringItem("Toggle wrapping", TBIDC("toggle wrapping")));
				source.AddItem(new TBGenericStringItem("-"));
				source.AddItem(new TBGenericStringItem("Align left", TBIDC("align left")));
				source.AddItem(new TBGenericStringItem("Align center", TBIDC("align center")));
				source.AddItem(new TBGenericStringItem("Align right", TBIDC("align right")));
			}

			if (TBMenuWindow *menu = new TBMenuWindow(ev.target, TBIDC("popup_menu")))
				menu->Show(&source, TBPopupAlignment());
			return true;
		}
		else if (ev.target->GetID() == TBIDC("popup_menu"))
		{
			if (ev.ref_id == TBIDC("default font"))
				edit->SetFontDescription(TBFontDescription());
			else if (ev.ref_id == TBIDC("large font"))
			{
				TBFontDescription fd = g_font_manager->GetDefaultFontDescription();
				fd.SetSize(28);
				edit->SetFontDescription(fd);
			}
			else if (ev.ref_id == TBIDC("rgb font Neon"))
			{
				TBFontDescription fd = edit->GetCalculatedFontDescription();
				fd.SetID(TBIDC("Neon"));
				edit->SetFontDescription(fd);
			}
			else if (ev.ref_id == TBIDC("rgb font Orangutang"))
			{
				TBFontDescription fd = edit->GetCalculatedFontDescription();
				fd.SetID(TBIDC("Orangutang"));
				edit->SetFontDescription(fd);
			}
			else if (ev.ref_id == TBIDC("rgb font Orange"))
			{
				TBFontDescription fd = edit->GetCalculatedFontDescription();
				fd.SetID(TBIDC("Orange"));
				edit->SetFontDescription(fd);
			}
			else if (ev.ref_id == TBIDC("CJK"))
			{
				TBTempBuffer buf;
				for (int i = 0, cp = 0x4E00; cp <= 0x9FCC; cp++, i++)
				{
					char utf8[8];
					int len = utf8::encode(cp, utf8);
					buf.Append(utf8, len);
					if (i % 64 == 63)
						buf.Append("\n", 1);
				}
				edit->GetStyleEdit()->SetText(buf.GetData(), buf.GetAppendPos());
			}
			else if (ev.ref_id == TBIDC("toggle wrapping"))
				edit->SetWrapping(!edit->GetWrapping());
			else if (ev.ref_id == TBIDC("align left"))
				edit->SetTextAlign(TB_TEXT_ALIGN_LEFT);
			else if (ev.ref_id == TBIDC("align center"))
				edit->SetTextAlign(TB_TEXT_ALIGN_CENTER);
			else if (ev.ref_id == TBIDC("align right"))
				edit->SetTextAlign(TB_TEXT_ALIGN_RIGHT);
			return true;
		}
	}
	return DemoWindow::OnEvent(ev);
}

// == LayoutWindow ============================================================

LayoutWindow::LayoutWindow(TBWidget *root, const char *filename) : DemoWindow(root)
{
	LoadResourceFile(filename);
}

bool LayoutWindow::OnEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CHANGED && ev.target->GetID() == TBIDC("select position"))
	{
		LAYOUT_POSITION pos = LAYOUT_POSITION_CENTER;
		if (TBSelectDropdown *select = GetWidgetByIDAndType<TBSelectDropdown>(TBIDC("select position")))
			pos = static_cast<LAYOUT_POSITION>(select->GetValue());
		for (int i = 0; i < 3; i++)
			if (TBLayout *layout = GetWidgetByIDAndType<TBLayout>(i + 1))
				layout->SetLayoutPosition(pos);
		return true;
	}
	else if (ev.type == EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("toggle axis"))
	{
		static AXIS axis = AXIS_Y;
		for (int i = 0; i < 3; i++)
			if (TBLayout *layout = GetWidgetByIDAndType<TBLayout>(i + 1))
				layout->SetAxis(axis);
		axis = axis == AXIS_X ? AXIS_Y : AXIS_X;
		if (TBLayout *layout = GetWidgetByIDAndType<TBLayout>(TBIDC("switch_layout")))
			layout->SetAxis(axis);
		ResizeToFitContent(RESIZE_FIT_CURRENT_OR_NEEDED);
		return true;
	}
	return DemoWindow::OnEvent(ev);
}

// == TabContainerWindow ============================================================

TabContainerWindow::TabContainerWindow(TBWidget *root) : DemoWindow(root)
{
	LoadResourceFile("demo01/ui_resources/test_tabcontainer01.tb.txt");
}

bool TabContainerWindow::OnEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("set_align"))
	{
		if (TBTabContainer *tc = GetWidgetByIDAndType<TBTabContainer>(TBIDC("tabcontainer")))
			tc->SetAlignment(static_cast<TB_ALIGN>(ev.target->data.GetInt()));
		ResizeToFitContent(RESIZE_FIT_CURRENT_OR_NEEDED);
	}
	else if (ev.type == EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("toggle_tab_axis"))
	{
		static AXIS axis = AXIS_X;
		axis = axis == AXIS_X ? AXIS_Y : AXIS_X;
		if (TBTabContainer *tc = GetWidgetByIDAndType<TBTabContainer>(TBIDC("tabcontainer")))
		{
			for (TBWidget *child = tc->GetTabLayout()->GetFirstChild(); child; child = child->GetNext())
			{
				if (TBButton *button = TBSafeCast<TBButton>(child))
					button->SetAxis(axis);
			}
		}
		ResizeToFitContent(RESIZE_FIT_CURRENT_OR_NEEDED);
	}
	else if (ev.type == EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("start_spinner"))
	{
		if (TBProgressSpinner *spinner = GetWidgetByIDAndType<TBProgressSpinner>(TBIDC("spinner")))
			spinner->SetValue(1);
	}
	else if (ev.type == EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("stop_spinner"))
	{
		if (TBProgressSpinner *spinner = GetWidgetByIDAndType<TBProgressSpinner>(TBIDC("spinner")))
			spinner->SetValue(0);
	}
	return DemoWindow::OnEvent(ev);
}

// == ConnectionWindow =========================================================

ConnectionWindow::ConnectionWindow(TBWidget *root) : DemoWindow(root)
{
	LoadResourceFile("demo01/ui_resources/test_connections.tb.txt");
}

bool ConnectionWindow::OnEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("reset-master-volume"))
	{
		if (TBWidgetValue *val = g_value_group.GetValue(TBIDC("master-volume")))
			val->SetInt(50);
	}
	else if (ev.type == EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("reset-user-name"))
	{
		if (TBWidgetValue *val = g_value_group.GetValue(TBIDC("user-name")))
			val->SetText("");
	}
	return DemoWindow::OnEvent(ev);
}

// == ScrollContainerWindow ===================================================

ScrollContainerWindow::ScrollContainerWindow(TBWidget *root) : DemoWindow(root)
{
	LoadResourceFile("demo01/ui_resources/test_scrollcontainer.tb.txt");

	if (TBSelectDropdown *select = GetWidgetByIDAndType<TBSelectDropdown>(TBIDC("name dropdown")))
		select->SetSource(&name_source);

	if (TBSelectDropdown *select = GetWidgetByIDAndType<TBSelectDropdown>(TBIDC("advanced dropdown")))
		select->SetSource(&advanced_source);
}

bool ScrollContainerWindow::OnEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CLICK)
	{
		if (ev.target->GetID() == TBIDC("add img"))
		{
			TBButton *button = TBSafeCast<TBButton>(ev.target);
			TBSkinImage *skin_image = new TBSkinImage;
			skin_image->SetSkinBg(TBIDC("Icon16"));
			button->GetContentRoot()->AddChild(skin_image, WIDGET_Z_BOTTOM);
			return true;
		}
		else if (ev.target->GetID() == TBIDC("new buttons"))
		{
			for(int i = 0; i < ev.target->data.GetInt(); i++)
			{
				TBStr str;
				str.SetFormatted("Remove %d", i);
				TBButton *button = new TBButton;
				button->SetID(TBIDC("remove button"));
				button->SetText(str);
				ev.target->GetParent()->AddChild(button);
			}
			return true;
		}
		else if (ev.target->GetID() == TBIDC("new buttons delayed"))
		{
			for(int i = 0; i < ev.target->data.GetInt(); i++)
			{
				TBMessageData *data = new TBMessageData();
				data->id1 = ev.target->GetParent()->GetID();
				data->v1.SetInt(i);
				PostMessageDelayed(TBIDC("new button"), data, 100 + i * 500);
			}
			return true;
		}
		else if (ev.target->GetID() == TBIDC("remove button"))
		{
			ev.target->GetParent()->RemoveChild(ev.target);
			delete ev.target;
			return true;
		}
		else if (ev.target->GetID() == TBIDC("showpopupmenu1"))
		{
			if (TBMenuWindow *menu = new TBMenuWindow(ev.target, TBIDC("popupmenu1")))
				menu->Show(&popup_menu_source, TBPopupAlignment());
			return true;
		}
		else if (ev.target->GetID() == TBIDC("popupmenu1"))
		{
			TBStr str;
			str.SetFormatted("Menu event received!\nref_id: %d", (int)ev.ref_id);
			TBMessageWindow *msg_win = new TBMessageWindow(this, TBIDC("popup_dialog"));
			msg_win->Show("Info", str);
			return true;
		}
	}
	return DemoWindow::OnEvent(ev);
}

void ScrollContainerWindow::OnMessageReceived(TBMessage *msg)
{
	if (msg->message == TBIDC("new button") && msg->data)
	{
		if (TBWidget *target = GetWidgetByID(msg->data->id1))
		{
			TBStr str;
			str.SetFormatted("Remove %d", msg->data->v1.GetInt());
			TBButton *button = new TBButton;
			button->SetID(TBIDC("remove button"));
			button->SetText(str);
			target->AddChild(button);
		}
	}
}

// == ImageWindow =============================================================

ImageWindow::ImageWindow(TBWidget *root) : DemoWindow(root)
{
	LoadResourceFile("demo01/ui_resources/test_image_widget.tb.txt");
}

bool ImageWindow::OnEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("remove"))
	{
		TBWidget *image = ev.target->GetParent();
		image->GetParent()->RemoveChild(image);
		delete image;
		return true;
	}
	return DemoWindow::OnEvent(ev);
}

// == PageWindow =============================================================

PageWindow::PageWindow(TBWidget *root) : DemoWindow(root)
{
	LoadResourceFile("demo01/ui_resources/test_scroller_snap.tb.txt");

	// Listen to the pagers scroller
	if (TBWidget *pager = GetWidgetByID(TBIDC("page-scroller")))
		pager->GetScroller()->SetSnapListener(this);
}

bool PageWindow::OnEvent(const TBWidgetEvent &ev)
{
	return DemoWindow::OnEvent(ev);
}

void PageWindow::OnScrollSnap(TBWidget *target_widget, int &target_x, int &target_y)
{
	int page_w = target_widget->GetPaddingRect().w;
	int target_page = (target_x + page_w / 2) / page_w;
	target_x = target_page * page_w;
}

// == AnimationsWindow ========================================================

AnimationsWindow::AnimationsWindow(TBWidget *root) : DemoWindow(root)
{
	LoadResourceFile("demo01/ui_resources/test_animations.tb.txt");
	Animate();
}

void AnimationsWindow::Animate()
{
	// Abort any still unfinished animations.
	TBWidgetsAnimationManager::AbortAnimations(this);

	ANIMATION_CURVE curve = ANIMATION_CURVE_SLOW_DOWN;
	double duration = 500;
	bool fade = true;

	if (TBSelectList *curve_select = GetWidgetByIDAndType<TBSelectList>("curve"))
		curve = static_cast<ANIMATION_CURVE>(curve_select->GetValue());
	if (TBInlineSelect *duration_select = GetWidgetByIDAndType<TBInlineSelect>("duration"))
		duration = duration_select->GetValueDouble();
	if (TBCheckBox *fade_check = GetWidgetByIDAndType<TBCheckBox>("fade"))
		fade = fade_check->GetValue() ? true : false;

	// Start move animation
	if (TBAnimationObject *anim = new TBWidgetAnimationRect(this, GetRect().Offset(-GetRect().x - GetRect().w, 0), GetRect()))
		TBAnimationManager::StartAnimation(anim, curve, duration);
	// Start fade animation
	if (fade)
	{
		if (TBAnimationObject *anim = new TBWidgetAnimationOpacity(this, TB_ALMOST_ZERO_OPACITY, 1, false))
			TBAnimationManager::StartAnimation(anim, ANIMATION_CURVE_SLOW_DOWN, duration);
	}
}

bool AnimationsWindow::OnEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("Animate!"))
		Animate();
	return DemoWindow::OnEvent(ev);
}

// == MainWindow ==============================================================

MainWindow::MainWindow(TBWidget *root) : DemoWindow(root)
{
	LoadResourceFile("demo01/ui_resources/test_ui.tb.txt");

	SetOpacity(0.97f);
}

void MainWindow::OnMessageReceived(TBMessage *msg)
{
	if (msg->message == TBIDC("instantmsg"))
	{
		TBMessageWindow *msg_win = new TBMessageWindow(this, TBIDC("test_dialog"));
		msg_win->Show("Message window", "Instant message received!");
	}
	else if (msg->message == TBIDC("busy"))
	{
		// Keep the message queue busy by posting another "busy" message.
		PostMessage(TBIDC("busy"), nullptr);
	}
	else if (msg->message == TBIDC("delayedmsg"))
	{
		TBStr text;
		text.SetFormatted("Delayed message received!\n\n"
							"It was received %d ms after its intended fire time.",
							(int)(TBSystem::GetTimeMS() - msg->GetFireTime()));
		TBMessageWindow *msg_win = new TBMessageWindow(this, TBIDC(""));
		msg_win->Show("Message window", text);
	}
}

bool MainWindow::OnEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CLICK)
	{
		if (ev.target->GetID() == TBIDC("new"))
		{
			new MainWindow(GetParentRoot());
			return true;
		}
		if (ev.target->GetID() == TBIDC("msg"))
		{
			PostMessage(TBIDC("instantmsg"), nullptr);
			return true;
		}
		else if (ev.target->GetID() == TBIDC("busymsg"))
		{
			if (ev.target->GetValue() == 1)
			{
				// Post the first "busy" message when we check the checkbox.
				core_assert(!GetMessageByID(TBIDC("busy")));
				if (!GetMessageByID(TBIDC("busy")))
				{
					PostMessage(TBIDC("busy"), nullptr);
					TBMessageWindow *msg_win = new TBMessageWindow(this, TBIDC("test_dialog"));
					msg_win->Show("Message window", "The message loop is now constantly busy with messages to process.\n\n"
								"The main thread should be working hard, but input & animations should still be running smoothly.");
				}
			}
			else
			{
				// Remove any pending "busy" message when we uncheck the checkbox.
				core_assert(GetMessageByID(TBIDC("busy")));
				if (TBMessage *busymsg = GetMessageByID(TBIDC("busy")))
					DeleteMessage(busymsg);
			}
			return true;
		}
		else if (ev.target->GetID() == TBIDC("delayedmsg"))
		{
			PostMessageDelayed(TBIDC("delayedmsg"), nullptr, 2000);
			return true;
		}
		else if (ev.target->GetID() == TBIDC("TBWindow.close"))
		{
			// Intercept the TBWindow.close message and stop it from bubbling
			// to TBWindow (prevent the window from closing)
			TBMessageWindow *msg_win = new TBMessageWindow(this, TBIDC("confirm_close_dialog"));
			TBMessageWindowSettings settings(TB_MSG_YES_NO, TBIDC("Icon48"));
			settings.dimmer = true;
			settings.styling = true;
			msg_win->Show("Are you sure?", "Really <color #0794f8>close</color> the window?", &settings);
			return true;
		}
		else if (ev.target->GetID() == TBIDC("confirm_close_dialog"))
		{
			if (ev.ref_id == TBIDC("TBMessageWindow.yes"))
				Close();
			return true;
		}
		else if (ev.target->GetID() == TBIDC("reload skin bitmaps"))
		{
			int reload_count = 10;
			double t1 = TBSystem::GetTimeMS();
			for (int i = 0; i < reload_count; i++)
				g_tb_skin->ReloadBitmaps();
			double t2 = TBSystem::GetTimeMS();

			TBStr message;
			message.SetFormatted("Reloading the skin graphics %d times took %dms", reload_count, (int)(t2 - t1));
			TBMessageWindow *msg_win = new TBMessageWindow(ev.target, TBID());
			msg_win->Show("GFX load performance", message);
			return true;
		}
		else if (ev.target->GetID() == TBIDC("test context lost"))
		{
			g_renderer->InvokeContextLost();
			g_renderer->InvokeContextRestored();
			TBMessageWindow *msg_win = new TBMessageWindow(ev.target, TBID());
			msg_win->Show("Context lost & restore",
							"Called InvokeContextLost and InvokeContextRestored.\n\n"
							"Does everything look fine?");
			return true;
		}
		else if (ev.target->GetID() == TBIDC("test-layout"))
		{
			TBStr resource_file("demo01/ui_resources/");
			resource_file.Append(ev.target->data.GetString());
			new LayoutWindow(GetParentRoot(), resource_file);
			return true;
		}
		else if (ev.target->GetID() == TBIDC("test-connections"))
		{
			new ConnectionWindow(GetParentRoot());
			return true;
		}
		else if (ev.target->GetID() == TBIDC("test-list"))
		{
			new AdvancedListWindow(GetParentRoot(), &advanced_source);
			return true;
		}
		else if (ev.target->GetID() == TBIDC("test-image"))
		{
			new ImageWindow(GetParentRoot());
			return true;
		}
		else if (ev.target->GetID() == TBIDC("test-page"))
		{
			new PageWindow(GetParentRoot());
			return true;
		}
		else if (ev.target->GetID() == TBIDC("test-animations"))
		{
			new AnimationsWindow(GetParentRoot());
			return true;
		}
		else if (ev.target->GetID() == TBIDC("test-scroll-container"))
		{
			new ScrollContainerWindow(GetParentRoot());
			return true;
		}
		else if (ev.target->GetID() == TBIDC("test-skin-conditions"))
		{
			(new DemoWindow(GetParentRoot()))->LoadResourceFile("demo01/ui_resources/test_skin_conditions01.tb.txt");
			(new DemoWindow(GetParentRoot()))->LoadResourceFile("demo01/ui_resources/test_skin_conditions02.tb.txt");
			return true;
		}
		else if (ev.target->GetID() == TBIDC("test-resource-edit"))
		{
			ResourceEditWindow *res_edit_win = new ResourceEditWindow();
			res_edit_win->Load("demo01/ui_resources/resource_edit_test.tb.txt");
			GetParent()->AddChild(res_edit_win);
			return true;
		}
		else if (ev.type == EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("debug settings"))
		{
#ifdef TB_RUNTIME_DEBUG_INFO
			ShowDebugInfoSettingsWindow(GetParentRoot());
#else
			TBMessageWindow *msg_win = new TBMessageWindow(ev.target, TBID());
			msg_win->Show("Debug settings",
							"Debug settings is only available in builds "
							"compiled with TB_RUNTIME_DEBUG_INFO defined.\n\n"
							"Debug builds enable this by default.");
#endif
			return true;
		}
	}
	return DemoWindow::OnEvent(ev);
}
