#include "GraphNode.h"
#include "image/Image.h"
#include "noise/Noise.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/Color.h"
#include "NNode.h"

GraphNode::GraphNode() {
	_texture = video::createEmptyTexture("graph");
}

GraphNode::~GraphNode() {
	_texture->shutdown();
	delete[] _graphBuffer;
}

void GraphNode::onEdited() {
	NNode* input = dynamic_cast<NNode*>(nge->getInputNodeForNodeAndSlot(this, 0));
	if (input == nullptr) {
		Log::info("No input node set");
		return;
	}

	const size_t graphBufferSize = _graphWidth * _graphHeight * BPP;

	delete[] _graphBuffer;
	_graphBuffer = new uint8_t[graphBufferSize];
	const int graphBufOffset = index(0, int(_graphHeight / 2));
	const uint32_t color = core::Color::getRGBA(core::Color::Gray);
	memset(&_graphBuffer[graphBufOffset], color, _graphWidth * BPP);

	for (int i = 0; i < _graphHeight; ++i) {
		uint8_t* gbuf = &_graphBuffer[index(10, i)];
		*((uint32_t*)gbuf) = color;
	}

	const uint32_t graphColor = core::Color::getRGBA(core::Color::Red);
	const int h = _graphHeight - 1;
	for (int x = 0; x < _graphWidth; ++x) {
		const float n = input->getNoise(x, _offset, 0);
		const float cn = noise::norm(n);
		const int gy = h - (cn * h);
		const int idx = index(x, gy);
		uint8_t* gbuf = &_graphBuffer[idx];
		*((uint32_t*)gbuf) = graphColor;
	}

	_texture->upload(_graphWidth, _graphHeight, _graphBuffer);
}

void GraphNode::getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut, ImU32& defaultTitleBgColorOut, float& defaultTitleBgColorGradientOut) const {
	defaultTitleTextColorOut = IM_COL32(230, 180, 180, 255);
	defaultTitleBgColorOut = IM_COL32(40, 55, 55, 200);
	defaultTitleBgColorGradientOut = 0.025f;
}

bool GraphNode::render(float nodeWidth) {
	const bool retVal = Node::render(nodeWidth);
	if (_texture->isLoaded()) {
		ImGui::Image((ImTextureID) (intptr_t) _texture->handle(), ImVec2(_graphWidth, _graphHeight));
	}
	return retVal;
}

GraphNode* GraphNode::Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge) {
	GraphNode* node = imguiAlloc<GraphNode>();
	if (!node->setup(nge, pos, "noise", nullptr, NodeType::Graph)) {
		return nullptr;
	}
	node->fields.addField(&node->_graphWidth, 1, "Width", "Image width", 0, 100, 4096);
	node->fields.addField(&node->_graphHeight, 1, "Height", "Image height", 0, 100, 4096);
	node->fields.addField(&node->_offset, 1, "Offset", "Y offset", 0, -4096, 4096);
	return node;
}
