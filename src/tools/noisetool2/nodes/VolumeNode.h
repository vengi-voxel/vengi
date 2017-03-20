#pragma once

#include "NNode.h"
#include "frontend/RawVolumeRenderer.h"
#include "video/FrameBuffer.h"
#include "video/Texture.h"
#include "video/Camera.h"
#include "core/Var.h"

class VolumeNode : public NodeBase {
private:
	frontend::RawVolumeRenderer _rawVolumeRenderer;
	video::FrameBuffer _frameBuffer;
	video::Camera _camera;
	core::VarPtr _rotationSpeed;
protected:
	int volumeWidth = 32;
	int volumeHeight = 32;
	int volumeDepth = 32;
	int voxelCnt = 0;

	void onEdited() override;
	bool render(float nodeWidth) override;
	bool onInit() override;
	float getThreshold();
	void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut, ImU32& defaultTitleBgColorOut, float& defaultTitleBgColorGradientOut) const override;
public:
	VolumeNode();
	virtual ~VolumeNode();
	static VolumeNode* Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge);
};
