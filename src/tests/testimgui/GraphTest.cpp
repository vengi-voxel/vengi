#include "GraphTest.h"

#include <vector>
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

// Creating a node graph editor for ImGui
// Quick demo, not production code! This is more of a demo of how to use ImGui to create custom stuff.
// Better version by @daniel_collin here https://gist.github.com/emoon/b8ff4b4ce4f1b43e79f2
// See https://github.com/ocornut/imgui/issues/306
// v0.02
// Animated gif: https://cloud.githubusercontent.com/assets/8225057/9472357/c0263c04-4b4c-11e5-9fdf-2cd4f33f6582.gif

// NB: You can use math functions/operators on ImVec2 if you #define IMGUI_DEFINE_MATH_OPERATORS and #include "imgui_internal.h"
// Here we only declare simple +/- operators so others don't leak into the demo code.
static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) {
	return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) {
	return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}

static const float NODE_SLOT_RADIUS = 8.0f;
static const float NODE_SLOT_PADDING = 10.0f;
static const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);
static const ImVec2 SLOT_RADIUS(NODE_SLOT_RADIUS, NODE_SLOT_RADIUS);
static const ImVec2 SLOT_SIZE(NODE_SLOT_RADIUS * 2, NODE_SLOT_RADIUS * 2);

struct Node {
	int id;
	char name[32];
	ImVec2 pos;
	ImVec2 internalSize;
	ImVec2 Size;
	float value;
	ImVec4 color;
	int inputsCount;
	int outputsCount;

	Node(int _id, const char* _name, const ImVec2& _pos, float _value, const ImVec4& _color, int _inputsCount, int _outputsCount) {
		id = _id;
		strncpy(name, _name, IM_ARRAYSIZE(name));
		name[31] = 0;
		pos = _pos;
		value = _value;
		color = _color;
		inputsCount = _inputsCount;
		outputsCount = _outputsCount;
	}

	ImVec2 GetInputSlotPos(int slotNumber) const {
		return ImVec2(pos.x + NODE_SLOT_RADIUS, pos.y + internalSize.y + NODE_SLOT_RADIUS + ((float) slotNumber) * (NODE_SLOT_RADIUS + NODE_SLOT_PADDING));
	}

	ImVec2 GetOutputSlotPos(int slotNumber) const {
		return ImVec2(pos.x + internalSize.x - NODE_SLOT_RADIUS, pos.y + internalSize.y + NODE_SLOT_RADIUS + ((float) slotNumber) * (NODE_SLOT_RADIUS + NODE_SLOT_PADDING));
	}
};
struct NodeLink {
	Node* input;
	int inputSlot;
	Node* output;
	int outputSlot;

	NodeLink(Node* _input, int _inputSlot, Node* _output, int _outputSlot) :
			input(_input), inputSlot(_inputSlot), output(_output), outputSlot(_outputSlot) {
	}
};

static std::vector<Node*> nodes;
static std::vector<NodeLink> links;
static bool inited = false;
static ImVec2 scrolling = ImVec2(0.0f, 0.0f);
static bool showGrid = true;
static Node* nodeSelected = nullptr;
static Node* copiedNode = nullptr;
static int nodeId = 0;
static const ImU32 GRID_COLOR = ImColor(200, 200, 200, 40);
static const float GRID_SZ = 64.0f;

enum class NodeConnectionType {
	InputNode,
	OutputNode,
	None
};
static NodeConnectionType connectNodes = NodeConnectionType::None;
static Node* connectNode = nullptr;
static int connectNodeIndex = -1;

static void unlink(Node* node) {
	for (auto i = links.begin(); i != links.end();) {
		NodeLink& link = *i;
		if (link.input == node) {
			i = links.erase(i);
		} else if (link.output == node) {
			i = links.erase(i);
		} else {
			++i;
		}
	}
}

