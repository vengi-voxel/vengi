/**
 * @file
 */

#pragma once

#include "app/tests/AbstractTest.h"
#include "core/collection/Map.h"

namespace http {

class AbstractHttpParserTest : public core::AbstractTest {
public:
	void validateMapEntry(const core::CharPointerMap& map, const char *key, const char *value) {
		const char* mapValue = "";
		EXPECT_TRUE(map.get(key, mapValue)) << printMap(map);
		EXPECT_STREQ(value, mapValue);
	}

	core::String printMap(const core::CharPointerMap& map) const {
		std::stringstream ss;
		ss << "Found map entries are: \"";
		bool first = true;
		for (const auto& iter : map) {
			if (!first) {
				ss << ", ";
			}
			ss << iter->first << ": " << iter->second;
			first = false;
		}
		ss << "\"";
		return core::String(ss.str().c_str());
	}
};

}