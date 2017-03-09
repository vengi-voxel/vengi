#include "imgui_node_graph_test.h"
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

struct Node {
	int ID;
	char Name[32];
	ImVec2 Pos;
	ImVec2 Size;
	float Value;
	ImVec4 Color;
	int InputsCount, OutputsCount;

	Node(int id, const char* name, const ImVec2& pos, float value, const ImVec4& color, int inputs_count, int outputs_count) {
		ID = id;
		strncpy(Name, name, IM_ARRAYSIZE(Name));
		Name[31] = 0;
		Pos = pos;
		Value = value;
		Color = color;
		InputsCount = inputs_count;
		OutputsCount = outputs_count;
	}

	ImVec2 GetInputSlotPos(int slot_no) const {
		return ImVec2(Pos.x, Pos.y + Size.y * ((float) slot_no + 1) / ((float) InputsCount + 1));
	}
	ImVec2 GetOutputSlotPos(int slot_no) const {
		return ImVec2(Pos.x + Size.x, Pos.y + Size.y * ((float) slot_no + 1) / ((float) OutputsCount + 1));
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

static ImVector<Node*> nodes;
static ImVector<NodeLink> links;
static bool inited = false;
static ImVec2 scrolling = ImVec2(0.0f, 0.0f);
static bool show_grid = true;
static Node* node_selected = nullptr;
static Node* copiedNode = nullptr;
static int nodeId = 0;

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

// Really dumb data structure provided for the example.
// Note that we storing links are INDICES (not ID) to make example code shorter, obviously a bad idea for any general purpose code.
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
		links.push_back(NodeLink(nodes[1], 0, nodes[2], 1));
		inited = true;
	}

	// Draw a list of nodes on the left side
	bool open_context_menu = false;
	Node* node_hovered_in_list = nullptr;
	Node* node_hovered_in_scene = nullptr;
	ImGui::BeginChild("node_list", ImVec2(100, 0));
	ImGui::Text("Nodes");
	ImGui::Separator();
	for (int node_idx = 0; node_idx < nodes.Size; node_idx++) {
		Node* node = nodes[node_idx];
		ImGui::PushID(node->ID);
		if (ImGui::Selectable(node->Name, node == node_selected)) {
			node_selected = node;
		}
		if (ImGui::IsItemHovered()) {
			node_hovered_in_list = node;
			open_context_menu |= ImGui::IsMouseClicked(1);
		}
		ImGui::PopID();
	}
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginGroup();

