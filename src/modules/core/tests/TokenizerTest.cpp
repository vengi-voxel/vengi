/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Tokenizer.h"

namespace core {

TEST(TokenizerTest, testTokenizerEmptyLengthExceedsString) {
	core::Tokenizer t("", 100, ";");
	ASSERT_EQ(0u, t.size());
}

TEST(TokenizerTest, testTokenizerLengthExceedsString) {
	core::Tokenizer t("abc;def", 100, ";");
	ASSERT_EQ(2u, t.size());
}

TEST(TokenizerTest, testTokenizerOnlyFirstMatch) {
	core::Tokenizer t("abc;def", 3, ";");
	ASSERT_EQ(1u, t.size());
}

TEST(TokenizerTest, testTokenizerSecondMatchButEmptyString) {
	core::Tokenizer t("abc;def", 4, ";");
	ASSERT_EQ(2u, t.size());
	ASSERT_EQ(t.tokens()[0], "abc");
	ASSERT_EQ(t.tokens()[1], "");
}

TEST(TokenizerTest, testTokenizerSecondMatchButOnlyOneChar) {
	core::Tokenizer t("abc;def", 5, ";");
	ASSERT_EQ(2u, t.size());
	ASSERT_EQ(t.tokens()[0], "abc");
	ASSERT_EQ(t.tokens()[1], "d");
}

TEST(TokenizerTest, testTokenizerEmpty) {
	core::Tokenizer t("", ";");
	ASSERT_EQ(0u, t.size());
}

TEST(TokenizerTest, testTokenizerOnlySep) {
	core::Tokenizer t(";", ";");
	ASSERT_EQ(2u, t.size());
}

TEST(TokenizerTest, testTokenizerSepAndSplit) {
	core::Tokenizer t("int main(void) { foo; }", ";", "(){}");
	ASSERT_EQ(8u, t.size());
}

TEST(TokenizerTest, testTokenizerStrings) {
	core::Tokenizer t(";2;3;", ";");
	ASSERT_EQ(4u, t.size());
	ASSERT_EQ("", t.next());
	ASSERT_EQ("2", t.next());
	ASSERT_EQ("3", t.next());
	ASSERT_EQ("", t.next());
	ASSERT_FALSE(t.hasNext());
}

TEST(TokenizerTest, testTokenizerSimple) {
	ASSERT_EQ(9u, core::Tokenizer("some nice string that is easy to be tokenized").size());
	ASSERT_EQ(3u, core::Tokenizer("foo()").size());
	ASSERT_EQ("foo", core::Tokenizer("foo()").next());
	ASSERT_EQ(5u, core::Tokenizer("a +foo\nb+bar\nc +foobar").size());
	ASSERT_EQ(1u, core::Tokenizer("\"somecommand +\"").size());
	ASSERT_EQ(2u, core::Tokenizer("\"somecommand +\" \"somecommand +\"").size());
	ASSERT_EQ(1u, core::Tokenizer("\"somecommand \\\"inner\\\"\"").size());
	ASSERT_EQ(3u, core::Tokenizer("1 \"somecommand \\\"inner\\\"\" 3").size());
	ASSERT_EQ(5u, core::Tokenizer("()()").size());
	ASSERT_EQ(4u, core::Tokenizer("1;2;3;4", ";").size());
	ASSERT_EQ(4u, core::Tokenizer("1;2;3;", ";").size());
	ASSERT_EQ(4u, core::Tokenizer(";2;3;", ";").size());
	ASSERT_EQ(4u, core::Tokenizer(";;;", ";").size());
	ASSERT_EQ(0u, core::Tokenizer("", ";").size());
	ASSERT_EQ(1u, core::Tokenizer("foo", ";").size());
	ASSERT_EQ(0u, core::Tokenizer("\n").size());
	ASSERT_EQ("foo", core::Tokenizer("foo\n").next());
	ASSERT_EQ("foo", core::Tokenizer("\nfoo\n").next());
	ASSERT_EQ(5u, core::Tokenizer("{}{}").size());
	ASSERT_EQ(5u, core::Tokenizer("(){}").size());
	ASSERT_EQ(8u, core::Tokenizer("w +foo\nalt+a \"somecommand +\"\nCTRL+s +bar\nSHIFT+d +xyz\n").size());
	ASSERT_EQ(0u, core::Tokenizer("// empty").size());
	ASSERT_EQ(1u, core::Tokenizer("// empty\none").size());
	ASSERT_EQ("one", core::Tokenizer("// empty\none").next());
	ASSERT_EQ(0u, core::Tokenizer("/* empty\none */").size());
	ASSERT_EQ(1u, core::Tokenizer("/* empty\none */\nfoo").size());
	ASSERT_EQ(2u, core::Tokenizer("one// empty\ntwo").size());
	ASSERT_EQ("one", core::Tokenizer("one// empty\ntwo").next());
	ASSERT_EQ(1u, core::Tokenizer("one/* empty\ntwo */").size());
	ASSERT_EQ(2u, core::Tokenizer("one /* empty\ntwo */\nfoo").size());
	ASSERT_EQ("foo", core::Tokenizer("/* empty\none */\nfoo").next());
	ASSERT_EQ("bar", core::Tokenizer("/* empty\none */\n// foo\n bar").next());
}

}
