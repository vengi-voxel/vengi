#include "NodeGraph.h"
#include "nodes/CombineNode.h"
#include "nodes/NoiseNode.h"
#include "nodes/NormalizeNode.h"
#include "nodes/RGBANode.h"

static ImGui::NodeGraphEditor nge;

namespace ImGui {

static Node* nodeFactory(int nodeType, const ImVec2& pos) {
	switch ((NodeType)nodeType) {
	case NodeType::Combine:
		return CombineNode::Create(pos, nge);
	case NodeType::Noise:
		return NoiseNode::Create(pos, nge);
	case NodeType::RGBA:
		return RGBANode::Create(pos, nge);
	case NodeType::Normalize:
		return NormalizeNode::Create(pos, nge);
	case NodeType::Max:
		break;
	}
	return nullptr;
}

static void linkCallback(const NodeLink& link, NodeGraphEditor::LinkState state, NodeGraphEditor& editor) {
}

void ShowNodeGraph() {
	if (nge.isInited()) {
		nge.registerNodeTypes(NodeTypeStr, int(NodeType::Max), nodeFactory, nullptr, -1);
		nge.setLinkCallback(linkCallback);

		Node* noise1Node = nge.addNode(int(NodeType::Noise), ImVec2(10, 10));
		Node* noise2Node = nge.addNode(int(NodeType::Noise), ImVec2(10, 210));
		Node* combineNode = nge.addNode(int(NodeType::Combine), ImVec2(310, 50));
		Node* normalizeNode = nge.addNode(int(NodeType::Normalize), ImVec2(310, 200));
		Node* outputNode = nge.addNode(int(NodeType::RGBA), ImVec2(550, 100));
		nge.addLink(noise1Node, 0, combineNode, 0);
		nge.addLink(noise2Node, 0, combineNode, 1);
		nge.addLink(combineNode, 0, normalizeNode, 0);
		nge.addLink(normalizeNode, 0, outputNode, 0);
		nge.addLink(normalizeNode, 0, outputNode, 1);
		nge.addLink(normalizeNode, 0, outputNode, 2);
		nge.addLink(normalizeNode, 0, outputNode, 3);
		nge.show_style_editor = false;
		nge.show_load_save_buttons = true;
	}
	nge.render();
}

}
