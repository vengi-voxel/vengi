#include "ListWindow.h"

// == AdvancedItemWidget ======================================================

AdvancedItemWidget::AdvancedItemWidget(AdvancedItem *item, AdvancedItemSource *source,
										TBSelectItemViewer *sourceViewer, int index)
	: m_source(source)
	, m_source_viewer(sourceViewer)
	, m_index(index)
{
	setSkinBg(TBIDC("TBSelectItem"));
	setLayoutDistribution(LAYOUT_DISTRIBUTION_GRAVITY);
	setLayoutDistributionPosition(LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP);
	setPaintOverflowFadeout(false);

	g_widgets_reader->loadFile(getContentRoot(), "demo01/ui_resources/test_list_item.tb.txt");
	TBCheckBox *checkbox = getWidgetByIDAndType<TBCheckBox>(TBIDC("check"));
	TBTextField *name = getWidgetByIDAndType<TBTextField>(TBIDC("name"));
	TBTextField *info = getWidgetByIDAndType<TBTextField>(TBIDC("info"));
	checkbox->setValue(item->getChecked() ? true : false);
	name->setText(item->str);
	info->setText(item->getMale() ? "Male" : "Female");
}

bool AdvancedItemWidget::onEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("check"))
	{
		AdvancedItem *item = m_source->getItem(m_index);
		item->setChecked(ev.target->getValue() ? true : false);

		m_source->invokeItemChanged(m_index, m_source_viewer);
		return true;
	}
	else if (ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("delete"))
	{
		m_source->deleteItem(m_index);
		return true;
	}
	return TBLayout::onEvent(ev);
}

// == AdvancedItemSource ======================================================

bool AdvancedItemSource::filter(int index, const char *filter)
{
	// Override this method so we can return hits for our extra data too.

	if (TBSelectItemSource::filter(index, filter))
		return true;

	AdvancedItem *item = getItem(index);
	return stristr(item->getMale() ? "Male" : "Female", filter) ? true : false;
}

TBWidget *AdvancedItemSource::createItemWidget(int index, TBSelectItemViewer *viewer)
{
	if (TBLayout *layout = new AdvancedItemWidget(getItem(index), this, viewer, index))
		return layout;
	return nullptr;
}

// == ListWindow ==============================================================

ListWindow::ListWindow(TBWidget *root, TBSelectItemSource *source) : DemoWindow(root)
{
	loadResourceFile("demo01/ui_resources/test_select.tb.txt");
	if (TBSelectList *select = getWidgetByIDAndType<TBSelectList>("list"))
	{
		select->setSource(source);
		select->getScrollContainer()->setScrollMode(SCROLL_MODE_Y_AUTO);
	}
}

bool ListWindow::onEvent(const TBWidgetEvent &ev)
{
	if (ev.type == EVENT_TYPE_CHANGED && ev.target->getID() == TBIDC("filter"))
	{
		if (TBSelectList *select = getWidgetByIDAndType<TBSelectList>("list"))
			select->setFilter(ev.target->getText());
		return true;
	}
	return DemoWindow::onEvent(ev);
}

// == AdvancedListWindow ==============================================================

AdvancedListWindow::AdvancedListWindow(TBWidget *root, AdvancedItemSource *source)
	: DemoWindow(root)
	, m_source(source)
{
	loadResourceFile("demo01/ui_resources/test_select_advanced.tb.txt");
	if (TBSelectList *select = getWidgetByIDAndType<TBSelectList>("list"))
	{
		select->setSource(source);
		select->getScrollContainer()->setScrollMode(SCROLL_MODE_X_AUTO_Y_AUTO);
	}
}

bool AdvancedListWindow::onEvent(const TBWidgetEvent &ev)
{
	TBSelectList *select = getWidgetByIDAndType<TBSelectList>("list");
	if (select && ev.type == EVENT_TYPE_CHANGED && ev.target->getID() == TBIDC("filter"))
	{
		select->setFilter(ev.target->getText());
		return true;
	}
	else if (select && ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("add"))
	{
		const core::String& name = getTextByID(TBIDC("add_name"));
		if (!name.empty())
			m_source->addItem(new AdvancedItem(name.c_str(), TBIDC("boy_item"), true));
		return true;
	}
	else if (select && ev.type == EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("delete all"))
	{
		m_source->deleteAllItems();
		return true;
	}
	return DemoWindow::onEvent(ev);
}
