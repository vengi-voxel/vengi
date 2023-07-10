/**
 * @file
 */

#include "voxelformat/private/voxelmax/BinaryPList.h"
#include "app/App.h"
#include "app/tests/AbstractTest.h"
#include "core/Log.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"

namespace voxelformat {

class BinaryPListTest : public app::AbstractTest {};

TEST_F(BinaryPListTest, testRead) {
	io::FileStream stream(io::filesystem()->open("test.plist", io::FileMode::Read));
	const priv::BinaryPList &plist = priv::BinaryPList::parse(stream);
	ASSERT_TRUE(plist.isDict());
	ASSERT_EQ(3u, plist.asDict().size());

	auto travelLog = plist.asDict().find("Travel Log");
	ASSERT_NE(travelLog, plist.asDict().end());
	ASSERT_TRUE(travelLog->value.isArray());
	const priv::PListArray &travelLogArray = travelLog->value.asArray();
	ASSERT_EQ(3u, travelLogArray.size());
	ASSERT_TRUE(travelLogArray[0].isString());
	ASSERT_TRUE(travelLogArray[1].isString());
	ASSERT_TRUE(travelLogArray[2].isString());
	ASSERT_STREQ("Tokyo, Honshu, Japan", travelLogArray[0].asString().c_str());
	ASSERT_STREQ("Philadelphia, PA", travelLogArray[1].asString().c_str());
	ASSERT_STREQ("Recife, Pernambuco, Brazil", travelLogArray[2].asString().c_str());

	auto birthYear = plist.asDict().find("Birth Year");
	ASSERT_NE(birthYear, plist.asDict().end());
	ASSERT_TRUE(birthYear->second.isInt());
	ASSERT_EQ(1942u, birthYear->second.asInt());

	auto name = plist.asDict().find("Name");
	ASSERT_NE(name, plist.asDict().end());
	ASSERT_STREQ("John Doe", name->second.asString().c_str());
}

TEST_F(BinaryPListTest, testReadVMaxPalette) {
	io::FileStream stream(io::filesystem()->open("palette.settings.vmaxpsb", io::FileMode::Read));
	const priv::BinaryPList &plist = priv::BinaryPList::parse(stream);
	ASSERT_TRUE(plist.isDict());
	ASSERT_EQ(11u, plist.asDict().size());
	ASSERT_TRUE(plist.asDict().hasKey("materials"));
	ASSERT_TRUE(plist.asDict().hasKey("name"));
}

} // namespace voxelformat
