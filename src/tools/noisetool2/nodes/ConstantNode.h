/**
 * @file
 */

#pragma once

#include "NNode.h"

class ConstantNode: public NNode {
public:
	float constant = 0.0f;
	float getNoise(int x, int y, int z) override;
public:
	static ConstantNode* Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge);
};
