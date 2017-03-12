#pragma once

#include "Node.h"

namespace ImGui {

class NormalizeNode: public NNode {
protected:
	const char* getTooltip() const override;

	const char* getInfo() const override;

	float getNoise(int x, int y) override;
public:
	static NormalizeNode* Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge);
};

}
