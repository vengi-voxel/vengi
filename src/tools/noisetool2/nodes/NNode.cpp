/**
 * @file
 */

#include "NNode.h"
#include <glm/common.hpp>

extern ImGui::Node* nodeFactory(int nodeType, const ImVec2& pos);

const char* NodeBase::getInfo() const {
	if (type.empty()) {
		type = std::string(NodeTypeStr[getType()]) + "\n" + info;
	}
	return type.c_str();
}

bool NodeBase::setup(ImGui::NodeGraphEditor& nge, const ImVec2& pos, const char* inputSlots, const char* outputSlots, NodeType nodeTypeID) {
	init(NodeTypeStr[int(nodeTypeID)], pos, inputSlots, outputSlots, int(nodeTypeID));
	this->nge = &nge;
	this->info = NodeTooltipStr[int(nodeTypeID)];
	return this->onInit();
}

const char* NodeBase::getTooltip() const {
	return info.c_str();
}

NNode* NNode::copy() {
	NNode* sourceCopyNode = (NNode*)nodeFactory(typeID, ImVec2(0, 0));
    sourceCopyNode->fields.copyPDataValuesFrom(fields);
    return sourceCopyNode;
}

bool NNode::acceptsLink(Node* inputNode) {
	return dynamic_cast<NNode*>(inputNode) != nullptr;
}

void NNode::onEdited() {
	markDirty();
}

void NNode::markDirty() {
	ImVector<ImGui::Node *> nodes;
	nge->getOutputNodesForNodeAndSlot(this, 0, nodes);
	for (ImGui::Node* node : nodes) {
		NNode* n = dynamic_cast<NNode*>(node);
		if (n == nullptr) {
			node->onEdited();
			continue;
		}
		n->markDirty();
	}
}

float ResultNode::getNoise(int x, int y, int z) {
	float result = 0.0f;
	const int n = getNumInputSlots();
	for (int i = 0; i < n; ++i) {
		NNode* in = dynamic_cast<NNode*>(nge->getInputNodeForNodeAndSlot(this, i));
		if (in == nullptr) {
			continue;
		}
		switch (getType()) {
		case (int)NodeType::Add:
			result += in->getNoise(x, y, z);
			break;
		case (int)NodeType::Subtract:
			if (i == 0) {
				result = in->getNoise(x, y, z);
			} else {
				result -= in->getNoise(x, y, z);
			}
			break;
		case (int)NodeType::Multiply:
			if (i == 0) {
				result = in->getNoise(x, y, z);
			} else {
				result *= in->getNoise(x, y, z);
			}
			break;
		case (int)NodeType::Divide:
			if (i == 0) {
				result = in->getNoise(x, y, z);
			} else {
				result /= in->getNoise(x, y, z);
			}
			break;
		case (int)NodeType::MinNoise:
			if (i == 0) {
				result = in->getNoise(x, y, z);
			} else {
				result = glm::min(result, in->getNoise(x, y, z));
			}
			break;
		case (int)NodeType::MaxNoise:
			if (i == 0) {
				result = in->getNoise(x, y, z);
			} else {
				result = glm::max(result, in->getNoise(x, y, z));
			}
			break;
		}
	}
	return result;
}
