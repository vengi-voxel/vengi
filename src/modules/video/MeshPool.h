/**
 * @file
 */

#pragma once

#include "Mesh.h"
#include <unordered_map>
#include <string>

namespace video {

class MeshPool {
private:
	typedef std::unordered_map<std::string, MeshPtr> Meshes;
	Meshes _meshes;

	std::string getName(const std::string& id) const;
public:
	MeshPool();
	~MeshPool();

	MeshPtr getMesh(const std::string& name);
};

typedef std::shared_ptr<MeshPool> MeshPoolPtr;

}
