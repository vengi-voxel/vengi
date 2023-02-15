/**
 * @file
 */

#include "core/collection/ConcurrentQueue.h"
#include "core/concurrent/Atomic.h"
#include "image/AVI.h"
#include "image/Image.h"
#include "io/FileStream.h"

namespace core {
class Thread;
}

namespace image {

class AVIRecorder {
private:
	image::AVI _avi;
	io::FileStream *_videoWriteStream = nullptr;
	core::ConcurrentQueue<image::ImagePtr> _frameQueue;
	core::AtomicBool _stop = false;
	core::Thread *_thread = nullptr;

	static int encodeFrame(void *data);

public:
	~AVIRecorder() {
		abort();
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
