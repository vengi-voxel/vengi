#pragma once

#include "NNode.h"
#include "core/ConcurrentQueue.h"
#include "core/ThreadPool.h"
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
	struct VolumeCommandReturn {
		int voxelCnt = 0;
		voxel::RawVolume* volume = nullptr;
		NNode* noise = nullptr;
		voxel::Mesh* mesh = nullptr;
		inline bool operator< (const VolumeCommandReturn& cmd) const { return noise > cmd.noise; }
	};
	struct VolumeCommand {
		float threshold = 0.0f;
		int volumeWidth = 0;
		int volumeHeight = 0;
		int volumeDepth = 0;
		voxel::Region region;
		NNode* noise = nullptr;
		inline bool operator< (const VolumeCommand& cmd) const { return noise > cmd.noise; }
	};
	core::ConcurrentQueue<VolumeCommand> _commands;
	core::ConcurrentQueue<VolumeCommandReturn> _return;
	std::thread _thread;
	std::atomic_bool _abortThread {false};

	void volumeCallback();
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
	void update();
	static VolumeNode* Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge);
};
