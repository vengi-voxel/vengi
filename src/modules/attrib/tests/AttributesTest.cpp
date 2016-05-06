/**
 * @file
 */

#include <gtest/gtest.h>
#include "attrib/Attributes.h"

namespace attrib {

TEST(Attributes, testCurrents) {
	Attributes attributes;
	ContainerBuilder t("test");
	t.addPercentage(Types::HEALTH, 100).addAbsolute(Types::HEALTH, 10);
	ASSERT_FALSE(attributes.onFrame(1L));
	attributes.add(t.create());
	ASSERT_TRUE(attributes.onFrame(1L));
	ASSERT_EQ(20, attributes.getMax(Types::HEALTH));
	ASSERT_EQ(20, attributes.setCurrent(Types::HEALTH, 100));
}

TEST(Attributes, testAddRemove) {
	Attributes attributes;
	ContainerBuilder test1("test1");
	test1.addAbsolute(Types::HEALTH, 1);
	attributes.add(test1.create());
	ASSERT_TRUE(attributes.onFrame(1L));
	ASSERT_EQ(1, attributes.getMax(Types::HEALTH));

	ContainerBuilder test2("test2");
	test2.addAbsolute(Types::HEALTH, 1);
	attributes.add(test2.create());
	ASSERT_TRUE(attributes.onFrame(1L));
	ASSERT_EQ(2, attributes.getMax(Types::HEALTH));

	ContainerBuilder test1Remove("test1");
	attributes.remove(test1Remove.create());
	ASSERT_TRUE(attributes.onFrame(1L));
	ASSERT_EQ(1, attributes.getMax(Types::HEALTH));
}

}
