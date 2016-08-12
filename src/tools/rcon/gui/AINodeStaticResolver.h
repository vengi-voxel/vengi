/**
 * @file
 */
#pragma once

#include <QHash>
#include <vector>

namespace ai {

class AIStateNodeStatic;

namespace debug {

class AINodeStaticResolver {
private:
	std::vector<AIStateNodeStatic> _data;
	QHash<int32_t, const AIStateNodeStatic*> _hash;
public:
	AINodeStaticResolver();

	void set(const std::vector<AIStateNodeStatic>& data);
	const AIStateNodeStatic& get(int32_t id) const;
};

}
}
