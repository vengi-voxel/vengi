#pragma once

#include "TurboBadger.h"
#include <cstdint>

namespace ui {

class UIApp;

#define FIELD(name, type, structtarget, structmember) tb::TBID(name), type, offsetof(structtarget, structmember)
#define INT_FIELD(name, structtarget, structmember) FIELD(name, T_INT, structtarget, structmember)
#define FLOAT_FIELD(name, structtarget, structmember) FIELD(name, T_FLOAT, structtarget, structmember)

class Window: public tb::TBWindow {
protected:
	enum FieldType {
		T_INT,
		T_FLOAT
	};

	struct Field {
		tb::TBID name;
		FieldType type;
		size_t offset;
	};

	void fillFields(TBWindow* window, const Field* fields, int fieldAmount, void* basePtr);

public:
	Window(UIApp* app);
	Window(Window* parent);
	virtual ~Window() {}

	bool loadResourceFile(const char *filename);
	void loadResourceData(const char *data);
	void loadResource(tb::TBNode &node);

	virtual bool OnEvent(const tb::TBWidgetEvent &ev) override;
};

}
