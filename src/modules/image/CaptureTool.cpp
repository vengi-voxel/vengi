/**
 * @file
 */

#include "CaptureTool.h"
#include "app/App.h"
#include "app/Async.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/Var.h"
#include "external/jo_mpeg.h"
#include "io/Filesystem.h"

namespace image {

bool CaptureTool::isRecording() const {
	if (_videoWriteStream == nullptr) {
		return false;
	}
	if (_stop) {
		return false;
	}
	return true;
}

void CaptureTool::enqueueFrame(const image::ImagePtr &image) {
	if (!image || !image->isLoaded()) {
		return;
	}
	_frameQueue.push(image);
}

uint32_t CaptureTool::pendingFrames() const {
	if (_videoWriteStream == nullptr) {
		return 0u;
	}
	return _frameQueue.size();
}

int CaptureTool::encodeFrame(CaptureTool *inst) {
	core::SharedPtr<io::FileStream> s = inst->_videoWriteStream;
	while (!inst->_stop) {
		image::ImagePtr image;
		while (inst->_frameQueue.pop(image)) {
			if (inst->_type == CaptureType::AVI) {
				inst->_avi.writeFrame(*s.get(), image->data(), image->width(), image->height());
			} else {
				jo_write_mpeg(*s.get(), image->data(), image->width(), image->height(), inst->_fps);
			}
		}
	}
	return 0;
}

bool CaptureTool::startRecording(const char *filename, int width, int height) {
	_videoWriteStream = core::make_shared<io::FileStream>(io::filesystem()->open(filename, io::FileMode::SysWrite));
	if (!_videoWriteStream->valid()) {
		Log::error("Failed to open filestream for %s", filename);
		_videoWriteStream = nullptr;
		return false;
	}
	const core::VarPtr &fps = core::Var::getSafe(cfg::CoreMaxFPS);
	_fps = fps->intVal();
	if (_type == CaptureType::AVI) {
		if (!_avi.open(*_videoWriteStream.get(), width, height, _fps)) {
			Log::error("Failed to open avi in filestream for %s", filename);
			_videoWriteStream = nullptr;
			return false;
		}
	}
	Log::debug("Starting avirecorder thread");
	app::schedule([this]() { encodeFrame(this); });
	return true;
}

bool CaptureTool::stopRecording() {
	if (!isRecording()) {
		return false;
	}
	_stop = true;
	return true;
}

bool CaptureTool::hasFinished() const {
	if (!isRecording()) {
		return false;
	}
	if (!_stop) {
		return false;
	}
	return _frameQueue.empty();
}

bool CaptureTool::flush() {
	if (!isRecording()) {
		return true;
	}
	bool closed;
	if (_type == CaptureType::AVI)
		closed = _avi.close(*_videoWriteStream.get());
	else
		closed = true;
	_videoWriteStream = nullptr;
	return closed;
}

void CaptureTool::abort() {
	_frameQueue.clear();
	flush();
	_stop = true;
}

} // namespace image
