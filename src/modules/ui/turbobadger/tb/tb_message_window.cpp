/**
 * @file
 */

#include "tb_message_window.h"
#include "tb_widgets_reader.h"
#include "tb_editfield.h"
#include "tb_language.h"
#include "core/Assert.h"

namespace tb {

// == TBMessageWindow =======================================================================================

TBMessageWindow::TBMessageWindow(TBWidget *target, TBID id)
	: m_target(target)
{
	TBWidgetListener::addGlobalListener(this);
	setID(id);
}

TBMessageWindow::~TBMessageWindow()
{
	TBWidgetListener::removeGlobalListener(this);
	if (TBWidget *dimmer = m_dimmer.get())
	{
		dimmer->removeFromParent();
		delete dimmer;
	}
}

bool TBMessageWindow::show(const char *title, const char *message, TBMessageWindowSettings *settings)
{
	TBWidget *target = m_target.get();
	if (!target)
		return false;

	TBMessageWindowSettings default_settings;
	if (!settings)
		settings = &default_settings;

	TBWidget *root = target->getParentRoot();

	const char *source =	"TBLayout: axis: y, distribution: available\n"
							"	TBLayout: distribution: available, size: available\n"
							"		TBSkinImage: id: 2\n"
							"		TBEditField: multiline: 1, readonly: 1, id: 1\n"
							"	TBLayout: distribution-position: right bottom, id: 3\n";
	if (!g_widgets_reader->loadData(getContentRoot(), source))
		return false;

	setText(title);

	getWidgetByIDAndType<TBSkinImage>(2)->setSkinBg(settings->icon_skin);

	TBEditField *editfield = getWidgetByIDAndType<TBEditField>(1);
	editfield->setStyling(settings->styling);
	editfield->setText(message);
	editfield->setTextAlign(settings->align);
	editfield->setSkinBg("");

	// Create buttons
	if (settings->msg == TB_MSG_OK)
	{
		addButton("TBMessageWindow.ok", true);
	}
	else if (settings->msg == TB_MSG_OK_CANCEL)
	{
		addButton("TBMessageWindow.ok", true);
		addButton("TBMessageWindow.cancel", false);
	}
	else if (settings->msg == TB_MSG_YES_NO)
	{
		addButton("TBMessageWindow.yes", true);
		addButton("TBMessageWindow.no", false);
	}
	else if (settings->msg == TB_MSG_YES_NO_CANCEL)
	{
		addButton("TBMessageWindow.yes", true);
		addButton("TBMessageWindow.no", false);
		addButton("TBMessageWindow.cancel", false);
	}

	// Size to fit content. This will use the default size of the textfield.
	resizeToFitContent();
	TBRect rect = getRect();

	// Get how much we overflow the textfield has given the current width, and grow our height to show all we can.
	// FIX: It would be better to use adapt-to-content on the editfield to achieve the most optimal size.
	// At least when we do full blown multi pass size checking.
	rect.h += editfield->getStyleEdit()->getOverflowY();

	// Create background dimmer
	if (settings->dimmer)
	{
		if (TBDimmer *dimmer = new TBDimmer)
		{
			root->addChild(dimmer);
			m_dimmer.set(dimmer);
		}
	}

	// Center and size to the new height
	TBRect bounds(0, 0, root->getRect().w, root->getRect().h);
	setRect(rect.centerIn(bounds).moveIn(bounds).clip(bounds));
	root->addChild(this);
	return true;
}

void TBMessageWindow::addButton(TBID id, bool focused)
{
	TBLayout *layout = getWidgetByIDAndType<TBLayout>(3);
	if (!layout)
		return;
	if (TBButton *btn = new TBButton)
	{
		btn->setID(id);
		btn->setText(g_tb_lng->getString(btn->getID()));
		layout->addChild(btn);
		if (focused)
			btn->setFocus(WIDGET_FOCUS_REASON_UNKNOWN);
	}
}

bool TBMessageWindow::onEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CLICK && ev.target->isOfType<TBButton>())
	{
		TBWidgetSafePointer this_widget(this);

		// Invoke the click on the target
		TBWidgetEvent target_ev(EVENT_TYPE_CLICK);
		target_ev.ref_id = ev.target->getID();
		invokeEvent(target_ev);

		// If target got deleted, close
		if (this_widget.get())
			close();
		return true;
	}
	else if (ev.type == EVENT_TYPE_KEY_DOWN && ev.special_key == TB_KEY_ESC)
	{
		TBWidgetEvent click_ev(EVENT_TYPE_CLICK);
		m_close_button.invokeEvent(click_ev);
		return true;
	}
	return TBWindow::onEvent(ev);
}

void TBMessageWindow::onDie()
{
	if (TBWidget *dimmer = m_dimmer.get())
		dimmer->die();
}

void TBMessageWindow::onWidgetDelete(TBWidget *widget)
{
	// If the target widget is deleted, close!
	if (!m_target.get())
		close();
}

bool TBMessageWindow::onWidgetDying(TBWidget *widget)
{
	// If the target widget or an ancestor of it is dying, close!
	if (widget == m_target.get() || widget->isAncestorOf(m_target.get()))
		close();
	return false;
}

} // namespace tb
