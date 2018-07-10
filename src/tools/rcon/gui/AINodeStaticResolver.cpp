/**
 * @file
 */
#include "AINodeStaticResolver.h"
#include <server/AIStubTypes.h>
#include <QObject>
#include <QDebug>

namespace ai {
namespace debug {

namespace anon {
static const AIStateNodeStatic UNKNOWN(-1, "unknown", "unknown", "unknown", "unknown", "unknown");
}

AINodeStaticResolver::AINodeStaticResolver() {
}

void AINodeStaticResolver::set(const std::vector<AIStateNodeStatic>& data) {
	_data = data;
	_hash.clear();
	for (const AIStateNodeStatic& s : _data) {
		_hash[s.getId()] = &s;
	}
	qDebug() << "received " << _hash.size() << " entries";
}

const AIStateNodeStatic& AINodeStaticResolver::get(int32_t id) const {
	const AIStateNodeStatic* s = _hash[id];
	if (s == nullptr) {
		qDebug() << "entry for " << id << " wasn't found";
		return anon::UNKNOWN;
	}
	return *s;
}

}
}
