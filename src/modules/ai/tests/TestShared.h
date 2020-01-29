#pragma once

#include "core/tests/AbstractTest.h"
#include "TestEntity.h"
#include <sstream>
#include <iostream>

namespace glm {

inline void PrintTo(const vec2& v, ::std::ostream* os) {
	*os << "glm::vec2(" << v.x << ":" << v.y << ")";
}

inline void PrintTo(const vec3& v, ::std::ostream* os) {
	*os << "glm::vec3(" << v.x << ":" << v.y << ":" << v.z << ")";
}

inline void PrintTo(const vec4& v, ::std::ostream* os) {
	*os << "glm::vec4(" << v.x << ":" << v.y << ":" << v.z << ":" << v.w << ")";
}

}

inline bool operator==(const glm::vec3 &vecA, const glm::vec3 &vecB) {
	static const double epsilion = 0.0001;
	return fabs(vecA[0] - vecB[0]) < epsilion && fabs(vecA[1] - vecB[1]) < epsilion && fabs(vecA[2] - vecB[2]) < epsilion;
}

class TestSuite: public core::AbstractTest {
protected:
	ai::AIRegistry _registry;
	ai::GroupMgr _groupManager;

	core::String printAggroList(ai::AggroMgr& aggroMgr) const {
		const ai::AggroMgr::Entries& e = aggroMgr.getEntries();
		if (e.empty()) {
			return "empty";
		}

		std::stringstream s;
		for (ai::AggroMgr::Entries::const_iterator i = e.begin(); i != e.end(); ++i) {
			const ai::Entry& ep = *i;
			s << ep.getCharacterId() << "=" << ep.getAggro() << ", ";
		}
		const ai::EntryPtr& highest = aggroMgr.getHighestEntry();
		s << "highest: " << highest->getCharacterId() << "=" << highest->getAggro();
		return s.str();
	}

	virtual void SetUp() override;
	virtual void TearDown() override;
};
