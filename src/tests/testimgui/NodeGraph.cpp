#include "NodeGraph.h"
#include "nodes/ColorNode.h"
#include "nodes/CombineNode.h"
#include "nodes/CommentNode.h"
#include "nodes/NoiseNode.h"
#include "nodes/OutputNode.h"

namespace ImGui {

static Node* NodeFactory(int nodeType, const ImVec2& pos) {
	switch ((NodeType)nodeType) {
	case NodeType::Color:
		return ColorNode::Create(pos);
	case NodeType::Combine:
		return CombineNode::Create(pos);
	case NodeType::Comment:
		return CommentNode::Create(pos);
	case NodeType::Noise:
		return NoiseNode::Create(pos);
	case NodeType::Output:
		return OutputNode::Create(pos);
	case NodeType::Max:
		break;
	}
	return nullptr;
}

static void linkCallback(const NodeLink& link, NodeGraphEditor::LinkState state, NodeGraphEditor& editor) {
}

void ShowNodeGraph() {
	static ImGui::NodeGraphEditor nge;
	if (nge.isInited()) {
		nge.registerNodeTypes(NodeTypeStr, int(NodeType::Max), NodeFactory, nullptr, -1);
		nge.registerNodeTypeMaxAllowedInstances(int(NodeType::Output), 1);
		nge.setLinkCallback(linkCallback);

		Node* colorNode = nge.addNode(int(NodeType::Color), ImVec2(40, 50));
		Node* complexNode = nge.addNode(int(NodeType::Noise), ImVec2(40, 150));
		Node* combineNode = nge.addNode(int(NodeType::Combine), ImVec2(275, 80));
		Node* outputNode = nge.addNode(int(NodeType::Output), ImVec2(520, 140));
		nge.addLink(colorNode, 0, combineNode, 0);
		nge.addLink(complexNode, 1, combineNode, 1);
		nge.addLink(complexNode, 0, outputNode, 1);
		nge.addLink(combineNode, 0, outputNode, 0);
		nge.show_style_editor = true;
		nge.show_load_save_buttons = true;
	}
	nge.render();
}

}
