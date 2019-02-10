/**
 * @file
 */

#include "tb_select.h"
#include "tb_menu_window.h"
#include "tb_widgets_listener.h"
#include "tb_language.h"
#include "core/Assert.h"
#include <stdlib.h>

namespace tb {

/** TBSimpleLayoutItemWidget is a item containing a layout with the following:
	-TBSkinImage showing the item image.
	-TBTextField showing the item string.
	-TBSkinImage showing the arrow for items with a submenu.
	It also handles submenu events. */
class TBSimpleLayoutItemWidget : public TBLayout, private TBWidgetListener
{
public:
	TBSimpleLayoutItemWidget(TBID image, TBSelectItemSource *source, const char *str);
	~TBSimpleLayoutItemWidget();
	virtual bool onEvent(const TBWidgetEvent &ev);
private:
	TBSelectItemSource *m_source;
	TBTextField m_textfield;
	TBSkinImage m_image;
	TBSkinImage m_image_arrow;
	TBMenuWindow *m_menu; ///< Points to the submenu window if opened
	virtual void onWidgetDelete(TBWidget *widget);
	void openSubMenu();
	void closeSubMenu();
};

TBSimpleLayoutItemWidget::TBSimpleLayoutItemWidget(TBID image, TBSelectItemSource *source, const char *str)
	: m_source(source)
	, m_menu(nullptr)
{
	setSkinBg(TBIDC("TBSelectItem"));
	setLayoutDistribution(LAYOUT_DISTRIBUTION_AVAILABLE);
	setPaintOverflowFadeout(false);

	if (image)
	{
		m_image.setSkinBg(image);
		m_image.setIgnoreInput(true);
		addChild(&m_image);
	}

	m_textfield.setText(str);
	m_textfield.setTextAlign(TB_TEXT_ALIGN_LEFT);
	m_textfield.setIgnoreInput(true);
	addChild(&m_textfield);

	if (source)
	{
		m_image_arrow.setSkinBg(TBIDC("arrow.right"));
		m_image_arrow.setIgnoreInput(true);
		addChild(&m_image_arrow);
	}
}

TBSimpleLayoutItemWidget::~TBSimpleLayoutItemWidget()
{
	m_image_arrow.removeFromParent();
	m_textfield.removeFromParent();
	m_image.removeFromParent();
	closeSubMenu();
}

bool TBSimpleLayoutItemWidget::onEvent(const TBWidgetEvent &ev)
{
	if (m_source && ev.type == EVENT_TYPE_CLICK && ev.target == this)
	{
		openSubMenu();
		return true;
	}
	return false;
}

void TBSimpleLayoutItemWidget::onWidgetDelete(TBWidget *widget)
{
	core_assert(widget == m_menu);
	closeSubMenu();
}

void TBSimpleLayoutItemWidget::openSubMenu()
{
	if (m_menu)
		return;

	// Open a new menu window for the submenu with this widget as target
	m_menu = new TBMenuWindow(this, TBIDC("submenu"));
	if (m_menu)
	{
		setState(WIDGET_STATE_SELECTED, true);
		m_menu->addListener(this);
		m_menu->show(m_source, TBPopupAlignment(TB_ALIGN_RIGHT), -1);
	}
}

void TBSimpleLayoutItemWidget::closeSubMenu()
{
	if (!m_menu)
		return;

	setState(WIDGET_STATE_SELECTED, false);
	m_menu->removeListener(this);
	if (!m_menu->getIsDying())
		m_menu->close();
	m_menu = nullptr;
}

void TBSelectItemViewer::setSource(TBSelectItemSource *source)
{
	if (m_source == source)
		return;

	if (m_source)
		m_source->m_viewers.remove(this);
	m_source = source;
	if (m_source)
		m_source->m_viewers.addLast(this);

	onSourceChanged();
}

TBSelectItemSource::~TBSelectItemSource()
{
	// If this core_assert trig, you are deleting a model that's still set on some
	// TBSelect widget. That might be dangerous.
	core_assert(!m_viewers.hasLinks());
}

bool TBSelectItemSource::filter(int index, const char *filter)
{
	const char *str = getItemString(index);
	if (str && stristr(str, filter))
		return true;
	return false;
}

TBWidget *TBSelectItemSource::createItemWidget(int index, TBSelectItemViewer *viewer)
{
	const char *string = getItemString(index);
	TBSelectItemSource *sub_source = getItemSubSource(index);
	TBID image = getItemImage(index);
	if (sub_source || image)
	{
		if (TBSimpleLayoutItemWidget *itemwidget = new TBSimpleLayoutItemWidget(image, sub_source, string))
			return itemwidget;
	}
	else if (string && *string == '-')
	{
		if (TBSeparator *separator = new TBSeparator)
		{
			separator->setGravity(WIDGET_GRAVITY_ALL);
			separator->setSkinBg(TBIDC("TBSelectItem.separator"));
			return separator;
		}
	}
	else if (TBTextField *textfield = new TBTextField)
	{
		textfield->setSkinBg("TBSelectItem");
		textfield->setText(string);
		textfield->setTextAlign(TB_TEXT_ALIGN_LEFT);
		return textfield;
	}
	return nullptr;
}

void TBSelectItemSource::invokeItemChanged(int index, TBSelectItemViewer *excludeViewer)
{
	TBLinkListOf<TBSelectItemViewer>::Iterator iter = m_viewers.iterateForward();
	while (TBSelectItemViewer *viewer = iter.getAndStep())
		if (viewer != excludeViewer)
			viewer->onItemChanged(index);
}

void TBSelectItemSource::invokeItemAdded(int index)
{
	TBLinkListOf<TBSelectItemViewer>::Iterator iter = m_viewers.iterateForward();
	while (TBSelectItemViewer *viewer = iter.getAndStep())
		viewer->onItemAdded(index);
}

void TBSelectItemSource::invokeItemRemoved(int index)
{
	TBLinkListOf<TBSelectItemViewer>::Iterator iter = m_viewers.iterateForward();
	while (TBSelectItemViewer *viewer = iter.getAndStep())
		viewer->onItemRemoved(index);
}

void TBSelectItemSource::invokeAllItemsRemoved()
{
	TBLinkListOf<TBSelectItemViewer>::Iterator iter = m_viewers.iterateForward();
	while (TBSelectItemViewer *viewer = iter.getAndStep())
		viewer->onAllItemsRemoved();
}

} // namespace tb