	const float NODE_SLOT_RADIUS = 4.0f;
	const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);

	// Create our child canvas
	ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", scrolling.x, scrolling.y);
	ImGui::SameLine(ImGui::GetWindowWidth() - 100);
	ImGui::Checkbox("Show grid", &show_grid);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImColor(60, 60, 70, 200));
	ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
	ImGui::PushItemWidth(120.0f);

	ImVec2 offset = ImGui::GetCursorScreenPos() - scrolling;
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	draw_list->ChannelsSplit(2);

	// Display grid
	if (show_grid) {
		ImU32 GRID_COLOR = ImColor(200, 200, 200, 40);
		float GRID_SZ = 64.0f;
		ImVec2 offset = ImGui::GetCursorPos() - scrolling;
		ImVec2 win_pos = ImGui::GetCursorScreenPos() + offset;
		ImVec2 canvas_sz = ImGui::GetWindowSize();
		for (float x = fmodf(offset.x, GRID_SZ); x < canvas_sz.x; x += GRID_SZ) {
			draw_list->AddLine(ImVec2(x, 0.0f) + win_pos, ImVec2(x, canvas_sz.y) + win_pos, GRID_COLOR);
		}
		for (float y = fmodf(offset.y, GRID_SZ); y < canvas_sz.y; y += GRID_SZ) {
			draw_list->AddLine(ImVec2(0.0f, y) + win_pos, ImVec2(canvas_sz.x, y) + win_pos, GRID_COLOR);
		}
	}

	// Display links
	draw_list->ChannelsSetCurrent(0); // Background
	for (int link_idx = 0; link_idx < links.Size; link_idx++) {
		NodeLink* link = &links[link_idx];
		Node* node_inp = link->input;
		Node* node_out = link->output;
		ImVec2 p1 = offset + node_inp->GetOutputSlotPos(link->inputSlot);
		ImVec2 p2 = offset + node_out->GetInputSlotPos(link->outputSlot);
		draw_list->AddBezierCurve(p1, p1 + ImVec2(+50, 0), p2 + ImVec2(-50, 0), p2, ImColor(200, 200, 100), 3.0f);
	}

	// Display nodes
	for (int node_idx = 0; node_idx < nodes.Size; node_idx++) {
		Node* node = nodes[node_idx];
		ImGui::PushID(node->ID);
		ImVec2 node_rect_min = offset + node->Pos;

		// Display node contents first
		draw_list->ChannelsSetCurrent(1); // Foreground
		bool old_any_active = ImGui::IsAnyItemActive();
		ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
		ImGui::BeginGroup(); // Lock horizontal position
		ImGui::Text("%s", node->Name);
		ImGui::SliderFloat("##value", &node->Value, 0.0f, 1.0f, "Alpha %.2f");
		ImGui::ColorEdit3("##color", &node->Color.x);
		ImGui::EndGroup();

		// Save the size of what we have emitted and whether any of the widgets are being used
		bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
		node->Size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
		ImVec2 node_rect_max = node_rect_min + node->Size;

		// Display node box
		draw_list->ChannelsSetCurrent(0); // Background
		ImGui::SetCursorScreenPos(node_rect_min);
		ImGui::InvisibleButton("node", node->Size);
		if (ImGui::IsItemHovered()) {
			node_hovered_in_scene = node;
			open_context_menu |= ImGui::IsMouseClicked(1);
		}
		bool node_moving_active = ImGui::IsItemActive();
		if (node_widgets_active || node_moving_active) {
			node_selected = node;
		}
		if (node_moving_active && ImGui::IsMouseDragging(0)) {
			node->Pos = node->Pos + ImGui::GetIO().MouseDelta;
		}

		ImU32 node_bg_color = ImColor(60, 60, 60);
		if (node_hovered_in_list == node || node_hovered_in_scene == node || (node_hovered_in_list == nullptr && node_selected == node)) {
			node_bg_color = ImColor(75, 75, 75);
		}
		draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f);
		draw_list->AddRect(node_rect_min, node_rect_max, ImColor(100, 100, 100), 4.0f);
		for (int slot_idx = 0; slot_idx < node->InputsCount; slot_idx++) {
			draw_list->AddCircleFilled(offset + node->GetInputSlotPos(slot_idx), NODE_SLOT_RADIUS, ImColor(150, 150, 150, 150));
		}
		for (int slot_idx = 0; slot_idx < node->OutputsCount; slot_idx++) {
			draw_list->AddCircleFilled(offset + node->GetOutputSlotPos(slot_idx), NODE_SLOT_RADIUS, ImColor(150, 150, 150, 150));
		}

		ImGui::PopID();
	}
	draw_list->ChannelsMerge();

	// Open context menu
	if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseHoveringWindow() && ImGui::IsMouseClicked(1)) {
		node_selected = node_hovered_in_list = node_hovered_in_scene = nullptr;
		open_context_menu = true;
	}
	if (open_context_menu) {
		ImGui::OpenPopup("context_menu");
		if (node_hovered_in_list != nullptr) {
			node_selected = node_hovered_in_list;
		}
		if (node_hovered_in_scene != nullptr) {
			node_selected = node_hovered_in_scene;
		}
	}

	// Draw context menu
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	if (ImGui::BeginPopup("context_menu")) {
		Node* node = node_selected;
		ImVec2 scene_pos = ImGui::GetMousePosOnOpeningCurrentPopup() - offset;
		if (node) {
			ImGui::Text("Node '%s'", node->Name);
			ImGui::Separator();
			if (ImGui::MenuItem("Delete")) {
				unlink(node);
				if (copiedNode == node) {
					copiedNode = nullptr;
				}
				if (node_hovered_in_list == node) {
					node_hovered_in_list = nullptr;
				}
				if (node_hovered_in_scene == node) {
					node_hovered_in_scene = nullptr;
				}
				nodes.erase(&node_selected);
			}
			if (ImGui::MenuItem("Copy")) {
				copiedNode = node;
			}
			if (ImGui::MenuItem("Unlink")) {
				unlink(node);
			}
		} else {
			if (ImGui::MenuItem("Add")) {
				nodes.push_back(new Node(++nodeId, "New node", scene_pos, 0.5f, ImColor(100, 100, 200), 2, 2));
			}
			if (ImGui::MenuItem("Paste", nullptr, false, copiedNode != nullptr)) {
				nodes.push_back(new Node(++nodeId, copiedNode->Name, scene_pos, 0.5f, ImColor(100, 100, 200), 2, 2));
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
