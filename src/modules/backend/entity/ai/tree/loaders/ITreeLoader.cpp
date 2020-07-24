/**
 * @file
 */

#include "ITreeLoader.h"
#include "core/StandardLib.h"

namespace backend {

void ITreeLoader::resetError() {
	core::ScopedLock scopedLock(_lock);
	_error = "";
}

ITreeLoader::ITreeLoader(const IAIFactory& aiFactory) :
		_aiFactory(aiFactory) {
}

ITreeLoader::~ITreeLoader() {
	_error = "";
	_treeMap.clear();
}

void ITreeLoader::shutdown() {
	core::ScopedLock scopedLock(_lock);
	_error = "";
	_treeMap.clear();
}

bool ITreeLoader::addTree(const core::String& name, const TreeNodePtr& root) {
	if (!root) {
		return false;
	}
	core::ScopedLock scopedLock(_lock);
	auto i = _treeMap.find(name);
	if (i != _treeMap.end()) {
		return false;
	}
	_treeMap.put(name, root);
	return true;
}

TreeNodePtr ITreeLoader::load(const core::String &name) {
	core::ScopedLock scopedLock(_lock);
	auto i = _treeMap.find(name);
	if (i != _treeMap.end())
		return i->second;
	return TreeNodePtr();
}

void ITreeLoader::setError(const char* msg, ...) {
	va_list args;
	va_start(args, msg);
	char buf[1024];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	core::ScopedLock scopedLock(_lock);
	_error = buf;
}

}
