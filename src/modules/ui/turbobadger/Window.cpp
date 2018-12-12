/**
 * @file
 */

#include "Window.h"
#include "UIApp.h"
#include "io/Filesystem.h"
#include "core/GLM.h"
#include "core/Singleton.h"
#include "core/String.h"
#include "core/Var.h"

namespace ui {
namespace turbobadger {

static const std::string EMPTY = "";

Window::Window(UIApp* app) :
		Super(), _app(app) {
	app->addChild(this);
	core::Singleton<io::EventHandler>::getInstance().registerObserver(this);
}

Window::Window(Window* parent) :
		Super(), _app(parent == nullptr ? nullptr : parent->_app) {
	// if this is null, make sure to add the window on your own
	if (parent != nullptr) {
		parent->AddChild(this);
	}
	core::Singleton<io::EventHandler>::getInstance().registerObserver(this);
}

Window::~Window() {
	RemoveFromParent();
	core::Singleton<io::EventHandler>::getInstance().removeObserver(this);
}

tb::TBGenericStringItem* Window::addStringItem(tb::TBGenericStringItemSource& items, const char *text, const char *id, bool translate) {
	tb::TBGenericStringItem* item;
	if (id == nullptr) {
		const std::string& lowerId = core::string::toLower(text);
		item = new tb::TBGenericStringItem(translate ? tr(text) : text, TBIDC(lowerId.c_str()));
		const std::string& iconId = core::App::getInstance()->appname() + "-" + lowerId;
		item->SetSkinImage(TBIDC(iconId.c_str()));
	} else {
		item = new tb::TBGenericStringItem(translate ? tr(text) : text, TBIDC(id));
		char buf[128];
		SDL_snprintf(buf, sizeof(buf), "%s-%s", core::App::getInstance()->appname().c_str(), id);
		item->SetSkinImage(TBIDC(buf));
	}
	items.AddItem(item);
	return item;
}

void Window::OnDie() {
	Super::OnDie();
	core::Singleton<io::EventHandler>::getInstance().removeObserver(this);
}

bool Window::OnEvent(const tb::TBWidgetEvent &ev) {
	return Super::OnEvent(ev);
}

float Window::getFloat(const char *nodeId) {
	return core::string::toFloat(getStr(nodeId));
}

int Window::getSelectedId(const char *nodeId) {
	if (tb::TBSelectDropdown *select = GetWidgetByIDAndType<tb::TBSelectDropdown>(TBIDC(nodeId))) {
		return select->GetValue();
	}
	return -1;
}

int Window::getInt(const char *nodeId) {
	return core::string::toInt(getStr(nodeId));
}

Window* Window::getParent() const {
	return static_cast<Window*>(GetParent());
}

UIApp* Window::getApp() const {
	if (_app) {
		return _app;
	}
	Window* parent = getParent();
	if (parent == nullptr) {
		return nullptr;
	}
	return parent->getApp();
}

void Window::fillWidgets(const Field* fields, int fieldAmount, void* basePtr) {
	for (int i = 0; i < fieldAmount; ++i) {
		const Field& field = fields[i];
		const tb::TBID& name = field.name;
		TBWidget *widget = GetWidgetByID(name);
		if (widget == nullptr) {
			Log::warn("Could not find widget in window %s", GetClassName());
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
		const tb::TBID name(field.name);
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
				Log::warn("Could not find widget with id %s in window %s", field.name, GetClassName());
				continue;
			}
			str = widget->GetText();
		}
		const char *string = str.CStr();
		void* fieldPtr = (uint8_t*)basePtr + field.offset;
		switch (field.type) {
		case T_INT: {
			const int value = atoi(string);
			Log::info("Set %i for %s (%s)", value, field.name, string);
			*(int*)fieldPtr = value;
			break;
		}
		case T_FLOAT: {
			const float value = atof(string);
			Log::info("Set %f for %s (%s)", value, field.name, string);
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
	_filename = filename;
	tb::TBNode node;
	const io::FilesystemPtr& filesystem = core::App::getInstance()->filesystem();
	const io::FilePtr& file = filesystem->open(filename);
	if (!file->exists()) {
		Log::error("%s doesn't exists", filename);
		return false;
	}
	const std::string& data = file->load();
	if (!node.ReadData(data.c_str(), (int)data.size(), tb::TB_NODE_READ_FLAGS_NONE)) {
		return false;
	}
	return loadResource(node);
}

void Window::popup(const std::string& title, const std::string& str, PopupType type, const char *id) {
	tb::TBMessageWindow *win = new tb::TBMessageWindow(this, TBIDC(id));
	tb::TBMessageWindowSettings settings((tb::TB_MSG)std::enum_value(type), tb::TBID(0u));
	settings.dimmer = true;
	win->Show(title.c_str(), str.c_str(), &settings);
}

void Window::setText(const char *nodeId, const std::string& text) {
	tb::TBEditField *widget = GetWidgetByIDAndType<tb::TBEditField>(tb::TBID(nodeId));
	if (widget == nullptr) {
		Log::info("could not find an edit field node with the name %s", nodeId);
		return;
	}
	widget->SetText(text.c_str());
}

void Window::toggleViaVar(const char *checkBoxNodeId, const core::VarPtr& var) {
	toggle(checkBoxNodeId, var->boolVal());
}

void Window::toggle(const char *checkBoxNodeId, bool state) {
	tb::TBCheckBox *widget = GetWidgetByIDAndType<tb::TBCheckBox>(checkBoxNodeId);
	if (widget == nullptr) {
		Log::info("could not find a checkbox node with the name %s", checkBoxNodeId);
		return;
	}
	return widget->SetValue(state ? 1 : 0);
}

bool Window::isToggled(const char *checkBoxNodeId) {
	tb::TBCheckBox *widget = GetWidgetByIDAndType<tb::TBCheckBox>(checkBoxNodeId);
	if (widget == nullptr) {
		Log::info("could not find a checkbox node with the name %s", checkBoxNodeId);
		return false;
	}
	return widget->GetValue() == 1;
}

std::string Window::getStr(const char *nodeId) {
	tb::TBWidget *widget = GetWidgetByID(nodeId);
	if (widget == nullptr) {
		Log::info("could not find a node with the name %s", nodeId);
		return EMPTY;
	}
	const tb::TBStr& amplitude = widget->GetText();
	return std::string(amplitude.CStr());
}

bool Window::loadResourceData(const char *data) {
	tb::TBNode node;
	if (!node.ReadData(data)) {
		return false;
	}
	return loadResource(node);
}

static void printNodeTree(const std::string& filename, tb::TBNode &node) {
	for (tb::TBNode *child = node.GetFirstChild(); child; child = child->GetNext()) {
		Log::trace("File: %s: node found: '%s' = '%s'", filename.c_str(), child->GetName(), child->GetValue().GetString());
		printNodeTree(filename, *child);
	}
}

bool Window::loadResource(tb::TBNode &node) {
	printNodeTree(_filename, node);

	tb::g_widgets_reader->LoadNodeTree(this, &node);

	// Get title from the WindowInfo section (or use "" if not specified)
	SetText(node.GetValueString("WindowInfo>title", ""));

	tb::TBWidget *parent = GetParent();
	if (parent == nullptr) {
		return false;
	}
	const tb::TBRect& r = parent->GetRect();
	const tb::TBRect parentRect(0, 0, r.w, r.h);
	const tb::TBDimensionConverter *dc = tb::g_tb_skin->GetDimensionConverter();
	tb::TBRect windowRect = GetResizeToFitContentRect();

	// Use specified size or adapt to the preferred content size.
	tb::TBNode *tmp = node.GetNode("WindowInfo>size");
	if (tmp && tmp->GetValue().GetArrayLength() == 2) {
		tb::TBValueArray *dimensions = tmp->GetValue().GetArray();
		const char *sizeW = dimensions->GetValue(0)->GetString();
		if (sizeW[strlen(sizeW) - 1] == '%') {
			_percentWidth = atof(sizeW);
			windowRect.w = _app->width() * _percentWidth / 100.0f;
		} else {
			windowRect.w = dc->GetPxFromString(sizeW, windowRect.w);
		}
		const char *sizeH = dimensions->GetValue(1)->GetString();
		if (sizeH[strlen(sizeH) - 1] == '%') {
			_percentHeight = atof(sizeW);
			windowRect.h = _app->height() * _percentHeight / 100.0f;
		} else {
			windowRect.h = dc->GetPxFromString(sizeH, windowRect.h);
		}
	}

	// Use the specified position or center in parent.
	tmp = node.GetNode("WindowInfo>position");
	if (tmp && tmp->GetValue().GetArrayLength() == 2) {
		tb::TBValueArray *position = tmp->GetValue().GetArray();
		const char *posW = position->GetValue(0)->GetString();
		const char *posH = position->GetValue(1)->GetString();
		windowRect.x = dc->GetPxFromString(posW, windowRect.x);
		windowRect.y = dc->GetPxFromString(posH, windowRect.y);
	} else {
		windowRect = windowRect.CenterIn(parentRect);
	}

	if (tb::TBNode *fullscreen = node.GetNode("WindowInfo>fullscreen")) {
		const int fullscreenVal = fullscreen->GetValue().GetInt();
		if (fullscreenVal != 0) {
			windowRect.x = 0;
			windowRect.y = 0;
			if (_app != nullptr) {
				windowRect.w = _app->width();
				windowRect.h = _app->height();
			} else {
				TBWidget *parent = GetParent();
				if (parent != nullptr) {
					windowRect.w = parent->GetPreferredSize().pref_w;
					windowRect.h = parent->GetPreferredSize().pref_h;
				}
			}
		}
	}

	// Make sure the window is inside the parent, and not larger.
	windowRect = windowRect.MoveIn(parentRect).Clip(parentRect);

	SetRect(windowRect);

	// Ensure we have focus - now that we've filled the window with possible focusable
	// widgets. EnsureFocus was automatically called when the window was activated (by
	// adding the window to the root), but then we had nothing to focus.
	// Alternatively, we could add the window after setting it up properly.
	EnsureFocus();
	return true;
}

void Window::onWindowResize() {
	const tb::TBRect parentRect(0, 0, GetParent()->GetRect().w, GetParent()->GetRect().h);
	tb::TBRect windowRect = GetRect();
	windowRect = windowRect.MoveIn(parentRect).Clip(parentRect);
	if (_percentHeight > 0.0f) {
		windowRect.w = _app->width() * _percentWidth;
	}
	if (_percentHeight > 0.0f) {
		windowRect.h = _app->height() * _percentHeight;
	}
	SetRect(windowRect);
}

tb::TBWidget* Window::getWidget(const char *name) {
	return GetWidgetByID(tb::TBID(name));
}

bool Window::setActive(const char *name, bool active) {
	tb::TBWidget* widget = getWidget(name);
	if (widget == nullptr) {
		return false;
	}

	widget->SetState(tb::WIDGET_STATE_DISABLED, !active);
	return true;
}

bool Window::setVisible(const char *name, bool visible) {
	tb::TBWidget* widget = getWidget(name);
	if (widget == nullptr) {
		return false;
	}

	if (visible) {
		widget->SetVisibility(tb::WIDGET_VISIBILITY_GONE);
	} else {
		widget->SetVisibility(tb::WIDGET_VISIBILITY_VISIBLE);
	}
	return true;
}

tb::TBWidget* Window::getWidgetAt(int x, int y, bool includeChildren) {
	return GetWidgetAt(x, y, includeChildren);
}

void Window::requestQuit() {
	core::App::getInstance()->requestQuit();
}

}
}
