/**
 * @file
 */

#include "CaptureTool.h"
#include "app/App.h"
#include "app/Async.h"
#include "core/Log.h"
#include "core/String.h"
#include "external/gif.h"
#include "external/jo_mpeg.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"

namespace image {

struct GifWriterWrapper {
	GifWriter writer;
};

bool CaptureTool::isRecording() const {
	if (_videoWriteStream == nullptr && _gifWriter == nullptr) {
		return false;
	}
	if (_stopAcceptingFrames) {
		return false;
	}
	return true;
}

bool CaptureTool::shouldCaptureFrame(double nowSeconds) const {
	if (!isRecording()) {
		return false;
	}
	if (_lastFrameSeconds < 0.0) {
		return true;
	}
	return (nowSeconds - _lastFrameSeconds) >= _frameIntervalSeconds;
}

void CaptureTool::enqueueFrame(const image::ImagePtr &image, double nowSeconds) {
	if (_stopAcceptingFrames) {
		Log::debug("CaptureTool is stopping, not accepting new frames");
		return;
	}
	if (!image || !image->isLoaded()) {
		return;
	}
	_lastFrameSeconds = nowSeconds;
	_frameQueue.push(image);
}

uint32_t CaptureTool::pendingFrames() const {
	if (_videoWriteStream == nullptr && _gifWriter == nullptr) {
		return 0u;
	}
	return _frameQueue.size();
}

int CaptureTool::encodeFrame(CaptureTool *inst) {
	// make a copy of the shared ptr to keep it alive
	core::SharedPtr<io::FileStream> s = inst->_videoWriteStream;
	while (!inst->_stopAcceptingFrames) {
		image::ImagePtr image;
		bool processed = false;
		while (inst->_frameQueue.pop(image)) {
			processed = true;
			if (inst->_type == CaptureType::AVI) {
				inst->_avi.writeFrame(*s.get(), image->data(), image->width(), image->height());
			} else if (inst->_type == CaptureType::MPEG2) {
				jo_write_mpeg(s.get(), image->data(), image->width(), image->height(), inst->_fps);
			} else if (inst->_type == CaptureType::GIF) {
				GifWriteFrame(&inst->_gifWriter->writer, image->data(), image->width(), image->height(),
							  100 / inst->_fps, inst->_gifBits, inst->_gifDither);
			}
		}
		if (!processed) {
			app::App::getInstance()->wait(10);
		}
	}
	// drain remaining frames after stop was requested
	image::ImagePtr image;
	while (inst->_frameQueue.pop(image)) {
		if (inst->_type == CaptureType::AVI) {
			inst->_avi.writeFrame(*s.get(), image->data(), image->width(), image->height());
		} else if (inst->_type == CaptureType::MPEG2) {
			jo_write_mpeg(s.get(), image->data(), image->width(), image->height(), inst->_fps);
		} else if (inst->_type == CaptureType::GIF) {
			GifWriteFrame(&inst->_gifWriter->writer, image->data(), image->width(), image->height(),
						  100 / inst->_fps, inst->_gifBits, inst->_gifDither);
		}
	}
	inst->_running = false;
	return 0;
}

bool CaptureTool::startRecording(const char *filename, int width, int height) {
	if (isRecording() || !hasFinished()) {
		Log::warn("CaptureTool is already recording");
		return false;
	}
	_type = image::CaptureType::Max;
	if (io::isA(filename, io::format::mpeg())) {
		_type = image::CaptureType::MPEG2;
	} else if (io::isA(filename, io::format::gif())) {
		_type = image::CaptureType::GIF;
	} else if (io::isA(filename, io::format::avi())) {
		_type = image::CaptureType::AVI;
	} else {
		Log::warn("Unknown capture type for extension '%s', defaulting to AVI", filename);
		_type = image::CaptureType::AVI;
	}

	_fps = defaultCaptureFps(_type);
	_frameIntervalSeconds = 1.0 / (double)_fps;
	_lastFrameSeconds = -1.0;

	if (_type == CaptureType::GIF) {
		_gifWriter = new GifWriterWrapper();
		if (!GifBegin(&_gifWriter->writer, filename, width, height, 100 / _fps)) {
			Log::error("Failed to open gif for %s", filename);
			delete _gifWriter;
			_gifWriter = nullptr;
			return false;
		}
		_stopAcceptingFrames = false;
		Log::debug("Starting gifrecorder thread");
		_running = true;
		app::schedule([this]() { encodeFrame(this); });
		return true;
	}

	const io::FilePtr &file = io::filesystem()->open(filename, io::FileMode::SysWrite);
	_videoWriteStream = core::make_shared<io::FileStream>(file);
	if (!_videoWriteStream->valid()) {
		Log::error("Failed to open filestream for %s", filename);
		_videoWriteStream = nullptr;
		return false;
	}
	if (_type == CaptureType::AVI) {
		if (!_avi.open(*_videoWriteStream.get(), width, height, _fps)) {
			Log::error("Failed to open avi in filestream for %s", filename);
			_videoWriteStream = nullptr;
			return false;
		}
	}
	_stopAcceptingFrames = false;
	Log::debug("Starting avirecorder thread");
	_running = true;
	app::schedule([this]() { encodeFrame(this); });
	return true;
}

bool CaptureTool::stopRecording() {
	if (!isRecording()) {
		return false;
	}
	_stopAcceptingFrames = true;
	return true;
}

bool CaptureTool::hasFinished() const {
	if (_videoWriteStream == nullptr && _gifWriter == nullptr) {
		return true;
	}
	if (!_stopAcceptingFrames) {
		return false;
	}
	return _frameQueue.empty() && !_running;
}

bool CaptureTool::flush() {
	if (_videoWriteStream == nullptr && _gifWriter == nullptr) {
		return true;
	}
	_stopAcceptingFrames = true;
	while (_running) {
		app::App::getInstance()->wait(10);
	}
	bool closed = true;
	if (_type == CaptureType::AVI) {
		closed = _avi.close(*_videoWriteStream.get());
	} else if (_type == CaptureType::GIF) {
		GifEnd(&_gifWriter->writer);
		delete _gifWriter;
		_gifWriter = nullptr;
	}
	_videoWriteStream = nullptr;
	return closed;
}

void CaptureTool::abort() {
	_stopAcceptingFrames = true;
	_frameQueue.clear();
	flush();
}

} // namespace image
