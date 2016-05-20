/**
 * @file
 */

#include "MeshPool.h"
#include "io/Filesystem.h"
#include "core/App.h"

namespace video {

static const char* supportedFormats[] = { "dae", "fbx", nullptr };

MeshPool::MeshPool() {
}

MeshPool::~MeshPool() {
}

void MeshPool::shutdown() {
	_meshes.clear();
}

std::string MeshPool::getName(const std::string& id) const {
	for (const char **format = supportedFormats; *format != nullptr; format++) {
		const std::string name = "mesh/" + id + "." + *format;
		if (core::App::getInstance()->filesystem()->open(name)->exists())
			return name;
	}

	return id;
}

MeshPtr MeshPool::getMesh(const std::string& id) {
	const std::string name = getName(id);
	auto i = _meshes.find(name);
	if (i != _meshes.end())
		return i->second;

	const MeshPtr mesh = std::make_shared<Mesh>();
	core::App::getInstance()->threadPool().enqueue([=]() {mesh->loadMesh(name);});
	_meshes[name] = mesh;
	return mesh;
}

}
