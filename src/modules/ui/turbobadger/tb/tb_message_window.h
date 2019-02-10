/**
 * @file
 */

#pragma once

#include "tb_window.h"
#include "tb_widgets_listener.h"

namespace tb {

enum TB_MSG {
	TB_MSG_OK,
	TB_MSG_OK_CANCEL,
	TB_MSG_YES_NO,
	TB_MSG_YES_NO_CANCEL,
};

/** TBMessageWindowSettings contains additional settings for TBMessageWindow. */
class TBMessageWindowSettings
{
public:
	TBMessageWindowSettings() : msg(TB_MSG_OK), dimmer(false), styling(false) {}
	TBMessageWindowSettings(TB_MSG msg, TBID icon_skin) : msg(msg), icon_skin(icon_skin), dimmer(false), styling(false), align(TB_TEXT_ALIGN_LEFT) {}
public:
	TB_MSG msg;			///< The type of response for the message.
	TBID icon_skin;		///< The icon skin (0 for no icon)
	bool dimmer;		///< Set to true to dim background widgets by a TBDimmer.
	bool styling;		///< Enable styling in the textfield.
	TB_TEXT_ALIGN align = TB_TEXT_ALIGN_LEFT;	///< Text alignment in message box.
};

/** TBMessageWindow is a window for showing simple messages.
	Events invoked in this window will travel up through the target widget.

	When the user click any of its buttons, it will invoke a click event
	(with the window ID), with the clicked buttons id as ref_id.
	Then it will delete itself.

	If the target widget is deleted while this window is alive, the
	window will delete itself. */
class TBMessageWindow : public TBWindow, private TBWidgetListener
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBMessageWindow, TBWindow);

	TBMessageWindow(TBWidget *target, TBID id);
	virtual ~TBMessageWindow();

	bool show(const char *title, const char *message, TBMessageWindowSettings *settings = nullptr);

	virtual TBWidget *getEventDestination() override { return m_target.get(); }

	virtual bool onEvent(const TBWidgetEvent &ev) override;
	virtual void onDie() override;
private:
	void addButton(TBID id, bool focused);
	// TBWidgetListener
	virtual void onWidgetDelete(TBWidget *widget) override;
	virtual bool onWidgetDying(TBWidget *widget) override;
	TBWidgetSafePointer m_dimmer;
	TBWidgetSafePointer m_target;
};

} // namespace tb
