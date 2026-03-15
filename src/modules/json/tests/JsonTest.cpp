/**
 * @file
 */

#include "json/JSON.h"
#include <gtest/gtest.h>

namespace json {

// --- Parse ---

TEST(JsonTest, parseValidObject) {
	Json j = Json::parse(R"({"key":"value"})");
	ASSERT_TRUE(j.isValid());
	ASSERT_TRUE(j.isObject());
}

TEST(JsonTest, parseValidArray) {
	Json j = Json::parse(R"([1,2,3])");
	ASSERT_TRUE(j.isValid());
	ASSERT_TRUE(j.isArray());
}

TEST(JsonTest, parseInvalid) {
	Json j = Json::parse("{invalid json}");
	ASSERT_FALSE(j.isValid());
}

TEST(JsonTest, parseNull) {
	Json j = Json::parse((const char *)nullptr);
	ASSERT_FALSE(j.isValid());
}

TEST(JsonTest, parseEmpty) {
	Json j = Json::parse("");
	ASSERT_FALSE(j.isValid());
}

TEST(JsonTest, parseCoreString) {
	core::String s = R"({"a":1})";
	Json j = Json::parse(s);
	ASSERT_TRUE(j.isValid());
	EXPECT_EQ(1, j.get("a").intVal());
}

// --- accept ---

TEST(JsonTest, acceptValid) {
	EXPECT_TRUE(Json::accept(R"({"key":"value"})"));
	EXPECT_TRUE(Json::accept(R"([1,2,3])"));
	EXPECT_TRUE(Json::accept("42"));
	EXPECT_TRUE(Json::accept("\"string\""));
}

TEST(JsonTest, acceptInvalid) {
	EXPECT_FALSE(Json::accept("{bad}"));
	EXPECT_FALSE(Json::accept(nullptr));
}

// --- Factory methods ---

TEST(JsonTest, objectFactory) {
	Json j = Json::object();
	ASSERT_TRUE(j.isValid());
	ASSERT_TRUE(j.isObject());
	EXPECT_TRUE(j.empty());
	EXPECT_EQ(0, j.size());
}

TEST(JsonTest, arrayFactory) {
	Json j = Json::array();
	ASSERT_TRUE(j.isValid());
	ASSERT_TRUE(j.isArray());
	EXPECT_TRUE(j.empty());
	EXPECT_EQ(0, j.size());
}

// --- Type checks ---

TEST(JsonTest, typeCheckString) {
	Json j = Json::parse(R"({"s":"hello"})");
	Json s = j.get("s");
	EXPECT_TRUE(s.isString());
	EXPECT_FALSE(s.isNumber());
	EXPECT_FALSE(s.isBool());
	EXPECT_FALSE(s.isNull());
	EXPECT_FALSE(s.isObject());
	EXPECT_FALSE(s.isArray());
}

TEST(JsonTest, typeCheckNumber) {
	Json j = Json::parse(R"({"n":42})");
	Json n = j.get("n");
	EXPECT_TRUE(n.isNumber());
	EXPECT_TRUE(n.isNumberInteger());
}

TEST(JsonTest, typeCheckFloat) {
	Json j = Json::parse(R"({"f":3.14})");
	Json f = j.get("f");
	EXPECT_TRUE(f.isNumber());
	EXPECT_TRUE(f.isNumberFloat());
}

TEST(JsonTest, typeCheckBool) {
	Json j = Json::parse(R"({"b":true})");
	Json b = j.get("b");
	EXPECT_TRUE(b.isBool());
	EXPECT_FALSE(b.isNumber());
	EXPECT_FALSE(b.isString());
}

TEST(JsonTest, typeCheckNull) {
	Json j = Json::parse(R"({"n":null})");
	Json n = j.get("n");
	EXPECT_TRUE(n.isNull());
}

// --- contains / get ---

TEST(JsonTest, containsExisting) {
	Json j = Json::parse(R"({"a":1,"b":2})");
	EXPECT_TRUE(j.contains("a"));
	EXPECT_TRUE(j.contains("b"));
}

TEST(JsonTest, containsMissing) {
	Json j = Json::parse(R"({"a":1})");
	EXPECT_FALSE(j.contains("z"));
}

TEST(JsonTest, getByKeyExisting) {
	Json j = Json::parse(R"({"key":"val"})");
	Json v = j.get("key");
	ASSERT_TRUE(v.isValid());
	EXPECT_STREQ("val", v.cStr());
}

TEST(JsonTest, getByKeyMissing) {
	Json j = Json::parse(R"({"key":"val"})");
	Json v = j.get("missing");
	EXPECT_FALSE(v.isValid());
}

TEST(JsonTest, getByIndex) {
	Json j = Json::parse(R"([10,20,30])");
	EXPECT_EQ(10, j.get(0).intVal());
	EXPECT_EQ(20, j.get(1).intVal());
	EXPECT_EQ(30, j.get(2).intVal());
}

TEST(JsonTest, getByIndexOutOfBounds) {
	Json j = Json::parse(R"([1])");
	Json v = j.get(5);
	EXPECT_FALSE(v.isValid());
}

// --- Value extraction ---

TEST(JsonTest, strValue) {
	Json j = Json::parse(R"({"s":"hello"})");
	EXPECT_EQ("hello", j.get("s").str());
	EXPECT_STREQ("hello", j.get("s").cStr());
}

TEST(JsonTest, intValue) {
	Json j = Json::parse(R"({"n":42})");
	EXPECT_EQ(42, j.get("n").intVal());
}

TEST(JsonTest, doubleValue) {
	Json j = Json::parse(R"({"d":3.14})");
	EXPECT_NEAR(3.14, j.get("d").doubleVal(), 0.001);
}

TEST(JsonTest, floatValue) {
	Json j = Json::parse(R"({"f":2.5})");
	EXPECT_FLOAT_EQ(2.5f, j.get("f").floatVal());
}

TEST(JsonTest, boolValue) {
	Json j = Json::parse(R"({"t":true,"f":false})");
	EXPECT_TRUE(j.get("t").boolVal());
	EXPECT_FALSE(j.get("f").boolVal());
}

// --- Value with default ---

TEST(JsonTest, strValWithDefault) {
	Json j = Json::parse(R"({"a":"hello"})");
	EXPECT_EQ("hello", j.strVal("a", "fallback"));
	EXPECT_EQ("fallback", j.strVal("missing", "fallback"));
}

TEST(JsonTest, intValWithDefault) {
	Json j = Json::parse(R"({"n":5})");
	EXPECT_EQ(5, j.intVal("n", 0));
	EXPECT_EQ(99, j.intVal("missing", 99));
}

TEST(JsonTest, doubleValWithDefault) {
	Json j = Json::parse(R"({"d":1.5})");
	EXPECT_NEAR(1.5, j.doubleVal("d", 0.0), 0.001);
	EXPECT_NEAR(9.9, j.doubleVal("missing", 9.9), 0.001);
}

TEST(JsonTest, floatValWithDefault) {
	Json j = Json::parse(R"({"f":2.0})");
	EXPECT_FLOAT_EQ(2.0f, j.floatVal("f", 0.0f));
	EXPECT_FLOAT_EQ(7.0f, j.floatVal("missing", 7.0f));
}

TEST(JsonTest, boolValWithDefault) {
	Json j = Json::parse(R"({"b":true})");
	EXPECT_TRUE(j.boolVal("b", false));
	EXPECT_FALSE(j.boolVal("missing", false));
}

// --- Mutation: set ---

TEST(JsonTest, setString) {
	Json j = Json::object();
	j.set("key", "value");
	EXPECT_STREQ("value", j.get("key").cStr());
}

TEST(JsonTest, setCoreString) {
	Json j = Json::object();
	core::String s = "corestr";
	j.set("key", s);
	EXPECT_EQ("corestr", j.get("key").str());
}

TEST(JsonTest, setInt) {
	Json j = Json::object();
	j.set("n", 123);
	EXPECT_EQ(123, j.get("n").intVal());
}

TEST(JsonTest, setDouble) {
	Json j = Json::object();
	j.set("d", 2.718);
	EXPECT_NEAR(2.718, j.get("d").doubleVal(), 0.001);
}

TEST(JsonTest, setFloat) {
	Json j = Json::object();
	j.set("f", 1.5f);
	EXPECT_FLOAT_EQ(1.5f, j.get("f").floatVal());
}

TEST(JsonTest, setBool) {
	Json j = Json::object();
	j.set("t", true);
	j.set("f", false);
	EXPECT_TRUE(j.get("t").boolVal());
	EXPECT_FALSE(j.get("f").boolVal());
}

TEST(JsonTest, setJson) {
	Json j = Json::object();
	Json child = Json::object();
	child.set("inner", 42);
	j.set("nested", child);
	EXPECT_EQ(42, j.get("nested").get("inner").intVal());
}

TEST(JsonTest, setOverwrite) {
	Json j = Json::object();
	j.set("k", 1);
	EXPECT_EQ(1, j.get("k").intVal());
	j.set("k", 2);
	EXPECT_EQ(2, j.get("k").intVal());
}

// --- Array operations ---

TEST(JsonTest, pushInt) {
	Json arr = Json::array();
	arr.push(1);
	arr.push(2);
	arr.push(3);
	EXPECT_EQ(3, arr.size());
	EXPECT_EQ(1, arr.get(0).intVal());
	EXPECT_EQ(2, arr.get(1).intVal());
	EXPECT_EQ(3, arr.get(2).intVal());
}

TEST(JsonTest, pushString) {
	Json arr = Json::array();
	arr.push("hello");
	arr.push("world");
	EXPECT_EQ(2, arr.size());
	EXPECT_STREQ("hello", arr.get(0).cStr());
	EXPECT_STREQ("world", arr.get(1).cStr());
}

TEST(JsonTest, pushCoreString) {
	Json arr = Json::array();
	core::String s = "test";
	arr.push(s);
	EXPECT_EQ(1, arr.size());
	EXPECT_EQ("test", arr.get(0).str());
}

TEST(JsonTest, pushDouble) {
	Json arr = Json::array();
	arr.push(1.5);
	EXPECT_NEAR(1.5, arr.get(0).doubleVal(), 0.001);
}

TEST(JsonTest, pushFloat) {
	Json arr = Json::array();
	arr.push(2.5f);
	EXPECT_FLOAT_EQ(2.5f, arr.get(0).floatVal());
}

TEST(JsonTest, pushBool) {
	Json arr = Json::array();
	arr.push(true);
	arr.push(false);
	EXPECT_TRUE(arr.get(0).boolVal());
	EXPECT_FALSE(arr.get(1).boolVal());
}

TEST(JsonTest, pushJson) {
	Json arr = Json::array();
	Json obj = Json::object();
	obj.set("x", 10);
	arr.push(obj);
	EXPECT_EQ(1, arr.size());
	EXPECT_EQ(10, arr.get(0).get("x").intVal());
}

TEST(JsonTest, pushBackAlias) {
	Json arr = Json::array();
	arr.push_back(42);
	arr.push_back("str");
	EXPECT_EQ(2, arr.size());
	EXPECT_EQ(42, arr.get(0).intVal());
	EXPECT_STREQ("str", arr.get(1).cStr());
}

// --- size / empty ---

TEST(JsonTest, sizeObject) {
	Json j = Json::parse(R"({"a":1,"b":2,"c":3})");
	EXPECT_EQ(3, j.size());
	EXPECT_FALSE(j.empty());
}

TEST(JsonTest, sizeArray) {
	Json j = Json::parse(R"([1,2])");
	EXPECT_EQ(2, j.size());
	EXPECT_FALSE(j.empty());
}

TEST(JsonTest, emptyObject) {
	Json j = Json::object();
	EXPECT_TRUE(j.empty());
	EXPECT_EQ(0, j.size());
}

TEST(JsonTest, emptyArray) {
	Json j = Json::array();
	EXPECT_TRUE(j.empty());
	EXPECT_EQ(0, j.size());
}

// --- dump ---

TEST(JsonTest, dumpRoundTrip) {
	const char *input = R"({"a":1,"b":"two","c":true})";
	Json j = Json::parse(input);
	core::String out = j.dump();
	Json j2 = Json::parse(out);
	ASSERT_TRUE(j2.isValid());
	EXPECT_EQ(1, j2.get("a").intVal());
	EXPECT_EQ("two", j2.get("b").str());
	EXPECT_TRUE(j2.get("c").boolVal());
}

TEST(JsonTest, dumpFormatted) {
	Json j = Json::object();
	j.set("x", 1);
	core::String formatted = j.dump(2);
	// Formatted output should contain newlines and indentation
	EXPECT_NE(core::String::npos, formatted.find("\n"));
}

TEST(JsonTest, dumpArray) {
	Json arr = Json::array();
	arr.push(1);
	arr.push(2);
	core::String out = arr.dump();
	Json j2 = Json::parse(out);
	ASSERT_TRUE(j2.isValid());
	EXPECT_EQ(2, j2.size());
}

// --- Copy semantics ---

TEST(JsonTest, copyConstruct) {
	Json j = Json::parse(R"({"key":"val"})");
	Json copy = j;
	ASSERT_TRUE(copy.isValid());
	EXPECT_EQ("val", copy.get("key").str());
	// Verify independence: modifying copy doesn't affect original
	copy.set("key", "changed");
	EXPECT_EQ("val", j.get("key").str());
	EXPECT_EQ("changed", copy.get("key").str());
}

TEST(JsonTest, copyAssign) {
	Json j = Json::parse(R"({"a":1})");
	Json other = Json::object();
	other.set("b", 2);
	other = j;
	ASSERT_TRUE(other.isValid());
	EXPECT_EQ(1, other.get("a").intVal());
}

// --- Move semantics ---

TEST(JsonTest, moveConstruct) {
	Json j = Json::parse(R"({"key":1})");
	Json moved(core::move(j));
	ASSERT_TRUE(moved.isValid());
	EXPECT_EQ(1, moved.get("key").intVal());
}

TEST(JsonTest, moveAssign) {
	Json j = Json::parse(R"({"a":1})");
	Json target = Json::object();
	target = core::move(j);
	ASSERT_TRUE(target.isValid());
	EXPECT_EQ(1, target.get("a").intVal());
}

// --- Iterator ---

TEST(JsonTest, iterateArray) {
	Json arr = Json::parse(R"([10,20,30])");
	int sum = 0;
	int count = 0;
	for (const Json &item : arr) {
		sum += item.intVal();
		++count;
	}
	EXPECT_EQ(3, count);
	EXPECT_EQ(60, sum);
}

TEST(JsonTest, iterateObject) {
	Json j = Json::parse(R"({"a":1,"b":2})");
	int count = 0;
	for (auto it = j.begin(); it != j.end(); ++it) {
		core::String k = it.key();
		EXPECT_TRUE(k == "a" || k == "b");
		Json v = *it;
		EXPECT_TRUE(v.intVal() == 1 || v.intVal() == 2);
		++count;
	}
	EXPECT_EQ(2, count);
}

TEST(JsonTest, iterateEmpty) {
	Json j = Json::object();
	int count = 0;
	for (auto it = j.begin(); it != j.end(); ++it) {
		++count;
	}
	EXPECT_EQ(0, count);
}

// --- toStr helpers ---

TEST(JsonTest, toStrByKey) {
	Json j = Json::parse(R"({"name":"test"})");
	EXPECT_EQ("test", toStr(j, "name"));
}

TEST(JsonTest, toStrByKeyDefault) {
	Json j = Json::parse(R"({})");
	EXPECT_EQ("fallback", toStr(j, "missing", "fallback"));
}

TEST(JsonTest, toStrDirect) {
	Json j = Json::parse(R"({"s":"hello"})");
	EXPECT_EQ("hello", toStr(j.get("s")));
}

// --- Nested structures ---

TEST(JsonTest, nestedObjectAccess) {
	Json j = Json::parse(R"({"outer":{"inner":{"deep":99}}})");
	EXPECT_EQ(99, j.get("outer").get("inner").get("deep").intVal());
}

TEST(JsonTest, arrayOfObjects) {
	Json j = Json::parse(R"([{"id":1},{"id":2},{"id":3}])");
	ASSERT_EQ(3, j.size());
	for (int i = 0; i < j.size(); ++i) {
		EXPECT_EQ(i + 1, j.get(i).get("id").intVal());
	}
}

TEST(JsonTest, objectWithArray) {
	Json j = Json::parse(R"({"items":[10,20,30],"count":3})");
	EXPECT_EQ(3, j.get("count").intVal());
	Json items = j.get("items");
	ASSERT_EQ(3, items.size());
	EXPECT_EQ(10, items.get(0).intVal());
}

// --- Default constructed ---

TEST(JsonTest, defaultConstruct) {
	Json j;
	EXPECT_FALSE(j.isValid());
	EXPECT_TRUE(j.empty());
	EXPECT_EQ(0, j.size());
}

// --- Build and dump complex structure ---

TEST(JsonTest, buildComplexAndRoundTrip) {
	Json root = Json::object();
	root.set("name", "test");
	root.set("version", 2);
	root.set("enabled", true);

	Json tags = Json::array();
	tags.push("alpha");
	tags.push("beta");
	root.set("tags", tags);

	Json nested = Json::object();
	nested.set("x", 1.0);
	nested.set("y", 2.0);
	root.set("position", nested);

	core::String serialized = root.dump();
	Json restored = Json::parse(serialized);

	ASSERT_TRUE(restored.isValid());
	EXPECT_EQ("test", restored.strVal("name", ""));
	EXPECT_EQ(2, restored.intVal("version", 0));
	EXPECT_TRUE(restored.boolVal("enabled", false));
	EXPECT_EQ(2, restored.get("tags").size());
	EXPECT_NEAR(1.0, restored.get("position").doubleVal("x", 0.0), 0.001);
	EXPECT_NEAR(2.0, restored.get("position").doubleVal("y", 0.0), 0.001);
}

} // namespace json
