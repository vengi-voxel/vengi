/**
 * @file
 */

#include "http/HttpCacheStream.h"
#include "app/App.h"
#include "app/tests/AbstractTest.h"
#include "http/Request.h"
#include "io/FilesystemArchive.h"
#include "io/MemoryReadStream.h"
#include "core/ArrayLength.h"
#include "core/concurrent/Atomic.h"
#include "core/concurrent/Thread.h"
#include <gtest/gtest.h>

namespace http {

class HttpCacheStreamTest : public app::AbstractTest {};

namespace {

class CacheReadStream : public io::MemoryReadStream {
private:
	core::AtomicInt &_active;
	bool _delayed = false;

public:
	CacheReadStream(const core::String &data, core::AtomicInt &active, core::AtomicInt &overlapping)
		: io::MemoryReadStream(data.c_str(), data.size()), _active(active) {
		++_active;
		if ((int)_active > 1) {
			++overlapping;
		}
	}

	~CacheReadStream() override {
		--_active;
	}

	int read(void *data, size_t size) override {
		if (!_delayed) {
			_delayed = true;
			app::App::getInstance()->wait(1);
		}
		return io::MemoryReadStream::read(data, size);
	}
};

class ConcurrentCacheArchive : public io::Archive {
public:
	const core::String data = R"({"tree":[1,2,3]})";
	core::AtomicInt active;
	core::AtomicInt overlapping;

	void list(const core::String &, io::ArchiveFiles &, const core::String &) const override {
	}

	bool exists(const core::String &file) const override {
		return file == "metadata.json";
	}

	io::SeekableReadStream *readStream(const core::String &) override {
		return new CacheReadStream(data, active, overlapping);
	}
};

} // namespace

TEST_F(HttpCacheStreamTest, testConcurrentGetAndPostTextReadsCloseBeforeReopeningTheCache) {
	const core::SharedPtr<ConcurrentCacheArchive> archive = core::make_shared<ConcurrentCacheArchive>();
	core::AtomicInt ready;
	core::AtomicInt failures;
	core::AtomicBool start(false);
	core::Thread threads[8];
	for (int i = 0; i < lengthof(threads); ++i) {
		threads[i] = core::Thread([&, i]() {
			++ready;
			while (!start) {
				app::App::getInstance()->wait(1);
			}
			for (int round = 0; round < 16; ++round) {
				const core::String text = i % 2 == 0
					? HttpCacheStream::string(archive, "metadata.json", "https://unused.invalid/get")
					: HttpCacheStream::stringPost(archive, "metadata.json", "https://unused.invalid/post", "body");
				if (text != archive->data) {
					++failures;
				}
			}
		}, "http-cache-reader");
	}
	while ((int)ready < lengthof(threads)) {
		_testApp->wait(1);
	}
	start = true;
	for (core::Thread &thread : threads) {
		thread.join();
	}
	EXPECT_EQ(0, (int)failures);
	EXPECT_EQ(0, (int)archive->active);
	EXPECT_EQ(0, (int)archive->overlapping);
}

// disabled because it requires network access
TEST_F(HttpCacheStreamTest, DISABLED_testGetRequest) {
	if (!Request::supported()) {
		GTEST_SKIP() << "No http support available";
	}

	const core::String filename = "testGetRequest.json";

	if (io::filesystem()->exists(filename)) {
		ASSERT_TRUE(io::Filesystem::sysRemoveFile(io::filesystem()->homeWritePath(filename)));
	}
	{
		http::HttpCacheStream stream(io::openFilesystemArchive(_testApp->filesystem()), filename,
									 "https://httpbin.org/get");
		ASSERT_TRUE(stream.valid());
		ASSERT_TRUE(stream.isNewInCache());
		ASSERT_GT(stream.size(), 0);
	}
	{
		http::HttpCacheStream stream(io::openFilesystemArchive(_testApp->filesystem()), filename,
									 "https://httpbin.org/get");
		ASSERT_TRUE(stream.valid());
		ASSERT_FALSE(stream.isNewInCache());
		ASSERT_GT(stream.size(), 0);
	}
}

} // namespace http
