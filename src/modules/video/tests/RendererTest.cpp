/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "video/Renderer.h"

namespace video {

class RendererTest : public app::AbstractTest {
};

TEST_F(RendererTest, testMapType) {
	ASSERT_EQ(DataType::Float, mapType<decltype(glm::vec3::x)>());
}

TEST_F(RendererTest, testMapTypeStructOffset) {
	struct Buf {
		unsigned char c;
		int8_t b;
		uint8_t ub;
		int32_t i;
		uint32_t ui;
		int16_t s;
		uint16_t us;
	};
	ASSERT_EQ(DataType::UnsignedByte, mapType<decltype(Buf::c)>());
	ASSERT_EQ(DataType::Byte, mapType<decltype(Buf::b)>());
	ASSERT_EQ(DataType::UnsignedByte, mapType<decltype(Buf::ub)>());
	ASSERT_EQ(DataType::Int, mapType<decltype(Buf::i)>());
	ASSERT_EQ(DataType::UnsignedInt, mapType<decltype(Buf::ui)>());
	ASSERT_EQ(DataType::Short, mapType<decltype(Buf::s)>());
	ASSERT_EQ(DataType::UnsignedShort, mapType<decltype(Buf::us)>());
}

}
