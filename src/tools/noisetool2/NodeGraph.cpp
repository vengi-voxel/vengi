#include "NodeGraph.h"
#include "nodes/NoiseNode.h"
#include "nodes/NormalizeNode.h"
#include "nodes/ResultNodes.h"
#include "nodes/ConstantNode.h"
#include "nodes/VolumeNode.h"
#include "nodes/GraphNode.h"
#include "nodes/RGBANode.h"

static ImGui::NodeGraphEditor nge;

static ImGui::Node* nodeFactory(int nodeType, const ImVec2& pos) {
	switch ((NodeType)nodeType) {
	case NodeType::Add:
		return AddNode::Create(pos, nge);
	case NodeType::Subtract:
		return SubtractNode::Create(pos, nge);
	case NodeType::Constant:
		return ConstantNode::Create(pos, nge);
	case NodeType::Volume:
		return VolumeNode::Create(pos, nge);
	case NodeType::Multiply:
		return MultiplyNode::Create(pos, nge);
	case NodeType::Divide:
		return DivideNode::Create(pos, nge);
	case NodeType::Noise:
		return NoiseNode::Create(pos, nge);
	case NodeType::RGBA:
		return RGBANode::Create(pos, nge);
	case NodeType::Normalize:
		return NormalizeNode::Create(pos, nge);
	case NodeType::Graph:
		return GraphNode::Create(pos, nge);
	case NodeType::MinNoise:
		return MinNoiseNode::Create(pos, nge);
	case NodeType::MaxNoise:
		return MaxNoiseNode::Create(pos, nge);
	case NodeType::Max:
		break;
	}
	return nullptr;
}

static void linkCallback(const ImGui::NodeLink& link, ImGui::NodeGraphEditor::LinkState state, ImGui::NodeGraphEditor& editor) {
}

void showNodeGraph() {
	if (nge.isInited()) {
		nge.registerNodeTypes(NodeTypeStr, int(NodeType::Max), nodeFactory, nullptr, -1);
		nge.setLinkCallback(linkCallback);

		ImGui::Node* noise1Node = nge.addNode(int(NodeType::Noise), ImVec2(10, 10));
		ImGui::Node* noise2Node = nge.addNode(int(NodeType::Noise), ImVec2(10, 210));
		ImGui::Node* combineNode = nge.addNode(int(NodeType::Add), ImVec2(310, 50));
		ImGui::Node* normalizeNode = nge.addNode(int(NodeType::Normalize), ImVec2(310, 200));
		ImGui::Node* outputNode = nge.addNode(int(NodeType::RGBA), ImVec2(550, 100));
		nge.addLink(noise1Node, 0, combineNode, 0);
		nge.addLink(noise2Node, 0, combineNode, 1);
		nge.addLink(combineNode, 0, normalizeNode, 0);
		nge.addLink(normalizeNode, 0, outputNode, 0);
		nge.addLink(normalizeNode, 0, outputNode, 1);
		nge.addLink(normalizeNode, 0, outputNode, 2);
		nge.addLink(normalizeNode, 0, outputNode, 3);

		ImGui::Node* noise3Node = nge.addNode(int(NodeType::Noise), ImVec2(10, 410));
		ImGui::Node* volumeNode = nge.addNode(int(NodeType::Volume), ImVec2(350, 380));
		nge.addLink(noise3Node, 0, volumeNode, 0);

		ImGui::Node* noiseGraphNode = nge.addNode(int(NodeType::Noise), ImVec2(700, 410));
		ImGui::Node* graphNode = nge.addNode(int(NodeType::Graph), ImVec2(1050, 380));
		nge.addLink(noiseGraphNode, 0, graphNode, 0);

		nge.show_style_editor = false;
		nge.show_load_save_buttons = true;
		nge.show_connection_names = false;
		nge.show_left_pane = true;
	}
	nge.render();
}

void shutdownNodeGraph() {
	nge.clear();
}
