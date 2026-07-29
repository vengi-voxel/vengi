/**
 * @file
 */

#include "core/UUID.h"
#include <gtest/gtest.h>
#include <type_traits>

namespace core {

TEST(UUIDTest, testGenerateAndString) {
	UUID u = UUID::generate();
	const String &s = u.str();
	// RFC4122 UUID string length
	ASSERT_EQ(36u, s.size());
	// dashes at expected positions
	EXPECT_EQ('-', s[8]);
	EXPECT_EQ('-', s[13]);
	EXPECT_EQ('-', s[18]);
	EXPECT_EQ('-', s[23]);
}

TEST(UUIDTest, testParseAndEquality) {
	const String sample = core::UUID::generate().str();
	UUID a(sample);
	UUID b;
	b = sample;
	EXPECT_EQ(a, b);
	EXPECT_EQ(a.str(), b.str());
}

TEST(UUIDTest, testInvalidParse) {
	UUID u("not-a-uuid");
	EXPECT_FALSE(u.isValid());
	EXPECT_EQ(0u, u.str().size());
}

TEST(UUIDTest, testTrivialCopyable) {
	EXPECT_EQ(16u, sizeof(UUID));
	EXPECT_TRUE(std::is_trivially_copyable<UUID>::value);
	EXPECT_TRUE(std::is_trivially_copy_constructible<UUID>::value);
	EXPECT_TRUE(std::is_trivially_move_constructible<UUID>::value);
	UUID a = UUID::generate();
	UUID b = a;
	EXPECT_EQ(a, b);
	UUID c(a.data0(), a.data1());
	EXPECT_EQ(a, c);
}

} // namespace core
