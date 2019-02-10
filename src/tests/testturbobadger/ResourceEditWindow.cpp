#include "ResourceEditWindow.h"
#include "tb_widgets_reader.h"
#include "tb_message_window.h"
#include "tb_system.h"
#include "tb_select.h"
#include "tb_editfield.h"
#include "tb_tempbuffer.h"
#include "tb_scroll_container.h"
#include <stdio.h>

// == ResourceItem ====================================================================================

ResourceItem::ResourceItem(TBWidget *widget, const char *str)
	: TBGenericStringItem(str)
	, m_widget(widget)
{
}

// == ResourceEditWindow ==============================================================================

ResourceEditWindow::ResourceEditWindow()
	: m_widget_list(nullptr)
	, m_scroll_container(nullptr)
	, m_build_container(nullptr)
	, m_source_edit(nullptr)
{
	// Register as global listener to intercept events in the build container
	TBWidgetListener::addGlobalListener(this);

	g_widgets_reader->loadFile(this, "demo01/ui_resources/resource_edit_window.tb.txt");

	m_scroll_container = getWidgetByIDAndType<TBScrollContainer>(TBIDC("scroll_container"));
	m_build_container = m_scroll_container->getContentRoot();
	m_source_edit = getWidgetByIDAndType<TBEditField>(TBIDC("source_edit"));

	m_widget_list = getWidgetByIDAndType<TBSelectList>(TBIDC("widget_list"));
	m_widget_list->setSource(&m_widget_list_source);

	setRect(TBRect(100, 50, 900, 600));
}

ResourceEditWindow::~ResourceEditWindow()
{
	TBWidgetListener::removeGlobalListener(this);

	// avoid assert
	m_widget_list->setSource(nullptr);
}

void ResourceEditWindow::load(const char *resourceFile)
{
	m_resource_filename.set(resourceFile);
	setText(resourceFile);

	TBTempBuffer buffer;
	if (buffer.appendFile(m_resource_filename))
		m_source_edit->setText(buffer.getData(), buffer.getAppendPos());
	else // Error, clear and show message
	{
		m_source_edit->setText("");
		TBStr text;
		text.setFormatted("Could not load file %s", resourceFile);
		if (TBMessageWindow *msg_win = new TBMessageWindow(getParentRoot(), TBIDC("")))
			msg_win->show("Error loading resource", text);
	}

	refreshFromSource();
}

void ResourceEditWindow::refreshFromSource()
{
	// Clear old widgets
	while (TBWidget *child = m_build_container->getFirstChild())
	{
		m_build_container->removeChild(child);
		delete child;
	}

	// Create new widgets from source
	g_widgets_reader->loadData(m_build_container, m_source_edit->getText());

	// Force focus back in case the edited resource has autofocus.
	// FIX: It would be better to prevent the focus change instead!
	m_source_edit->setFocus(WIDGET_FOCUS_REASON_UNKNOWN);
}

void ResourceEditWindow::updateWidgetList(bool immediately)
{
	if (!immediately)
	{
		TBID id = TBIDC("update_widget_list");
		if (!getMessageByID(id))
			postMessage(id, nullptr);
	}
	else
	{
		m_widget_list_source.deleteAllItems();
		addWidgetListItemsRecursive(m_build_container, 0);

		m_widget_list->invalidateList();
	}
}

void ResourceEditWindow::addWidgetListItemsRecursive(TBWidget *widget, int depth)
{
	if (depth > 0) // Ignore the root
	{
		// Add a new ResourceItem for this widget
		TBStr str;
		const char *classname = widget->getClassName();
		if (!*classname)
			classname = "<Unknown widget type>";
		str.setFormatted("% *s%s", depth - 1, "", classname);

		if (ResourceItem *item = new ResourceItem(widget, str))
			m_widget_list_source.addItem(item);
	}

	for (TBWidget *child = widget->getFirstChild(); child; child = child->getNext())
		addWidgetListItemsRecursive(child, depth + 1);
}

ResourceEditWindow::ITEM_INFO ResourceEditWindow::getItemFromWidget(TBWidget *widget)
{
	ITEM_INFO item_info = { nullptr, -1 };
	for (int i = 0; i < m_widget_list_source.getNumItems(); i++)
		if (m_widget_list_source.getItem(i)->getWidget() == widget)
		{
			item_info.index = i;
			item_info.item = m_widget_list_source.getItem(i);
			break;
		}
	return item_info;
}

