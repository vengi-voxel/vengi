/**
 * @file
 */

#pragma once

#include "TurboBadger.h"
#include <cstdint>
#include "core/String.h"
#include "io/IEventObserver.h"

namespace ui {

class UIApp;

#define FIELD(name, type, structtarget, structmember) name, type, offsetof(structtarget, structmember)
#define INT_FIELD(name, structtarget, structmember) FIELD(name, ui::Window::T_INT, structtarget, structmember)
#define FLOAT_FIELD(name, structtarget, structmember) FIELD(name, ui::Window::T_FLOAT, structtarget, structmember)
#define IVEC2_FIELD(name, structtarget, structmember) FIELD(name, ui::Window::T_IVEC2, structtarget, structmember)
#define VEC2_FIELD(name, structtarget, structmember) FIELD(name, ui::Window::T_VEC2, structtarget, structmember)

class Window: public tb::TBWindow, public io::IEventObserver {
protected:
	UIApp* _app;
	float _percentWidth = 0.0f;
	float _percentHeight = 0.0f;
public:
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
public:
	Window(UIApp* app);
	Window(Window* parent);
	virtual ~Window() {}

	std::string getStr(const char* nodeId);
	float getFloat(const char *nodeId);
	int getInt(const char *nodeId);
	bool isToggled(const char *checkBoxNodeId);

	bool loadResourceFile(const char *filename);
	void loadResourceData(const char *data);
	void loadResource(tb::TBNode &node);

	virtual bool OnEvent(const tb::TBWidgetEvent &ev) override;

	virtual void onWindowResize() override;
};

inline float Window::getFloat(const char *nodeId) {
	return core::string::toFloat(getStr(nodeId));
}

inline int Window::getInt(const char *nodeId) {
	return core::string::toInt(getStr(nodeId));
}

}
