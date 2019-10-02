/**
 * @file
 */

#include "MeshPool.h"
#include "io/Filesystem.h"
#include "core/App.h"
#include "core/Log.h"
#include <assimp/scene.h>
#include <assimp/importerdesc.h>
#include <array>

namespace video {

static const char* supportedFormats[] = { "ogex", "obj", "dae", "fbx", "DAE", "FBX", "md5mesh", nullptr };

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

std::string MeshPool::getName(std::string_view id) const {
	const io::FilesystemPtr& filesystem = core::App::getInstance()->filesystem();
	const std::array<std::string, 2> directories {".", "mesh"};
	for (const auto& dir : directories) {
		for (const char **format = supportedFormats; *format != nullptr; format++) {
			const std::string& name = core::string::format("%s/%s.%s", dir.c_str(), id.data(), *format);
			if (filesystem->exists(name)) {
				return name;
			}
		}
	}

	return std::string(id);
}

MeshPtr MeshPool::getMesh(std::string_view id, bool async) {
	const std::string& name = getName(id);
	auto i = _meshes.find(name);
	if (i != _meshes.end()) {
		return i->second;
	}

	const MeshPtr& mesh = std::make_shared<Mesh>();
	if (async) {
		core::App::getInstance()->threadPool().enqueue([=]() {mesh->loadMesh(name);});
	} else {
		mesh->loadMesh(name);
	}
	_meshes[name] = mesh;
	return mesh;
}

}
