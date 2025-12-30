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

struct GifWriterWrapper;

class CaptureTool {
private:
	CaptureType _type = CaptureType::AVI;
	int _fps = 30;
	image::AVI _avi{};
	GifWriterWrapper* _gifWriter = nullptr;
	int _gifBits = 8;
	bool _gifDither = false;
	core::SharedPtr<io::FileStream> _videoWriteStream = nullptr;
	core::ConcurrentQueue<image::ImagePtr> _frameQueue;
	core::AtomicBool _stop = false;
	core::AtomicBool _running = false;

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

	bool isRecording() const;
	bool startRecording(const char *filename, int width, int height);
	void enqueueFrame(const image::ImagePtr &image);
	/**
	 * @brief Returns @c true if all queued frames were encoded and are part of the avi
	 * @note stopRecording() must have been called before!
	 */
	bool hasFinished() const;

	uint32_t pendingFrames() const;
	/**
	 * @brief Stop the jpeg encoding thread and prepare the avi stream to be closed.
	 */
	bool stopRecording();
	/**
	 * @brief After the recording was done and stopRecording() was called, this returns @c true if everything was
	 * written.
	 * @note This is blocking until all frames were written - see hasFinished()
	 */
	bool flush();

	void abort();
};

} // namespace image
