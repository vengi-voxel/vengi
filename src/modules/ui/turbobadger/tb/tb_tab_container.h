/**
 * @file
 */

#pragma once

#include "tb_widgets_common.h"

namespace tb {

/** TBTabLayout is a TBLayout used in TBTabContainer to apply
	some default properties on any TBButton added to it. */
class TBTabLayout : public TBLayout
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBTabLayout, TBLayout);

	virtual void onChildAdded(TBWidget *child) override;
	virtual PreferredSize onCalculatePreferredContentSize(const SizeConstraints &constraints) override;
};

/** TBTabContainer - A container with tabs for multiple pages. */
class TBTabContainer : public TBWidget
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBTabContainer, TBWidget);

	TBTabContainer();
	~TBTabContainer();

	/** Set along which axis the content should layouted.
		Use SetAlignment instead for more choice! Also, calling
		setAxis directly does not update the current alignment. */
	virtual void setAxis(AXIS axis) override;
	virtual AXIS getAxis() const override { return m_root_layout.getAxis(); }

	/** Set alignment of the tabs. */
	void setAlignment(TB_ALIGN align);
	TB_ALIGN getAlignment() const { return m_align; }

	/** Set which page should be selected and visible. */
	virtual void setValue(int value) override;
	virtual int getValue() const override { return m_current_page; }

	/** Set which page should be selected and visible. */
	void setCurrentPage(int index) { setValue(index); }
	int getCurrentPage() { return getValue(); }
	int getNumPages();

	/** Return the widget that is the current page, or nullptr if none is active. */
	TBWidget *getCurrentPageWidget() const;

	virtual void onInflate(const INFLATE_INFO &info) override;
	virtual bool onEvent(const TBWidgetEvent &ev) override;
	virtual void onProcess() override;

	virtual TBWidget *getContentRoot() override { return &m_content_root; }
	TBLayout *getTabLayout() { return &m_tab_layout; }
protected:
	TBLayout m_root_layout;
	TBTabLayout m_tab_layout;
	TBWidget m_content_root;
	bool m_need_page_update;
	int m_current_page;
	TB_ALIGN m_align;
};

}
