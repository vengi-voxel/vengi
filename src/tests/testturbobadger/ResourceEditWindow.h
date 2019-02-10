#ifndef ResourceEditWindow_H
#define ResourceEditWindow_H

#include "tb_widgets.h"
#include "tb_select.h"
#include "tb_widgets_common.h"
#include "tb_widgets_listener.h"
#include "tb_editfield.h"
#include "tb_msg.h"

using namespace tb;

class ResourceItem : public TBGenericStringItem
{
public:
	ResourceItem(TBWidget *widget, const char *str);
	TBWidget *getWidget() { return m_widget; }
private:
	TBWidget *m_widget;
};

class ResourceEditWindow : public TBWindow, public TBMessageHandler, public TBWidgetListener
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(ResourceEditWindow, TBWindow);

	ResourceEditWindow();
	~ResourceEditWindow();

	void updateWidgetList(bool immediately);

	struct ITEM_INFO {
		ResourceItem *item;
		int index;
	};
	ITEM_INFO getItemFromWidget(TBWidget *widget);
	TBWidget *getSelectedWidget() { return m_selected_widget.get(); }
	void setSelectedWidget(TBWidget *widget);

	void load(const char *resource_file);
	void refreshFromSource();

	// == TBWindow ======================================================================
	virtual bool onEvent(const TBWidgetEvent &ev) override;
	virtual void onPaintChildren(const PaintProps &paint_props) override;

	// == TBMessageHandler ==============================================================
	virtual void onMessageReceived(TBMessage *msg) override;

	// == TBWidgetListener ========================================================
	virtual bool onWidgetInvokeEvent(TBWidget *widget, const TBWidgetEvent &ev) override;
	virtual void onWidgetAdded(TBWidget *parent, TBWidget *child) override;
	virtual void onWidgetRemove(TBWidget *parent, TBWidget *child) override;
private:
	TBSelectList *m_widget_list;
	TBSelectItemSourceList<ResourceItem> m_widget_list_source;
	TBScrollContainer *m_scroll_container;
	TBWidget *m_build_container;
	TBEditField *m_source_edit;
	TBStr m_resource_filename;
	TBWidgetSafePointer m_selected_widget;
	void addWidgetListItemsRecursive(TBWidget *widget, int depth);
	bool onDropFileEvent(const TBWidgetEvent &ev);
};

#endif // ResourceEditWindow_H
