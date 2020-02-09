/**
 * @file
 */

#include "Window.h"
#include "UIApp.h"
#include "core/io/Filesystem.h"
#include "core/GLM.h"
#include "core/Singleton.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include <SDL_stdinc.h>

namespace ui {
namespace turbobadger {

static const core::String EMPTY = "";

Window::Window(UIApp* app) :
		Super(), _app(app) {
	app->addChild(this);
	core::Singleton<io::EventHandler>::getInstance().registerObserver(this);
}

Window::Window(Window* parent) :
		Super(), _app(parent == nullptr ? nullptr : parent->_app) {
	// if this is null, make sure to add the window on your own
	if (parent != nullptr) {
		parent->addChild(this);
	}
	core::Singleton<io::EventHandler>::getInstance().registerObserver(this);
}

Window::~Window() {
	removeFromParent();
	core::Singleton<io::EventHandler>::getInstance().removeObserver(this);
}

tb::TBGenericStringItem* Window::addStringItem(tb::TBGenericStringItemSource& items, const char *text, const char *id, bool translate) {
	tb::TBGenericStringItem* item;
	if (id == nullptr) {
		const core::String& lowerId = core::String::lower(text);
		item = new tb::TBGenericStringItem(translate ? tr(text) : text, TBIDC(lowerId.c_str()));
		const core::String& iconId = core::App::getInstance()->appname() + "-" + lowerId;
		item->setSkinImage(TBIDC(iconId.c_str()));
	} else {
		item = new tb::TBGenericStringItem(translate ? tr(text) : text, TBIDC(id));
		char buf[128];
		SDL_snprintf(buf, sizeof(buf), "%s-%s", core::App::getInstance()->appname().c_str(), id);
		item->setSkinImage(TBIDC(buf));
	}
	items.addItem(item);
	return item;
}

void Window::onDie() {
	Super::onDie();
	core::Singleton<io::EventHandler>::getInstance().removeObserver(this);
}

bool Window::onEvent(const tb::TBWidgetEvent &ev) {
	return Super::onEvent(ev);
}

float Window::getFloat(const char *nodeId) {
	return core::string::toFloat(getStr(nodeId));
}

int Window::getSelectedId(const char *nodeId) {
	if (tb::TBSelectDropdown *select = getWidgetByIDAndType<tb::TBSelectDropdown>(TBIDC(nodeId))) {
		return select->getValue();
	}
	return -1;
}

int Window::getInt(const char *nodeId) {
	return core::string::toInt(getStr(nodeId));
}

Window* Window::getParent() const {
	return static_cast<Window*>(Super::getParent());
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
		TBWidget *widget = getWidgetByID(name);
		if (widget == nullptr) {
			Log::warn("Could not find widget in window %s", getClassName());
			continue;
		}
		void* fieldPtr = (uint8_t*)basePtr + field.offset;
		switch (field.type) {
		case T_INT: {
			tb::TBStr str;
			str.setFormatted("%i", *(int*)fieldPtr);
			widget->setText(str);
			break;
		}
		case T_FLOAT: {
			tb::TBStr str;
			str.setFormatted("%f", *(float*)fieldPtr);
			widget->setText(str);
			break;
		}
		case T_IVEC2: {
			glm::ivec2* vec = (glm::ivec2*)fieldPtr;
			tb::TBStr str;
			str.setFormatted("%i:%i", vec->x, vec->y);
			widget->setText(str);
			break;
		}
		case T_VEC2: {
			glm::vec2* vec = (glm::vec2*)fieldPtr;
			tb::TBStr str;
			str.setFormatted("%f:%f", vec->x, vec->y);
			widget->setText(str);
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

		tb::TBSelectList *list = getWidgetByIDAndType<tb::TBSelectList>(name);
		if (list != nullptr) {
			const int value = list->getValue();
			tb::TBGenericStringItem* item = list->getDefaultSource()->getItem(value);
			if (field.type == T_INT) {
				const uint32_t id = item->id;
				str.setFormatted("%i", id);
			} else {
				str = item->str;
			}
		} else {
			TBWidget *widget = getWidgetByID(name);
			if (widget == nullptr) {
				Log::warn("Could not find widget with id %s in window %s", field.name, getClassName());
				continue;
			}
			str = widget->getText();
		}
		const char *string = str.c_str();
		void* fieldPtr = (uint8_t*)basePtr + field.offset;
		switch (field.type) {
		case T_INT: {
			const int value = core::string::toInt(string);
			Log::info("Set %i for %s (%s)", value, field.name, string);
			*(int*)fieldPtr = value;
			break;
		}
		case T_FLOAT: {
			const float value = core::string::toFloat(string);
			Log::info("Set %f for %s (%s)", value, field.name, string);
			*(float*)fieldPtr = value;
			break;
		}
		case T_IVEC2: {
			char buf[64];
			SDL_strlcpy(buf, string, sizeof(buf) - 1);
			buf[sizeof(buf) - 1] = '\0';
			char *sep = SDL_strchr(buf, ':');
			if (sep == nullptr)
				break;
			*sep++ = '\0';
			glm::ivec2* vec = (glm::ivec2*)fieldPtr;
			vec->x = core::string::toInt(string);
			vec->y = core::string::toInt(sep);
			break;
		}
		case T_VEC2: {
			char buf[64];
			SDL_strlcpy(buf, string, sizeof(buf) - 1);
			buf[sizeof(buf) - 1] = '\0';
			char *sep = SDL_strchr(buf, ':');
			if (sep == nullptr)
				break;
			*sep++ = '\0';
			glm::vec2* vec = (glm::vec2*)fieldPtr;
			vec->x = core::string::toFloat(string);
			vec->y = core::string::toFloat(sep);
			break;
		}
		}
	}
}

bool Window::loadResourceFile(const char *filename) {
	_filename = filename;
	tb::TBNode node;
	const io::FilesystemPtr& filesystem = io::filesystem();
	const io::FilePtr& file = filesystem->open(filename);
	if (!file->exists()) {
		Log::error("%s doesn't exists", filename);
		return false;
	}
	const core::String& data = file->load();
	if (!node.readData(data.c_str(), (int)data.size(), tb::TB_NODE_READ_FLAGS_NONE)) {
		return false;
	}
	return loadResource(node);
}

void Window::popup(const core::String& title, const core::String& str, PopupType type, const char *id) {
	tb::TBMessageWindow *win = new tb::TBMessageWindow(this, TBIDC(id));
	tb::TBMessageWindowSettings settings((tb::TB_MSG)core::enumVal(type), tb::TBID(0u));
	settings.dimmer = true;
	win->show(title.c_str(), str.c_str(), &settings);
}

void Window::setStr(const char *nodeId, const core::String& text) {
	tb::TBEditField *widget = getWidgetByType<tb::TBEditField>(nodeId);
	if (widget == nullptr) {
		Log::info("could not find an edit field node with the name %s", nodeId);
		return;
	}
	widget->setText(text.c_str());
}

void Window::toggleViaVar(const char *checkBoxNodeId, const core::VarPtr& var) {
	toggle(checkBoxNodeId, var->boolVal());
}

void Window::toggle(const char *checkBoxNodeId, bool state) {
	tb::TBCheckBox *widget = getWidgetByIDAndType<tb::TBCheckBox>(checkBoxNodeId);
	if (widget == nullptr) {
		Log::info("could not find a checkbox node with the name %s", checkBoxNodeId);
		return;
	}
	return widget->setValue(state ? 1 : 0);
}

bool Window::isToggled(const char *checkBoxNodeId) {
	tb::TBCheckBox *widget = getWidgetByIDAndType<tb::TBCheckBox>(checkBoxNodeId);
	if (widget == nullptr) {
		Log::info("could not find a checkbox node with the name %s", checkBoxNodeId);
		return false;
	}
	return widget->getValue() == 1;
}

core::String Window::getStr(const char *nodeId) {
	tb::TBWidget *widget = getWidgetByID(nodeId);
	if (widget == nullptr) {
		Log::info("could not find a node with the name %s", nodeId);
		return EMPTY;
	}
	const tb::TBStr& amplitude = widget->getText();
	return core::String(amplitude.c_str());
}

bool Window::loadResourceData(const char *data) {
	tb::TBNode node;
	if (!node.readData(data)) {
		return false;
	}
	return loadResource(node);
}

static void printNodeTree(const core::String& filename, tb::TBNode &node) {
	for (tb::TBNode *child = node.getFirstChild(); child; child = child->getNext()) {
		Log::trace("File: %s: node found: '%s' = '%s'", filename.c_str(), child->getName(), child->getValue().getString());
		printNodeTree(filename, *child);
	}
}

bool Window::loadResource(tb::TBNode &node) {
	printNodeTree(_filename, node);

	tb::g_widgets_reader->loadNodeTree(this, &node);

	// Get title from the WindowInfo section (or use "" if not specified)
	setText(node.getValueString("WindowInfo>title", ""));

	tb::TBWidget *parent = getParent();
	if (parent == nullptr) {
		return false;
	}
	const tb::TBRect& r = parent->getRect();
	const tb::TBRect parentRect(0, 0, r.w, r.h);
	const tb::TBDimensionConverter *dc = tb::g_tb_skin->getDimensionConverter();
	tb::TBRect windowRect = getResizeToFitContentRect();

	// Use specified size or adapt to the preferred content size.
	tb::TBNode *tmp = node.getNode("WindowInfo>size");
	if (tmp && tmp->getValue().getArrayLength() == 2) {
		tb::TBValueArray *dimensions = tmp->getValue().getArray();
		const char *sizeW = dimensions->getValue(0)->getString();
		if (sizeW[SDL_strlen(sizeW) - 1] == '%') {
			_percentWidth = atof(sizeW);
			windowRect.w = _app->frameBufferWidth() * _percentWidth / 100.0f;
		} else {
			windowRect.w = dc->getPxFromString(sizeW, windowRect.w);
		}
		const char *sizeH = dimensions->getValue(1)->getString();
		if (sizeH[SDL_strlen(sizeH) - 1] == '%') {
			_percentHeight = atof(sizeW);
			windowRect.h = _app->frameBufferHeight() * _percentHeight / 100.0f;
		} else {
			windowRect.h = dc->getPxFromString(sizeH, windowRect.h);
		}
	}

	// Use the specified position or center in parent.
	tmp = node.getNode("WindowInfo>position");
	if (tmp && tmp->getValue().getArrayLength() == 2) {
		tb::TBValueArray *position = tmp->getValue().getArray();
		const char *posW = position->getValue(0)->getString();
		const char *posH = position->getValue(1)->getString();
		windowRect.x = dc->getPxFromString(posW, windowRect.x);
		windowRect.y = dc->getPxFromString(posH, windowRect.y);
	} else {
		windowRect = windowRect.centerIn(parentRect);
	}

	if (tb::TBNode *fullscreen = node.getNode("WindowInfo>fullscreen")) {
		const int fullscreenVal = fullscreen->getValue().getInt();
		if (fullscreenVal != 0) {
			windowRect.x = 0;
			windowRect.y = 0;
			if (_app != nullptr) {
				windowRect.w = _app->frameBufferWidth();
				windowRect.h = _app->frameBufferHeight();
			} else {
				TBWidget *parent = getParent();
				if (parent != nullptr) {
					windowRect.w = parent->getPreferredSize().pref_w;
					windowRect.h = parent->getPreferredSize().pref_h;
				}
			}
		}
	}

	// Make sure the window is inside the parent, and not larger.
	windowRect = windowRect.moveIn(parentRect).clip(parentRect);

	setRect(windowRect);

	// Ensure we have focus - now that we've filled the window with possible focusable
	// widgets. EnsureFocus was automatically called when the window was activated (by
	// adding the window to the root), but then we had nothing to focus.
	// Alternatively, we could add the window after setting it up properly.
	ensureFocus();
	return true;
}

void Window::onWindowResize(int, int) {
	const tb::TBRect parentRect(0, 0, getParent()->getRect().w, getParent()->getRect().h);
	tb::TBRect windowRect = getRect();
	windowRect = windowRect.moveIn(parentRect).clip(parentRect);
	if (_percentHeight > 0.0f) {
		windowRect.w = _app->frameBufferWidth() * _percentWidth;
	}
	if (_percentHeight > 0.0f) {
		windowRect.h = _app->frameBufferHeight() * _percentHeight;
	}
	setRect(windowRect);
}

tb::TBWidget* Window::getWidget(const char *name) {
	return getWidgetByID(tb::TBID(name));
}

bool Window::setActive(const char *name, bool active) {
	tb::TBWidget* widget = getWidget(name);
	if (widget == nullptr) {
		return false;
	}

	widget->setState(tb::WIDGET_STATE_DISABLED, !active);
	return true;
}

bool Window::setVisible(const char *name, bool visible) {
	tb::TBWidget* widget = getWidget(name);
	if (widget == nullptr) {
		return false;
	}

	if (visible) {
		widget->setVisibility(tb::WIDGET_VISIBILITY_GONE);
	} else {
		widget->setVisibility(tb::WIDGET_VISIBILITY_VISIBLE);
	}
	return true;
}

void Window::requestQuit() {
	core::App::getInstance()->requestQuit();
}

}
}
