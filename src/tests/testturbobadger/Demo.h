/**
 * @file
 */

#ifndef DEMO_H
#define DEMO_H

#include "tb_widgets.h"
#include "tb_widgets_common.h"
#include "tb_widgets_reader.h"
#include "tb_widgets_listener.h"
#include "tb_message_window.h"
#include "tb_msg.h"
#include "tb_scroller.h"

using namespace tb;

class DemoWindow : public TBWindow
{
public:
	DemoWindow(TBWidget *root);
	bool loadResourceFile(const char *filename);
	void loadResourceData(const char *data);
	void loadResource(TBNode &node);

	virtual bool onEvent(const TBWidgetEvent &ev);
};

class EditWindow : public DemoWindow
{
public:
	EditWindow(TBWidget *root);
	virtual void onProcessStates();
	virtual bool onEvent(const TBWidgetEvent &ev);
};

class MainWindow : public DemoWindow, public TBMessageHandler
{
public:
	MainWindow(TBWidget *root);
	virtual bool onEvent(const TBWidgetEvent &ev);

	// Implement TBMessageHandler
	virtual void onMessageReceived(TBMessage *msg);
};

class ImageWindow : public DemoWindow
{
public:
	ImageWindow(TBWidget *root);
	virtual bool onEvent(const TBWidgetEvent &ev);
};

class PageWindow : public DemoWindow, public TBScrollerSnapListener
{
public:
	PageWindow(TBWidget *root);
	virtual bool onEvent(const TBWidgetEvent &ev);
	virtual void onScrollSnap(TBWidget *target_widget, int &target_x, int &target_y);
};

class AnimationsWindow : public DemoWindow
{
public:
	AnimationsWindow(TBWidget *root);
	void animate();
	virtual bool onEvent(const TBWidgetEvent &ev);
};

class LayoutWindow : public DemoWindow
{
public:
	LayoutWindow(TBWidget *root, const char *filename);
	virtual bool onEvent(const TBWidgetEvent &ev);
};

class TabContainerWindow : public DemoWindow
{
public:
	TabContainerWindow(TBWidget *root);
	virtual bool onEvent(const TBWidgetEvent &ev);
};

class ConnectionWindow : public DemoWindow
{
public:
	ConnectionWindow(TBWidget *root);
	virtual bool onEvent(const TBWidgetEvent &ev);
};

class ScrollContainerWindow : public DemoWindow, public TBMessageHandler
{
public:
	ScrollContainerWindow(TBWidget *root);
	virtual bool onEvent(const TBWidgetEvent &ev);

	// Implement TBMessageHandler
	virtual void onMessageReceived(TBMessage *msg);
};

#endif
