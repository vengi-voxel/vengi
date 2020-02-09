/**
 * @file
 */

#pragma once

#include "TurboBadger.h"
#include "core/io/IEventObserver.h"
#include <stdint.h>
#include <memory>

namespace core {
class Var;
typedef std::shared_ptr<Var> VarPtr;
}

namespace ui {
namespace turbobadger {

class UIApp;

#define FIELD(name, type, structtarget, structmember) name, type, offsetof(structtarget, structmember)
#define INT_FIELD(name, structtarget, structmember) FIELD(name, ui::turbobadger::Window::T_INT, structtarget, structmember)
#define FLOAT_FIELD(name, structtarget, structmember) FIELD(name, ui::turbobadger::Window::T_FLOAT, structtarget, structmember)
#define IVEC2_FIELD(name, structtarget, structmember) FIELD(name, ui::turbobadger::Window::T_IVEC2, structtarget, structmember)
#define VEC2_FIELD(name, structtarget, structmember) FIELD(name, ui::turbobadger::Window::T_VEC2, structtarget, structmember)
#define tr(id) ui::turbobadger::Window::getTranslation(id)

class Window: public tb::TBWindow, public io::IEventObserver {
private:
	UIApp* _app;
protected:
	using Super = tb::TBWindow;
	float _percentWidth = 0.0f;
	float _percentHeight = 0.0f;
	core::String _filename;

	tb::TBGenericStringItem* addStringItem(tb::TBGenericStringItemSource& items, const char *text, const char *id = nullptr, bool translate = true);

public:
	static inline const char *getTranslation(const char *input) {
		const char *str = tb::g_tb_lng->getString(tb::TBID(input));
		if (!SDL_strncmp(str, "<TRANSLATE", 10)) {
			return input;
		}
		return str;
	}

	enum FieldType {
		T_INT,
		T_FLOAT,
		T_IVEC2,
		T_VEC2
	};

	struct Field {
		const char *name;
		FieldType type;
		size_t offset;
	};

	void fillFields(const Field* fields, int fieldAmount, void* basePtr);
	void fillWidgets(const Field* fields, int fieldAmount, void* basePtr);

	Window(UIApp* app);
	Window(Window* parent);
	virtual ~Window();

	enum class PopupType {
		Ok = tb::TB_MSG_OK,
		OkCancel = tb::TB_MSG_OK_CANCEL,
		YesNo = tb::TB_MSG_YES_NO
	};
	void popup(const core::String& title, const core::String& str, PopupType type = PopupType::Ok, const char *id = "");

	Window* getParent() const;
	UIApp* getApp() const;
	void requestQuit();

	core::String getStr(const char* nodeId);
	float getFloat(const char *nodeId);
	int getInt(const char *nodeId);
	int getSelectedId(const char *nodeId);
	bool isToggled(const char *checkBoxNodeId);
	void toggleViaVar(const char *checkBoxNodeId, const core::VarPtr& var);
	void toggle(const char *checkBoxNodeId, bool state);

	bool loadResourceFile(const char *filename);
	bool loadResourceData(const char *data);
	bool loadResource(tb::TBNode &node);

	template<class T>
	T* getWidgetByType(const char *name);

	tb::TBWidget* getWidget(const char *name);
	bool setVisible(const char *name, bool visible);
	bool setActive(const char *name, bool active);
	void setStr(const char *nodeId, const core::String& text);

	virtual void onDie() override;
	virtual bool onEvent(const tb::TBWidgetEvent &ev) override;

	virtual void onWindowResize(int windowWidth, int windowHeight) override;
};

template<class T>
inline T* Window::getWidgetByType(const char *name) {
	return getWidgetByIDAndType<T>(tb::TBID(name));
}

}
}
