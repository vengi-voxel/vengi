#include "NodeGraph.h"
#include "nodes/ColorNode.h"
#include "nodes/CombineNode.h"
#include "nodes/CommentNode.h"
#include "nodes/NoiseNode.h"
#include "nodes/NormalizeNode.h"
#include "nodes/RGBANode.h"

static ImGui::NodeGraphEditor nge;

namespace ImGui {

static Node* nodeFactory(int nodeType, const ImVec2& pos) {
	switch ((NodeType)nodeType) {
	case NodeType::Color:
		return ColorNode::Create(pos, nge);
	case NodeType::Combine:
		return CombineNode::Create(pos, nge);
	case NodeType::Comment:
		return CommentNode::Create(pos, nge);
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
		Node* combineNode = nge.addNode(int(NodeType::Combine), ImVec2(310, 150));
		Node* outputNode = nge.addNode(int(NodeType::RGBA), ImVec2(550, 100));
		nge.addLink(noise1Node, 0, combineNode, 0);
		nge.addLink(noise2Node, 0, combineNode, 1);
		nge.addLink(combineNode, 0, outputNode, 0);
		nge.show_style_editor = false;
		nge.show_load_save_buttons = true;
	}
	nge.render();
}

}
