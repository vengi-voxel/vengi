#include "Window.h"
#include "UIApp.h"
#include <glm/glm.hpp>

namespace ui {

Window::Window(UIApp* app) {
	app->addChild(this);
}

Window::Window(Window* parent) {
	parent->AddChild(this);
}

void Window::fillWidgets(const Field* fields, int fieldAmount, void* basePtr) {
	for (int i = 0; i < fieldAmount; ++i) {
		const Field& field = fields[i];
		const tb::TBID& name = field.name;
		const char *widgetName = name.debug_string.CStr();
		TBWidget *widget = GetWidgetByID(name);
		if (widget == nullptr) {
			Log::warn("Could not find widget with id %s in window %s", widgetName, GetID().debug_string.CStr());
			continue;
		}
		void* fieldPtr = (uint8_t*)basePtr + field.offset;
		switch (field.type) {
		case T_INT: {
			tb::TBStr str;
			str.SetFormatted("%i", *(int*)fieldPtr);
			widget->SetText(str);
			break;
		}
		case T_FLOAT: {
			tb::TBStr str;
			str.SetFormatted("%f", *(float*)fieldPtr);
			widget->SetText(str);
			break;
		}
		case T_IVEC2: {
			glm::ivec2* vec = (glm::ivec2*)fieldPtr;
			tb::TBStr str;
			str.SetFormatted("%i:%i", vec->x, vec->y);
			widget->SetText(str);
			break;
		}
		case T_VEC2: {
			glm::vec2* vec = (glm::vec2*)fieldPtr;
			tb::TBStr str;
			str.SetFormatted("%f:%f", vec->x, vec->y);
			widget->SetText(str);
			break;
		}
		}
	}
}

void Window::fillFields(const Field* fields, int fieldAmount, void* basePtr) {
	for (int i = 0; i < fieldAmount; ++i) {
		const Field& field = fields[i];
		const tb::TBID& name = field.name;
		const char *widgetName = name.debug_string.CStr();
		tb::TBStr str;

		tb::TBSelectList *list = GetWidgetByIDAndType<tb::TBSelectList>(name);
		if (list != nullptr) {
			const int value = list->GetValue();
			tb::TBGenericStringItem* item = list->GetDefaultSource()->GetItem(value);
			if (field.type == T_INT) {
				const uint32_t id = item->id;
				str.SetFormatted("%i", id);
			} else {
				str = item->str;
			}
		} else {
			TBWidget *widget = GetWidgetByID(name);
			if (widget == nullptr) {
				Log::warn("Could not find widget with id %s in window %s", widgetName, GetID().debug_string.CStr());
				continue;
			}
			str = widget->GetText();
		}
		const char *string = str.CStr();
		void* fieldPtr = (uint8_t*)basePtr + field.offset;
		switch (field.type) {
		case T_INT: {
			const int value = atoi(string);
			Log::info("Set %i for %s (%s)", value, widgetName, string);
			*(int*)fieldPtr = value;
			break;
		}
		case T_FLOAT: {
			const float value = atof(string);
			Log::info("Set %f for %s (%s)", value, widgetName, string);
			*(float*)fieldPtr = value;
			break;
		}
		case T_IVEC2: {
			char buf[64];
			strncpy(buf, string, sizeof(buf) - 1);
			buf[sizeof(buf) - 1] = '\0';
			char *sep = strchr(buf, ':');
			if (sep == nullptr)
				break;
			*sep++ = '\0';
			glm::ivec2* vec = (glm::ivec2*)fieldPtr;
			vec->x = atoi(string);
			vec->y = atoi(sep);
			break;
		}
		case T_VEC2: {
			char buf[64];
			strncpy(buf, string, sizeof(buf) - 1);
			buf[sizeof(buf) - 1] = '\0';
			char *sep = strchr(buf, ':');
			if (sep == nullptr)
				break;
			*sep++ = '\0';
			glm::vec2* vec = (glm::vec2*)fieldPtr;
			vec->x = atof(string);
			vec->y = atof(sep);
			break;
		}
		}
	}
}

bool Window::loadResourceFile(const char *filename) {
	tb::TBNode node;
	if (!node.ReadFile(filename))
		return false;
	loadResource(node);
	return true;
}

void Window::loadResourceData(const char *data) {
	tb::TBNode node;
	node.ReadData(data);
	loadResource(node);
}

void Window::loadResource(tb::TBNode &node) {
	tb::g_widgets_reader->LoadNodeTree(this, &node);

	// Get title from the WindowInfo section (or use "" if not specified)
	SetText(node.GetValueString("WindowInfo>title", ""));

	const tb::TBRect parentRect(0, 0, GetParent()->GetRect().w, GetParent()->GetRect().h);
	const tb::TBDimensionConverter *dc = tb::g_tb_skin->GetDimensionConverter();
	tb::TBRect windowRect = GetResizeToFitContentRect();

	// Use specified size or adapt to the preferred content size.
	tb::TBNode *tmp = node.GetNode("WindowInfo>size");
	if (tmp && tmp->GetValue().GetArrayLength() == 2) {
		windowRect.w = dc->GetPxFromString(tmp->GetValue().GetArray()->GetValue(0)->GetString(), windowRect.w);
		windowRect.h = dc->GetPxFromString(tmp->GetValue().GetArray()->GetValue(1)->GetString(), windowRect.h);
	}

	// Use the specified position or center in parent.
	tmp = node.GetNode("WindowInfo>position");
	if (tmp && tmp->GetValue().GetArrayLength() == 2) {
		windowRect.x = dc->GetPxFromString(tmp->GetValue().GetArray()->GetValue(0)->GetString(), windowRect.x);
		windowRect.y = dc->GetPxFromString(tmp->GetValue().GetArray()->GetValue(1)->GetString(), windowRect.y);
	} else {
		windowRect = windowRect.CenterIn(parentRect);
	}

	// Make sure the window is inside the parent, and not larger.
	windowRect = windowRect.MoveIn(parentRect).Clip(parentRect);

	SetRect(windowRect);

	// Ensure we have focus - now that we've filled the window with possible focusable
	// widgets. EnsureFocus was automatically called when the window was activated (by
	// adding the window to the root), but then we had nothing to focus.
	// Alternatively, we could add the window after setting it up properly.
	EnsureFocus();
}

bool Window::OnEvent(const tb::TBWidgetEvent &ev) {
	return TBWindow::OnEvent(ev);
}

}
