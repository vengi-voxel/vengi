/**
 * @file
 */

#include "MeshPool.h"
#include "io/Filesystem.h"
#include "core/App.h"
#include <assimp/importerdesc.h>

namespace video {

static const char* supportedFormats[] = { "dae", "fbx", "DAE", "FBX", "md5mesh", nullptr };

MeshPool::MeshPool() {
}

MeshPool::~MeshPool() {
	shutdown();
}

bool MeshPool::init() {
	bool state = true;
	for (const char **format = supportedFormats; *format != nullptr; format++) {
		const std::string& f = core::string::toLower(*format);
		// this method is not available in the macports version of assimp
		if (aiGetImporterDesc(f.c_str()) == nullptr) {
			Log::warn("Could not find an importer for %s", *format);
			state = false;
		}
	}
	return state;
}

void MeshPool::shutdown() {
	_meshes.clear();
}

std::string MeshPool::getName(const std::string& id) const {
	const io::FilesystemPtr& filesystem = core::App::getInstance()->filesystem();
	for (const char **format = supportedFormats; *format != nullptr; format++) {
		const std::string name = "mesh/" + id + "." + *format;
		if (filesystem->open(name)->exists()) {
			return name;
		}
	}

	return id;
}

MeshPtr MeshPool::getMesh(const std::string& id) {
	const std::string name = getName(id);
	auto i = _meshes.find(name);
	if (i != _meshes.end()) {
		return i->second;
	}

	const MeshPtr& mesh = std::make_shared<Mesh>();
	core::App::getInstance()->threadPool().enqueue([=]() {mesh->loadMesh(name);});
	_meshes[name] = mesh;
	return mesh;
}

}