void ShowExampleAppCustomNodeGraph(bool* opened) {
	ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiSetCond_FirstUseEver);
	if (!ImGui::Begin("Example: Custom Node Graph", opened)) {
		ImGui::End();
		return;
	}

	if (!inited) {
		nodes.push_back(new Node(++nodeId, "MainTex", ImVec2(40, 50), 0.5f, ImColor(255, 100, 100), 1, 1));
		nodes.push_back(new Node(++nodeId, "BumpMap", ImVec2(40, 150), 0.42f, ImColor(200, 100, 200), 1, 1));
		nodes.push_back(new Node(++nodeId, "Combine", ImVec2(270, 80), 1.0f, ImColor(0, 200, 100), 2, 2));
		links.push_back(NodeLink(nodes[0], 0, nodes[2], 0));
		//links.push_back(NodeLink(nodes[1], 0, nodes[2], 1));
		inited = true;
	}

	// Draw a list of nodes on the left side
	bool openContextMenu = false;
	Node* nodeHoveredInList = nullptr;
	Node* nodeHoveredInScene = nullptr;
	ImGui::BeginChild("node_list", ImVec2(100, 0));
	ImGui::Text("Nodes");
	ImGui::Separator();
	for (int nodeIdx = 0; nodeIdx < nodes.size(); ++nodeIdx) {
		Node* node = nodes[nodeIdx];
		ImGui::PushID(node->id);
		if (ImGui::Selectable(node->name, node == nodeSelected)) {
			nodeSelected = node;
		}
		if (ImGui::IsItemHovered()) {
			nodeHoveredInList = node;
			openContextMenu |= ImGui::IsMouseClicked(1);
		}
		ImGui::PopID();
	}
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginGroup();

	// Create our child canvas
	ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", scrolling.x, scrolling.y);
	ImGui::SameLine(ImGui::GetWindowWidth() - 100);
	ImGui::Checkbox("Show grid", &showGrid);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImColor(60, 60, 70, 200));
	ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
	ImGui::PushItemWidth(120.0f);

	ImVec2 offset = ImGui::GetCursorScreenPos() - scrolling;
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->ChannelsSplit(2);

	// Display grid
	if (showGrid) {
		ImVec2 winPos = ImGui::GetCursorScreenPos();
		ImVec2 canvasSZ = ImGui::GetWindowSize();
		for (float x = fmodf(offset.x, GRID_SZ); x < canvasSZ.x; x += GRID_SZ) {
			drawList->AddLine(ImVec2(x, 0.0f) + winPos, ImVec2(x, canvasSZ.y) + winPos, GRID_COLOR);
		}
		for (float y = fmodf(offset.y, GRID_SZ); y < canvasSZ.y; y += GRID_SZ) {
			drawList->AddLine(ImVec2(0.0f, y) + winPos, ImVec2(canvasSZ.x, y) + winPos, GRID_COLOR);
		}
	}

	// Display links
	drawList->ChannelsSetCurrent(0); // Background
	for (int linkIdx = 0; linkIdx < links.size(); ++linkIdx) {
		NodeLink* link = &links[linkIdx];
		Node* nodeInp = link->input;
		Node* nodeOut = link->output;
		ImVec2 p1 = offset + nodeInp->GetOutputSlotPos(link->inputSlot);
		ImVec2 p2 = offset + nodeOut->GetInputSlotPos(link->outputSlot);
		drawList->AddBezierCurve(p1, p1 + ImVec2(+50, 0), p2 + ImVec2(-50, 0), p2, ImColor(200, 200, 100), 3.0f);
	}

	const NodeConnectionType oldConnectNodes = connectNodes;
	connectNodes = NodeConnectionType::None;
	// Display nodes
	for (int nodeIdx = 0; nodeIdx < nodes.size(); ++nodeIdx) {
		Node* node = nodes[nodeIdx];
		ImGui::PushID(node->id);
		ImVec2 nodeRectWin = offset + node->pos;

		// Display node contents first
		drawList->ChannelsSetCurrent(1); // Foreground
		bool oldAnyActive = ImGui::IsAnyItemActive();
		ImGui::SetCursorScreenPos(nodeRectWin + NODE_WINDOW_PADDING);
		ImGui::BeginGroup(); // Lock horizontal position
		ImGui::Text("%s", node->name);
		ImGui::SliderFloat("##value", &node->value, 0.0f, 1.0f, "Alpha %.2f");
		ImGui::ColorEdit3("##color", &node->color.x);
		ImGui::EndGroup();

		// Save the size of what we have emitted and whether any of the widgets are being used
		bool nodeWidgetsActive = (!oldAnyActive && ImGui::IsAnyItemActive());
		const int maxSlots = std::max(node->inputsCount, node->outputsCount);
		const ImVec2 additionalSizeSlots(0.0f, maxSlots * (NODE_SLOT_RADIUS + NODE_SLOT_PADDING));
		node->internalSize = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
		node->Size = node->internalSize + additionalSizeSlots;
		ImVec2 nodeRectMax = nodeRectWin + node->Size;

		for (int slotIdx = 0; slotIdx < node->inputsCount; ++slotIdx) {
			ImGui::PushID(-slotIdx);
			const ImVec2 slotPos = offset + node->GetInputSlotPos(slotIdx);
			ImGui::SetCursorScreenPos(ImVec2(slotPos.x - NODE_SLOT_RADIUS, slotPos.y - NODE_SLOT_RADIUS));
			drawList->AddCircleFilled(slotPos, NODE_SLOT_RADIUS, ImColor(150, 150, 150, 150));
			ImGui::SetCursorScreenPos(ImVec2(slotPos.x - NODE_SLOT_RADIUS * 1.5f, slotPos.y - NODE_SLOT_RADIUS * 1.5f));
			ImGui::InvisibleButton("slot", SLOT_SIZE);
			if (connectNodes == NodeConnectionType::None && ImGui::IsItemActive()) {
				const ImVec2& mousePos = ImGui::GetIO().MousePos;
				const ImColor lineColor = ImColor(ImGui::GetStyle().Colors[ImGuiCol_Button]);
				const ImVec2& closestPoint = ImGui::CalcItemRectClosestPoint(mousePos, true, -2.0f);
				drawList->PushClipRectFullScreen();
				drawList->AddLine(closestPoint, mousePos, lineColor, 4.0f);
				drawList->PopClipRect();
				connectNodes = NodeConnectionType::InputNode;
				connectNode = node;
				connectNodeIndex = slotIdx;
			}
			ImGui::PopID();
		}
		for (int slotIdx = 0; slotIdx < node->outputsCount; ++slotIdx) {
			ImGui::PushID(-slotIdx - node->inputsCount);
			const ImVec2 slotPos = offset + node->GetOutputSlotPos(slotIdx);
			ImGui::SetCursorScreenPos(ImVec2(slotPos.x - NODE_SLOT_RADIUS, slotPos.y - NODE_SLOT_RADIUS));
			drawList->AddCircleFilled(slotPos, NODE_SLOT_RADIUS, ImColor(150, 150, 150, 150));
			ImGui::SetCursorScreenPos(ImVec2(slotPos.x - NODE_SLOT_RADIUS * 1.5f, slotPos.y - NODE_SLOT_RADIUS * 1.5f));
			ImGui::InvisibleButton("slotoutput", SLOT_SIZE);
			if (connectNodes == NodeConnectionType::None && ImGui::IsItemActive()) {
				const ImVec2& mousePos = ImGui::GetIO().MousePos;
				const ImColor lineColor = ImColor(ImGui::GetStyle().Colors[ImGuiCol_Button]);
				const ImVec2& closestPoint = ImGui::CalcItemRectClosestPoint(mousePos, true, -2.0f);
				drawList->PushClipRectFullScreen();
				drawList->AddLine(closestPoint, mousePos, lineColor, 4.0f);
				drawList->PopClipRect();
				connectNodes = NodeConnectionType::OutputNode;
				connectNode = node;
				connectNodeIndex = slotIdx;
			}
			ImGui::PopID();
		}

		// Display node box
		drawList->ChannelsSetCurrent(0); // Background
		ImGui::SetCursorScreenPos(nodeRectWin);

		ImU32 nodeBackgroundColor = ImColor(60, 60, 60);
		if (nodeHoveredInList == node || nodeHoveredInScene == node || (nodeHoveredInList == nullptr && nodeSelected == node)) {
			nodeBackgroundColor = ImColor(75, 75, 75);
		}
		drawList->AddRectFilled(nodeRectWin, nodeRectMax, nodeBackgroundColor, 4.0f);
		drawList->AddRect(nodeRectWin, nodeRectMax, ImColor(100, 100, 100), 4.0f);

		ImGui::SetCursorScreenPos(nodeRectWin + SLOT_RADIUS);
		ImGui::InvisibleButton("node", node->Size);
		if (ImGui::IsItemHoveredRect()) {
			nodeHoveredInScene = node;
			openContextMenu |= ImGui::IsMouseClicked(1);
		}
		bool nodeMovingActive = ImGui::IsItemActive();
		if (nodeWidgetsActive || nodeMovingActive) {
			nodeSelected = node;
		}
		if (nodeMovingActive && ImGui::IsMouseDragging(0)) {
			node->pos = node->pos + ImGui::GetIO().MouseDelta;
		}

		ImGui::PopID();
	}
	drawList->ChannelsMerge();

	// Open context menu
	if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseHoveringWindow() && ImGui::IsMouseClicked(1)) {
		nodeSelected = nodeHoveredInList = nodeHoveredInScene = nullptr;
		openContextMenu = true;
	}
	if (openContextMenu) {
		ImGui::OpenPopup("context_menu");
		if (nodeHoveredInList != nullptr) {
			nodeSelected = nodeHoveredInList;
		}
		if (nodeHoveredInScene != nullptr) {
			nodeSelected = nodeHoveredInScene;
		}
	}

	if (oldConnectNodes != NodeConnectionType::None && connectNodes == NodeConnectionType::None) {
		Node* selected = nodeHoveredInScene;
		if (selected != nullptr && connectNode != selected) {
			const ImVec2& mousePos = ImGui::GetIO().MousePos;
			if (oldConnectNodes == NodeConnectionType::OutputNode) {
				for (int slotIdx = 0; slotIdx < selected->inputsCount; slotIdx++) {
					const ImVec2 slotPos = offset + selected->GetInputSlotPos(slotIdx);
					const ImVec2& delta = mousePos - slotPos;
					if (fabs(delta.x) < SLOT_SIZE.x && fabs(delta.y) < SLOT_SIZE.y) {
						links.push_back(NodeLink(connectNode, connectNodeIndex, selected, slotIdx));
						break;
					}
				}
			} else {
				for (int slotIdx = 0; slotIdx < selected->outputsCount; slotIdx++) {
					const ImVec2 slotPos = offset + selected->GetOutputSlotPos(slotIdx);
					const ImVec2& delta = mousePos - slotPos;
					if (fabs(delta.x) < SLOT_SIZE.x && fabs(delta.y) < SLOT_SIZE.y) {
						links.push_back(NodeLink(selected, slotIdx, connectNode, connectNodeIndex));
						break;
					}
				}
			}
		}
	}

	// Draw context menu
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	if (ImGui::BeginPopup("context_menu")) {
		Node* node = nodeSelected;
		ImVec2 scenePos = ImGui::GetMousePosOnOpeningCurrentPopup() - offset;
		if (node) {
			ImGui::Text("Node '%s'", node->name);
			ImGui::Separator();
			if (ImGui::MenuItem("Delete")) {
				unlink(node);
				if (copiedNode == node) {
					copiedNode = nullptr;
				}
				if (nodeHoveredInList == node) {
					nodeHoveredInList = nullptr;
				}
				if (nodeHoveredInScene == node) {
					nodeHoveredInScene = nullptr;
				}
				for (auto i = nodes.begin(); i != nodes.end(); ++i) {
					if (*i == node) {
						nodes.erase(i);
						break;
					}
				}
			}
			if (ImGui::MenuItem("Copy")) {
				copiedNode = node;
			}
			if (ImGui::MenuItem("Unlink")) {
				unlink(node);
			}
		} else {
			if (ImGui::MenuItem("Add")) {
				nodes.push_back(new Node(++nodeId, "New node", scenePos, 0.5f, ImColor(100, 100, 200), 2, 2));
			}
			if (ImGui::MenuItem("Paste", nullptr, false, copiedNode != nullptr)) {
				nodes.push_back(new Node(++nodeId, copiedNode->name, scenePos, 0.5f, ImColor(100, 100, 200), 2, 2));
			}
		}
		ImGui::EndPopup();
	}
	ImGui::PopStyleVar();

	// Scrolling
	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(2, 0.0f)) {
		scrolling = scrolling - ImGui::GetIO().MouseDelta;
	}

	ImGui::PopItemWidth();
	ImGui::EndChild();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
	ImGui::EndGroup();

	ImGui::End();
}
