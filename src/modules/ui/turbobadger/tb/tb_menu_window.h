/**
 * @file
 */

#pragma once

#include "tb_select.h"
#include "tb_popup_window.h"

namespace tb {

/** TBMenuWindow is a popup window that shows a list of items (TBSelectList).

	When selected it will invoke a click with the id given to the menu,
	and the id of the clicked item as ref_id, and then close itself.

	It may open sub items as new windows at the same time as this window is open.*/

class TBMenuWindow : public TBPopupWindow
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBMenuWindow, TBPopupWindow);

	TBMenuWindow(TBWidget *target, TBID id);
	~TBMenuWindow();

	bool show(TBSelectItemSource *source, const TBPopupAlignment &alignment, int initial_value = -1);

	TBSelectList *getList() { return &m_select_list; }

	virtual bool onEvent(const TBWidgetEvent &ev) override;
	virtual void onDie() override;
private:
	TBSelectList m_select_list;
};

} // namespace tb
