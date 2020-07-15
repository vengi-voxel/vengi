/**
 * @file
 */
#include "AINodeStaticResolver.h"
#include "ai-shared/protocol/AIStubTypes.h"
#include <QObject>
#include <QDebug>

namespace ai {
namespace debug {

namespace anon {
static const ai::AIStateNodeStatic UNKNOWN(-1, "unknown", "unknown", "unknown", "unknown", "unknown");
}

AINodeStaticResolver::AINodeStaticResolver() {
}

void AINodeStaticResolver::set(const std::vector<ai::AIStateNodeStatic>& data) {
	_data = data;
	_hash.clear();
	for (const ai::AIStateNodeStatic& s : _data) {
		_hash[s.getId()] = &s;
	}
	qDebug() << "received " << _hash.size() << " entries";
}

const ai::AIStateNodeStatic& AINodeStaticResolver::get(int32_t id) const {
	const ai::AIStateNodeStatic* s = _hash[id];
	if (s == nullptr) {
		qDebug() << "entry for " << id << " wasn't found";
		return anon::UNKNOWN;
	}
	return *s;
}

}
}
