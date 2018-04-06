/**
 * @file
 */

#pragma once

#include "NNode.h"

class NormalizeNode: public NNode {
protected:
	float getNoise(int x, int y, int z) override;
	bool canBeCopied() const override { return false; }
public:
	static NormalizeNode* Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge);
};
