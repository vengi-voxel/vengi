/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Tokenizer.h"

namespace core {

TEST(TokenizerTest, testTokenizerSimple) {
	ASSERT_EQ(9u, core::Tokenizer("some nice string that is easy to be tokenized").size());
	ASSERT_EQ(3u, core::Tokenizer("foo()").size());
	ASSERT_EQ("foo", core::Tokenizer("foo()").next());
	ASSERT_EQ(5u, core::Tokenizer("a +foo\nb+bar\nc +foobar").size());
	ASSERT_EQ(1u, core::Tokenizer("\"somecommand +\"").size());
	ASSERT_EQ(2u, core::Tokenizer("\"somecommand +\" \"somecommand +\"").size());
	ASSERT_EQ(1u, core::Tokenizer("\"somecommand \\\"inner\\\"\"").size());
	ASSERT_EQ(3u, core::Tokenizer("1 \"somecommand \\\"inner\\\"\" 3").size());
	ASSERT_EQ(4u, core::Tokenizer("()()").size());
	ASSERT_EQ(0u, core::Tokenizer("\n").size());
	ASSERT_EQ("foo", core::Tokenizer("foo\n").next());
	ASSERT_EQ("foo", core::Tokenizer("\nfoo\n").next());
	ASSERT_EQ(4u, core::Tokenizer("{}{}").size());
	ASSERT_EQ(4u, core::Tokenizer("(){}").size());
	ASSERT_EQ(8u, core::Tokenizer("w +foo\nalt+a \"somecommand +\"\nCTRL+s +bar\nSHIFT+d +xyz\n").size());
}

}
