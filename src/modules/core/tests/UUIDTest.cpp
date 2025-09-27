/**
 * @file
 */

#include "core/UUID.h"
#include "core/Hash.h"
#include <gtest/gtest.h>

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
    const String sample = generateUUID();
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

} // namespace core
