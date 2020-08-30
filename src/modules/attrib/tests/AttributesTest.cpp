/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "attrib/Attributes.h"

namespace attrib {

class AttributesTest: public app::AbstractTest {
};

TEST_F(AttributesTest, testCurrents) {
	Attributes attributes;
	ContainerBuilder t("test");
	t.addPercentage(Type::HEALTH, 100).addAbsolute(Type::HEALTH, 10);
	ASSERT_FALSE(attributes.update(1L));
	attributes.add(t.create());
	ASSERT_TRUE(attributes.update(1L));
	ASSERT_EQ(20, attributes.max(Type::HEALTH));
	ASSERT_EQ(20, attributes.setCurrent(Type::HEALTH, 100));
}

TEST_F(AttributesTest, testAddRemove) {
	Attributes attributes;
	ContainerBuilder test1("test1");
	test1.addAbsolute(Type::HEALTH, 1);
	attributes.add(test1.create());
	ASSERT_TRUE(attributes.update(1L));
	ASSERT_EQ(1, attributes.max(Type::HEALTH));

	ContainerBuilder test2("test2");
	test2.addAbsolute(Type::HEALTH, 1);
	attributes.add(test2.create());
	ASSERT_TRUE(attributes.update(1L));
	ASSERT_EQ(2, attributes.max(Type::HEALTH));

	ContainerBuilder test1Remove("test1");
	attributes.remove(test1Remove.create());
	ASSERT_TRUE(attributes.update(1L));
	ASSERT_EQ(1, attributes.max(Type::HEALTH));
}

TEST_F(AttributesTest, testParent) {
	Attributes parent;
	parent.setName("parent");
	ContainerBuilder test1("test1");
	test1.addAbsolute(Type::HEALTH, 1);
	parent.add(test1.create());

	Attributes attributes(&parent);
	attributes.setName("parent");
	ASSERT_TRUE(attributes.update(1L));
	ASSERT_EQ(1, attributes.max(Type::HEALTH));
}

TEST_F(AttributesTest, testCappedCurrent) {
	Attributes attributes;
	ContainerBuilder test1("test1");
	test1.addAbsolute(Type::HEALTH, 1);
	attributes.add(test1.create());

	ASSERT_TRUE(attributes.update(1L));
	ASSERT_EQ(1, attributes.max(Type::HEALTH));
	ASSERT_EQ(1, attributes.setCurrent(Type::HEALTH, 2));
}

TEST_F(AttributesTest, testParentPercentage) {
	Attributes parent;
	parent.setName("parent");
	ContainerBuilder test1("test1");
	test1.addAbsolute(Type::HEALTH, 1);
	test1.addPercentage(Type::HEALTH, 100.0);
	parent.add(test1.create());

	Attributes attributes(&parent);
	ASSERT_TRUE(attributes.update(1L));
	ASSERT_EQ(2, attributes.max(Type::HEALTH));
}

TEST_F(AttributesTest, testParentAndOwnPercentage) {
	Attributes parent;
	parent.setName("parent");
	ContainerBuilder test1("test1");
	test1.addAbsolute(Type::HEALTH, 1);
	test1.addPercentage(Type::HEALTH, 100.0);
	parent.add(test1.create());

	Attributes attributes(&parent);

	ContainerBuilder test2("test2");
	test2.addAbsolute(Type::HEALTH, 99);
	test2.addPercentage(Type::HEALTH, 10.0);
	attributes.add(test2.create());

	ASSERT_TRUE(attributes.update(1L));
	ASSERT_EQ(210, attributes.max(Type::HEALTH));
	ASSERT_EQ(2, parent.max(Type::HEALTH));
}

TEST_F(AttributesTest, testStackCount) {
	Attributes attributes;
	ContainerBuilder test1("test1", 4);
	test1.addAbsolute(Type::HEALTH, 1);

	attributes.add(test1.create());
	ASSERT_TRUE(attributes.update(1L));
	ASSERT_EQ(1, attributes.max(Type::HEALTH));
	ASSERT_EQ(1, attributes.setCurrent(Type::HEALTH, 2));

	attributes.add(test1.create());
	ASSERT_TRUE(attributes.update(1L));
	ASSERT_EQ(2, attributes.max(Type::HEALTH));
	ASSERT_EQ(2, attributes.setCurrent(Type::HEALTH, 3));

	attributes.add(test1.create());
	ASSERT_TRUE(attributes.update(1L));
	ASSERT_EQ(3, attributes.max(Type::HEALTH));
	ASSERT_EQ(3, attributes.setCurrent(Type::HEALTH, 4));

	attributes.add(test1.create());
	ASSERT_TRUE(attributes.update(1L));
	ASSERT_EQ(4, attributes.max(Type::HEALTH));
	ASSERT_EQ(4, attributes.setCurrent(Type::HEALTH, 5));

	attributes.add(test1.create());
	ASSERT_FALSE(attributes.update(1L));
	ASSERT_EQ(4, attributes.max(Type::HEALTH));
	ASSERT_EQ(4, attributes.setCurrent(Type::HEALTH, 6));
}

TEST_F(AttributesTest, testListeners) {
	Attributes parent;
	parent.setName("parent");
	ContainerBuilder test1("test1");
	test1.addAbsolute(Type::SPEED, 1);
	test1.addPercentage(Type::HEALTH, 100.0);
	parent.add(test1.create());

	int changes[static_cast<int>(Type::MAX)];
	SDL_zero(changes);
	Attributes attributes(&parent);
	attributes.addListener([&] (const DirtyValue& v) {
		++changes[static_cast<int>(v.type)];
	});

	ContainerBuilder test2("test2");
	test2.addAbsolute(Type::HEALTH, 100);
	test2.addPercentage(Type::HEALTH, 10.0);
	attributes.add(test2.create());

	ASSERT_TRUE(attributes.update(1L));

	ASSERT_EQ(changes[static_cast<int>(Type::HEALTH)], 1);
	ASSERT_EQ(changes[static_cast<int>(Type::SPEED)], 1);
}

}
