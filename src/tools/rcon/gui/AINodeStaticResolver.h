/**
 * @file
 */
#pragma once

#include <QHash>
#include <vector>

namespace ai {
class AIStateNodeStatic;
}

namespace ai {
namespace debug {

class AINodeStaticResolver {
private:
	std::vector<ai::AIStateNodeStatic> _data;
	QHash<int32_t, const ai::AIStateNodeStatic*> _hash;
public:
	AINodeStaticResolver();

	void set(const std::vector<ai::AIStateNodeStatic>& data);
	const ai::AIStateNodeStatic& get(int32_t id) const;
};

}
}
