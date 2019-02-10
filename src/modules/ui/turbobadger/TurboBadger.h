/**
 * @file
 */

#pragma once

#include <tb_core.h>
#include <tb_language.h>
#include <tb_node_tree.h>
#include <tb_skin.h>
#include <tb_system.h>
#include <tb_str.h>
#include <tb_select.h>
#include <image/tb_image_widget.h>
#include <tb_select_item.h>
#include <tb_widgets.h>
#include <tb_editfield.h>
#include <tb_window.h>
#include <tb_msg.h>
#include <tb_debug.h>
#include <tb_value.h>
#include <tb_inline_select.h>
#include <tb_bitmap_fragment.h>
#include <tb_system.h>
#include <tb_font_renderer.h>
#include <renderers/tb_renderer_batcher.h>
#include <animation/tb_widget_animation.h>
#include <tb_widgets_reader.h>
#include <tb_widgets_listener.h>
#include <tb_menu_window.h>
#include <tb_node_tree.h>
#include <tb_message_window.h>
#include "ui_renderer_gl.h"

#define UIWIDGET_SUBCLASS(clazz, baseclazz) TBOBJECT_SUBCLASS(clazz, baseclazz)
#define UIWIDGET_FACTORY(classname, sync_type, add_child_z)  \
	class classname##Factory : public tb::TBWidgetFactory \
	{ \
	public: \
		classname##Factory() \
			: tb::TBWidgetFactory(#classname, sync_type) { doRegister(); } \
		virtual ~classname##Factory() {} \
		virtual tb::TBWidget *create(tb::INFLATE_INFO *info) \
		{ \
			classname *widget = new classname(); \
			if (widget) { \
				widget->getContentRoot()->setZInflate(add_child_z); \
			} \
			return widget; \
		} \
	};

namespace ui {
namespace turbobadger {
using UIRect = tb::TBRect;
using UICheckBox = tb::TBCheckBox;
using UIRadioButton = tb::TBRadioButton;
using UITextField = tb::TBTextField;
}
}
