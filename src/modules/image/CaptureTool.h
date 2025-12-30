/**
 * @file
 */

#include "core/collection/ConcurrentQueue.h"
#include "core/concurrent/Atomic.h"
#include "image/AVI.h"
#include "image/Image.h"
#include "io/FileStream.h"

namespace image {

enum class CaptureType { AVI, MPEG2, GIF, Max };

/**
 * @brief Returns a sensible default FPS for the given capture type
 * GIF: 10 fps, AVI: 25 fps, MPEG2: 25 fps
 */
inline int defaultCaptureFps(CaptureType type) {
	switch (type) {
	case CaptureType::GIF:
		return 10;
	case CaptureType::AVI:
	case CaptureType::MPEG2:
		return 25;
	default:
		return 25;
	}
}

struct GifWriterWrapper;

class CaptureTool {
private:
	CaptureType _type = CaptureType::AVI;
	int _fps = 25;
	image::AVI _avi{};
	GifWriterWrapper* _gifWriter = nullptr;
	int _gifBits = 8;
	bool _gifDither = false;
	core::SharedPtr<io::FileStream> _videoWriteStream = nullptr;
	core::ConcurrentQueue<image::ImagePtr> _frameQueue;
	core::AtomicBool _stopAcceptingFrames = false;
	core::AtomicBool _running = false;
	double _lastFrameSeconds = -1.0;
	double _frameIntervalSeconds = 0.0;

	static int encodeFrame(CaptureTool *data);

public:
	CaptureTool() {
	}
	~CaptureTool() {
		abort();
	}

	inline CaptureType type() const {
		return _type;
	}

	inline int fps() const {
		return _fps;
	}

	/**
	 * @brief Check if enough time has passed since the last captured frame
	 * @param nowSeconds The current time in seconds
	 * @return @c true if a new frame should be captured based on the fps setting
	 */
	bool shouldCaptureFrame(double nowSeconds) const;

	bool isRecording() const;
	bool startRecording(const char *filename, int width, int height);
	void enqueueFrame(const image::ImagePtr &image, double nowSeconds);
	/**
	 * @brief Returns @c true if all queued frames were encoded and are part of the avi
	 * @note stopRecording() must have been called before!
	 */
	bool hasFinished() const;

	uint32_t pendingFrames() const;
	/**
	 * @brief Stop accepting new frames - continue to work on the pending frames
	 * @sa @c abort() if you want to stop immediately and discard pending frames
	 */
	bool stopRecording();
	/**
	 * @brief After the recording was done and stopRecording() was called, this returns @c true if everything was
	 * written.
	 * @note This is blocking until all frames were written - see hasFinished()
	 */
	bool flush();

	/**
	 * @sa @c stopRecording() if you want to stop gracefully and wait for pending frames to be processed
	 */
	void abort();
};

} // namespace image
