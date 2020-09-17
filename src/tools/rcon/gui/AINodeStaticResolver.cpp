/**
 * @file
 */
#include "AINodeStaticResolver.h"
#include "ai-shared/protocol/AIStubTypes.h"
#include "core/Log.h"

namespace ai {
namespace debug {

namespace anon {
static const ai::AIStateNodeStatic UNKNOWN(-1, "unknown", "unknown", "unknown", "unknown", "unknown");
}

AINodeStaticResolver::AINodeStaticResolver() {
}

void AINodeStaticResolver::set(const core::DynamicArray<ai::AIStateNodeStatic>& data) {
	_data = data;
	_hash.clear();
	for (const ai::AIStateNodeStatic& s : _data) {
		_hash.put(s.getId(), &s);
	}
	Log::debug("received %i entities", (int)_hash.size());
}

const ai::AIStateNodeStatic& AINodeStaticResolver::get(int32_t id) const {
	const ai::AIStateNodeStatic* s;
	if (_hash.get(id, s)) {
		Log::debug("entry for %i wasn't found", id);
		return anon::UNKNOWN;
	}
	return *s;
}

}
}
