/**
 * @file
 */

#pragma once

#include "Mesh.h"
#include "core/String.h"
#include <unordered_map>
#include <memory>

namespace video {

/**
 * @ingroup Video
 */
class MeshPool {
private:
	typedef std::unordered_map<std::string, MeshPtr> Meshes;
	Meshes _meshes;

	std::string getName(std::string_view id) const;
public:
	MeshPool();
	~MeshPool();

	bool init();
	void shutdown();

	MeshPtr getMesh(std::string_view name, bool async = true);
};

typedef std::shared_ptr<MeshPool> MeshPoolPtr;

}
