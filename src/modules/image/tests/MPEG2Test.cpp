/**
 * @file
 */

#include "image/external/jo_mpeg.h"
#include "app/App.h"
#include "app/tests/AbstractTest.h"
#include "color/RGBA.h"
#include "io/File.h"
#include "io/FileStream.h"

namespace image {

class MPEG2Test : public app::AbstractTest {};

TEST_F(MPEG2Test, testCreate) {
	const io::FilePtr &file = io::filesystem()->open("test.mpeg", io::FileMode::SysWrite);
	io::FileStream stream(file);
	ASSERT_TRUE(stream.valid());

	const core::RGBA r = core::RGBA(255, 0, 0);
	const core::RGBA b = core::RGBA(0, 0, 0);
	const core::RGBA img1[]{
		r, r, b, b, b, b,
		r, r, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
	};
	const core::RGBA img2[]{
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		r, r, b, b, b, b,
		r, r, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
	};
	const core::RGBA img3[]{
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, r, r, b, b,
		b, b, r, r, b, b,
	};
	for (int i = 0; i < 100; ++i) {
		ASSERT_TRUE(jo_write_mpeg(stream, (const uint8_t *)img1, 6, 6, 3));
		ASSERT_TRUE(jo_write_mpeg(stream, (const uint8_t *)img2, 6, 6, 3));
		ASSERT_TRUE(jo_write_mpeg(stream, (const uint8_t *)img3, 6, 6, 3));
	}
}

} // namespace image
