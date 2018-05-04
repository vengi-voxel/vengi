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
	bool LoadResourceFile(const char *filename);
	void LoadResourceData(const char *data);
	void LoadResource(TBNode &node);

	virtual bool OnEvent(const TBWidgetEvent &ev);
};

class EditWindow : public DemoWindow
{
public:
	EditWindow(TBWidget *root);
	virtual void OnProcessStates();
	virtual bool OnEvent(const TBWidgetEvent &ev);
};

class MainWindow : public DemoWindow, public TBMessageHandler
{
public:
	MainWindow(TBWidget *root);
	virtual bool OnEvent(const TBWidgetEvent &ev);

	// Implement TBMessageHandler
	virtual void OnMessageReceived(TBMessage *msg);
};

class ImageWindow : public DemoWindow
{
public:
	ImageWindow(TBWidget *root);
	virtual bool OnEvent(const TBWidgetEvent &ev);
};

class PageWindow : public DemoWindow, public TBScrollerSnapListener
{
public:
	PageWindow(TBWidget *root);
	virtual bool OnEvent(const TBWidgetEvent &ev);
	virtual void OnScrollSnap(TBWidget *target_widget, int &target_x, int &target_y);
};

class AnimationsWindow : public DemoWindow
{
public:
	AnimationsWindow(TBWidget *root);
	void Animate();
	virtual bool OnEvent(const TBWidgetEvent &ev);
};

class LayoutWindow : public DemoWindow
{
public:
	LayoutWindow(TBWidget *root, const char *filename);
	virtual bool OnEvent(const TBWidgetEvent &ev);
};

class TabContainerWindow : public DemoWindow
{
public:
	TabContainerWindow(TBWidget *root);
	virtual bool OnEvent(const TBWidgetEvent &ev);
};

class ConnectionWindow : public DemoWindow
{
public:
	ConnectionWindow(TBWidget *root);
	virtual bool OnEvent(const TBWidgetEvent &ev);
};

class ScrollContainerWindow : public DemoWindow, public TBMessageHandler
{
public:
	ScrollContainerWindow(TBWidget *root);
	virtual bool OnEvent(const TBWidgetEvent &ev);

	// Implement TBMessageHandler
	virtual void OnMessageReceived(TBMessage *msg);
};

#endif // DEMO_H