void ResourceEditWindow::setSelectedWidget(TBWidget *widget)
{
	m_selected_widget.set(widget);
	ITEM_INFO item_info = getItemFromWidget(widget);
	if (item_info.item)
		m_widget_list->setValue(item_info.index);
}

bool ResourceEditWindow::onEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CHANGED && ev.target->getID() == TBIDC("widget_list_search"))
	{
		m_widget_list->setFilter(ev.target->getText());
		return true;
	}
	else if (ev.type == EVENT_TYPE_CHANGED && ev.target == m_widget_list)
	{
		if (m_widget_list->getValue() >= 0 && m_widget_list->getValue() < m_widget_list_source.getNumItems())
			if (ResourceItem *item = m_widget_list_source.getItem(m_widget_list->getValue()))
				setSelectedWidget(item->getWidget());
	}
	else if (ev.type == EVENT_TYPE_CHANGED && ev.target == m_source_edit)
	{
		refreshFromSource();
		return true;
	}
	else if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("test"))
	{
		// Create a window containing the current layout, resize and center it.
		if (TBWindow *win = new TBWindow())
		{
			win->setText("Test window");
			g_widgets_reader->loadData(win->getContentRoot(), m_source_edit->getText());
			TBRect bounds(0, 0, getParent()->getRect().w, getParent()->getRect().h);
			win->setRect(win->getResizeToFitContentRect().centerIn(bounds).moveIn(bounds).clip(bounds));
			getParent()->addChild(win);
		}
		return true;
	}
	else if (ev.target->getID() == TBIDC("constrained"))
	{
		m_scroll_container->setAdaptContentSize(ev.target->getValue() ? true : false);
		return true;
	}
	else if (ev.type == EVENT_TYPE_FILE_DROP)
	{
		return onDropFileEvent(ev);
	}
	return TBWindow::onEvent(ev);
}

void ResourceEditWindow::onPaintChildren(const PaintProps &paintProps)
{
	TBWindow::onPaintChildren(paintProps);

	// Paint the selection of the selected widget
	if (TBWidget *selected_widget = getSelectedWidget())
	{
		TBRect widget_rect(0, 0, selected_widget->getRect().w, selected_widget->getRect().h);
		selected_widget->convertToRoot(widget_rect.x, widget_rect.y);
		convertFromRoot(widget_rect.x, widget_rect.y);
		g_tb_skin->paintRect(widget_rect, TBColor(255, 205, 0), 1);
	}
}

void ResourceEditWindow::onMessageReceived(TBMessage *msg)
{
	if (msg->message == TBIDC("update_widget_list"))
		updateWidgetList(true);
}

bool ResourceEditWindow::onWidgetInvokeEvent(TBWidget *widget, const TBWidgetEvent &ev)
{
	// Intercept all events to widgets in the build container
	if (m_build_container->isAncestorOf(ev.target))
	{
		// Let events through if alt is pressed so we can test some
		// functionality right in the editor (like toggle hidden UI).
		if (ev.modifierkeys & TB_ALT)
			return false;

		// Select widget when clicking
		if (ev.type == EVENT_TYPE_POINTER_DOWN)
			setSelectedWidget(ev.target);

		if (ev.type == EVENT_TYPE_FILE_DROP)
			onDropFileEvent(ev);
		return true;
	}
	return false;
}

void ResourceEditWindow::onWidgetAdded(TBWidget *parent, TBWidget *child)
{
	if (m_build_container && m_build_container->isAncestorOf(child))
		updateWidgetList(false);
}

void ResourceEditWindow::onWidgetRemove(TBWidget *parent, TBWidget *child)
{
	if (m_build_container && m_build_container->isAncestorOf(child))
		updateWidgetList(false);
}

bool ResourceEditWindow::onDropFileEvent(const TBWidgetEvent &ev)
{
	const TBWidgetEventFileDrop *fd_event = TBSafeCast<TBWidgetEventFileDrop>(&ev);
	if (fd_event->files.getNumItems() > 0)
		load(*fd_event->files.get(0));
	return true;
}
