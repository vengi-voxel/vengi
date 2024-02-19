/**
 * @file
 */

#include "AVIRecorder.h"
#include "app/App.h"
#include "app/Async.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/Var.h"
#include "io/Filesystem.h"

namespace image {

bool AVIRecorder::isRecording() const {
	if (_videoWriteStream == nullptr) {
		return false;
	}
	if (_stop) {
		return false;
	}
	return true;
}

void AVIRecorder::enqueueFrame(const image::ImagePtr &image) {
	if (!image || !image->isLoaded()) {
		return;
	}
	_frameQueue.push(image);
}

uint32_t AVIRecorder::pendingFrames() const {
	if (_videoWriteStream == nullptr) {
		return 0u;
	}
	return _frameQueue.size();
}

int AVIRecorder::encodeFrame(AVIRecorder *inst) {
	core::SharedPtr<io::FileStream> s = inst->_videoWriteStream;
	while (!inst->_stop) {
		image::ImagePtr image;
		while (inst->_frameQueue.pop(image)) {
			inst->_avi.writeFrame(*s.get(), image->data(), image->width(), image->height());
		}
	}
	return 0;
}

bool AVIRecorder::startRecording(const char *filename, int width, int height) {
	_videoWriteStream = core::make_shared<io::FileStream>(io::filesystem()->open(filename, io::FileMode::SysWrite));
	if (!_videoWriteStream->valid()) {
		Log::error("Failed to open filestream for %s", filename);
		_videoWriteStream = nullptr;
		return false;
	}
	const core::VarPtr &fps = core::Var::getSafe(cfg::CoreMaxFPS);
	if (!_avi.open(*_videoWriteStream.get(), width, height, fps->intVal())) {
		Log::error("Failed to open avi in filestream for %s", filename);
		_videoWriteStream = nullptr;
		return false;
	}
	Log::debug("Starting avirecorder thread");
	app::async(encodeFrame, this);
	return true;
}

bool AVIRecorder::stopRecording() {
	if (!isRecording()) {
		return false;
	}
	_stop = true;
	return true;
}

bool AVIRecorder::hasFinished() const {
	if (!isRecording()) {
		return false;
	}
	if (!_stop) {
		return false;
	}
	return _frameQueue.empty();
}

bool AVIRecorder::flush() {
	if (!isRecording()) {
		return true;
	}
	bool closed = _avi.close(*_videoWriteStream.get());
	_videoWriteStream = nullptr;
	return closed;
}

void AVIRecorder::abort() {
	_frameQueue.clear();
	flush();
	_stop = true;
}

} // namespace image
