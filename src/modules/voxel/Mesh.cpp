/**
 * @file
 */

#include "Mesh.h"
#include "core/Algorithm.h"
#include "core/Assert.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/Trace.h"
#include "meshoptimizer.h"
#include "util/BufferUtil.h"
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/vector_relational.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/norm.hpp>

namespace voxel {

Mesh::Mesh(int vertices, int indices, bool mayGetResized)
	: _mayGetResized(mayGetResized), _initialVertices(vertices), _initialIndices(indices) {
	if (vertices > 0) {
		_vecVertices.reserve(vertices);
	}
	if (indices > 0) {
		_vecIndices.reserve(indices);
	}
}

Mesh::Mesh(Mesh &&other) noexcept {
	_vecIndices = core::move(other._vecIndices);
	_normals = core::move(other._normals);
	_vecVertices = core::move(other._vecVertices);
	_compressedIndices = other._compressedIndices;
	other._compressedIndices = nullptr;
	_compressedIndexSize = other._compressedIndexSize;
	other._compressedIndexSize = 0u;
	_offset = other._offset;
	_mayGetResized = other._mayGetResized;
}

Mesh::Mesh(const Mesh &other) {
	_vecIndices = other._vecIndices;
	_normals = other._normals;
	_vecVertices = other._vecVertices;
	_compressedIndexSize = other._compressedIndexSize;
	if (other._compressedIndices != nullptr) {
		_compressedIndices = (uint8_t *)core_malloc(_vecIndices.size() * _compressedIndexSize);
		core_memcpy(_compressedIndices, other._compressedIndices, _vecIndices.size() * _compressedIndexSize);
	} else {
		_compressedIndices = nullptr;
	}
	_offset = other._offset;
	_mayGetResized = other._mayGetResized;
}

Mesh &Mesh::operator=(const Mesh &other) {
	if (&other == this) {
		return *this;
	}
	_vecIndices = other._vecIndices;
	_normals = other._normals;
	_vecVertices = other._vecVertices;
	_compressedIndexSize = other._compressedIndexSize;
	core_free(_compressedIndices);
	if (other._compressedIndices != nullptr) {
		_compressedIndices = (uint8_t *)core_malloc(_vecIndices.size() * _compressedIndexSize);
		core_memcpy(_compressedIndices, other._compressedIndices, _vecIndices.size() * _compressedIndexSize);
	} else {
		_compressedIndices = nullptr;
	}
	_offset = other._offset;
	_mayGetResized = other._mayGetResized;
	return *this;
}

Mesh &Mesh::operator=(Mesh &&other) noexcept {
	_vecIndices = core::move(other._vecIndices);
	_normals = core::move(other._normals);
	_vecVertices = core::move(other._vecVertices);
	core_free(_compressedIndices);
	_compressedIndices = other._compressedIndices;
	other._compressedIndices = nullptr;
	_compressedIndexSize = other._compressedIndexSize;
	other._compressedIndexSize = 4u;
	_offset = other._offset;
	_mayGetResized = other._mayGetResized;
	return *this;
}

Mesh::~Mesh() {
	if (_compressedIndices != nullptr) {
		core_free(_compressedIndices);
	}
}

const NormalArray &Mesh::getNormalVector() const {
	return _normals;
}

const IndexArray &Mesh::getIndexVector() const {
	return _vecIndices;
}

const VertexArray &Mesh::getVertexVector() const {
	return _vecVertices;
}

IndexArray &Mesh::getIndexVector() {
	return _vecIndices;
}

VertexArray &Mesh::getVertexVector() {
	return _vecVertices;
}

NormalArray &Mesh::getNormalVector() {
	return _normals;
}

size_t Mesh::getNoOfVertices() const {
	return _vecVertices.size();
}

const VoxelVertex &Mesh::getVertex(IndexType index) const {
	return _vecVertices[index];
}

const VoxelVertex *Mesh::getRawVertexData() const {
	return _vecVertices.data();
}

size_t Mesh::getNoOfIndices() const {
	return _vecIndices.size();
}

IndexType Mesh::getIndex(IndexType index) const {
	return _vecIndices[index];
}

const IndexType *Mesh::getRawIndexData() const {
	return _vecIndices.data();
}

const glm::ivec3 &Mesh::getOffset() const {
	return _offset;
}

void Mesh::setOffset(const glm::ivec3 &offset) {
	_offset = offset;
}

void Mesh::clear() {
	_vecVertices.clear();
	_vecIndices.clear();
	_offset = glm::ivec3(0);
}

bool Mesh::isEmpty() const {
	return getNoOfVertices() == 0 || getNoOfIndices() == 0;
}

void Mesh::addTriangle(IndexType index0, IndexType index1, IndexType index2) {
	// Make sure the specified indices correspond to valid vertices.
	core_assert_msg(index0 < _vecVertices.size(), "Index points at an invalid vertex (%i/%i).", (int)index0,
					(int)_vecVertices.size());
	core_assert_msg(index1 < _vecVertices.size(), "Index points at an invalid vertex (%i/%i).", (int)index1,
					(int)_vecVertices.size());
	core_assert_msg(index2 < _vecVertices.size(), "Index points at an invalid vertex (%i/%i).", (int)index2,
					(int)_vecVertices.size());
	if (!_mayGetResized) {
		core_assert_msg(
			_vecIndices.size() + 3 < _vecIndices.capacity(),
			"addTriangle() call exceeds the capacity of the indices vector and will trigger a realloc (%i vs %i)",
			(int)_vecIndices.size(), (int)_vecIndices.capacity());
	}

	if (_vecIndices.capacity() < _vecIndices.size() + 3) {
		_vecIndices.reserve(_vecIndices.capacity() + _initialIndices);
	}

	_vecIndices.push_back(index0);
	_vecIndices.push_back(index1);
	_vecIndices.push_back(index2);
}

IndexType Mesh::addVertex(const VoxelVertex &vertex) {
	// We should not add more vertices than our chosen index type will let us index.
	core_assert_msg(_vecVertices.size() < (std::numeric_limits<IndexType>::max)(),
					"Mesh has more vertices that the chosen index type allows.");
	if (!_mayGetResized) {
		core_assert_msg(
			_vecVertices.size() + 1 < _vecVertices.capacity(),
			"addVertex() call exceeds the capacity of the vertices vector and will trigger a realloc (%i vs %i)",
			(int)_vecVertices.size(), (int)_vecVertices.capacity());
	}

	if (_vecVertices.capacity() < _vecVertices.size() + 1) {
		_vecVertices.reserve(_vecVertices.capacity() + _initialVertices);
	}
	_vecVertices.push_back(vertex);
	return (IndexType)_vecVertices.size() - 1;
}

void Mesh::setNormal(IndexType index, const glm::vec3 &normal) {
	// We should not add more vertices than our chosen index type will let us index.
	core_assert_msg(_normals.size() < (std::numeric_limits<IndexType>::max)(),
					"Mesh has more normals that the chosen index type allows.");
	_normals.resize(_vecVertices.size());
	_normals[index] = normal;
}

void Mesh::removeUnusedVertices() {
	if (_vecVertices.empty()) {
		return;
	}
	if (_vecIndices.empty()) {
		return;
	}
	const size_t vertices = _vecVertices.size();
	const size_t indices = _vecIndices.size();
	core::Buffer<bool> isVertexUsed(vertices);
	isVertexUsed.fill(false);

	for (size_t triCt = 0u; triCt < indices; ++triCt) {
		IndexType v = _vecIndices[triCt];
		isVertexUsed[v] = true;
	}

	size_t noOfUsedVertices = 0;
	IndexArray newPos(vertices);
	for (size_t vertCt = 0u; vertCt < vertices; ++vertCt) {
		if (!isVertexUsed[vertCt]) {
			continue;
		}
		_vecVertices[noOfUsedVertices] = _vecVertices[vertCt];
		// here the assumption that a normal is added for each vertex if
		// at least one normal was added holds
		if (!_normals.empty()) {
			_normals[noOfUsedVertices] = _normals[vertCt];
		}
		newPos[vertCt] = noOfUsedVertices;
		++noOfUsedVertices;
	}

	_vecVertices.resize(noOfUsedVertices);

	for (size_t triCt = 0u; triCt < indices; ++triCt) {
		_vecIndices[triCt] = newPos[_vecIndices[triCt]];
	}
	_vecIndices.resize(indices);
}

void Mesh::compressIndices() {
	if (_vecIndices.empty()) {
		core_free(_compressedIndices);
		_compressedIndices = nullptr;
		_compressedIndexSize = 0;
		return;
	}
	const size_t maxSize = _vecIndices.size() * sizeof(voxel::IndexType);
	core_free(_compressedIndices);
	_compressedIndices = (uint8_t *)core_malloc(maxSize);
	util::indexCompress(&_vecIndices.front(), maxSize, _compressedIndexSize, _compressedIndices, maxSize);
}

void Mesh::calculateNormals() {
	if (!_normals.empty()) {
		return;
	}
	_normals.resize(_vecVertices.size());
	_normals.fill(glm::vec3(0.0f));

	for (size_t i = 0; i < _vecIndices.size(); i += 3) {
		IndexType index0 = _vecIndices[i + 0];
		IndexType index1 = _vecIndices[i + 1];
		IndexType index2 = _vecIndices[i + 2];

		const glm::vec3 &v0 = _vecVertices[index0].position;
		const glm::vec3 &v1 = _vecVertices[index1].position;
		const glm::vec3 &v2 = _vecVertices[index2].position;

		glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

		_normals[index0] += normal;
		_normals[index1] += normal;
		_normals[index2] += normal;
	}

	for (size_t i = 0; i < _normals.size(); ++i) {
		if (glm::length(_normals[i]) > 0.0f) {
			_normals[i] = glm::normalize(_normals[i]);
		}
	}
}

void Mesh::calculateBounds() {
	_mins = glm::vec3(std::numeric_limits<float>::max());
	_maxs = glm::vec3(std::numeric_limits<float>::min());
	for (const VoxelVertex &vertex : _vecVertices) {
		_mins = glm::min(_mins, vertex.position);
		_maxs = glm::max(_maxs, vertex.position);
	}
}

bool Mesh::operator<(const Mesh &rhs) const {
	return glm::all(glm::lessThan(getOffset(), rhs.getOffset()));
}

struct Triangle {
	const VoxelVertex *v0;
	const VoxelVertex *v1;
	const VoxelVertex *v2;
	voxel::IndexType *i0;
	voxel::IndexType *i1;
	voxel::IndexType *i2;
	voxel::IndexType vi0;
	voxel::IndexType vi1;
	voxel::IndexType vi2;

	inline glm::vec3 center() const {
		return (v0->position + v1->position + v2->position) / 3.0f;
	}

	Triangle(const VoxelVertex *_v0, const VoxelVertex *_v1, const VoxelVertex *_v2, voxel::IndexType *_i0,
			 voxel::IndexType *_i1, voxel::IndexType *_i2)
		: v0(_v0), v1(_v1), v2(_v2), i0(_i0), i1(_i1), i2(_i2) {
		if (_i0 != nullptr) {
			vi0 = *_i0;
			vi1 = *_i1;
			vi2 = *_i2;
		} else {
			vi0 = -1;
			vi1 = -1;
			vi2 = -1;
		}
	}

	Triangle(const Triangle &rhs)
		: v0(rhs.v0), v1(rhs.v1), v2(rhs.v2), i0(rhs.i0), i1(rhs.i1), i2(rhs.i2), vi0(rhs.vi0), vi1(rhs.vi1),
		  vi2(rhs.vi2) {
	}

	Triangle(Triangle &&rhs) noexcept
		: v0(rhs.v0), v1(rhs.v1), v2(rhs.v2), i0(rhs.i0), i1(rhs.i1), i2(rhs.i2), vi0(rhs.vi0), vi1(rhs.vi1),
		  vi2(rhs.vi2) {
	}

	Triangle &operator=(const Triangle &rhs) {
		if (this == &rhs) {
			return *this;
		}
		v0 = rhs.v0;
		v1 = rhs.v1;
		v2 = rhs.v2;
		vi0 = rhs.vi0;
		vi1 = rhs.vi1;
		vi2 = rhs.vi2;
		if (rhs.v0 != nullptr) {
			*i0 = vi0;
			*i1 = vi1;
			*i2 = vi2;
		} else {
			i0 = nullptr;
			i1 = nullptr;
			i2 = nullptr;
		}
		return *this;
	}

	Triangle &operator=(Triangle &&rhs) noexcept {
		if (this == &rhs) {
			return *this;
		}
		v0 = rhs.v0;
		v1 = rhs.v1;
		v2 = rhs.v2;
		vi0 = rhs.vi0;
		vi1 = rhs.vi1;
		vi2 = rhs.vi2;
		if (rhs.i0 != nullptr) {
			*i0 = vi0;
			*i1 = vi1;
			*i2 = vi2;
		} else {
			i0 = nullptr;
			i1 = nullptr;
			i2 = nullptr;
		}
		return *this;
	}
};

struct TriangleView {
	TriangleView(const VertexArray &vecVertices, IndexArray &vecIndices)
		: _vecVertices(vecVertices), _vecIndices(vecIndices) {
	}
	const VertexArray &_vecVertices;
	IndexArray &_vecIndices;

	int size() const {
		return (int)_vecIndices.size() / 3;
	}

	class iterator {
	private:
		Triangle _tri{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
		const VertexArray *_vecVertices;
		IndexArray *_vecIndices;
		int _idx;

		void updateTri() {
			if (_idx < 0 || _idx >= (int)_vecIndices->size() / 3) {
				_tri = Triangle(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
				return;
			}
			_tri.i0 = &(*_vecIndices)[(_idx * 3) + 0];
			_tri.i1 = &(*_vecIndices)[(_idx * 3) + 1];
			_tri.i2 = &(*_vecIndices)[(_idx * 3) + 2];
			_tri.v0 = &(*_vecVertices)[*_tri.i0];
			_tri.v1 = &(*_vecVertices)[*_tri.i1];
			_tri.v2 = &(*_vecVertices)[*_tri.i2];
			_tri.vi0 = *_tri.i0;
			_tri.vi1 = *_tri.i1;
			_tri.vi2 = *_tri.i2;
		}

	public:
		iterator(const VertexArray *vecVertices, IndexArray *vecIndices, int idx)
			: _vecVertices(vecVertices), _vecIndices(vecIndices), _idx(idx) {
			updateTri();
		}

		iterator(iterator &&i) noexcept : _vecVertices(i._vecVertices), _vecIndices(i._vecIndices), _idx(i._idx) {
			i._vecVertices = nullptr;
			i._vecIndices = nullptr;
			i._idx = -1;
			i.updateTri();
			updateTri();
		}

		iterator(const iterator &i) : _vecVertices(i._vecVertices), _vecIndices(i._vecIndices), _idx(i._idx) {
			updateTri();
		}

		iterator operator=(const iterator &i) {
			if (this == &i) {
				return *this;
			}
			_idx = i._idx;
			updateTri();
			return *this;
		}

		iterator operator=(iterator &&i) noexcept {
			if (this == &i) {
				return *this;
			}
			_idx = i._idx;
			i._vecVertices = nullptr;
			i._vecIndices = nullptr;
			i._idx = -1;
			i.updateTri();
			updateTri();
			return *this;
		}

		Triangle &operator*() {
			return _tri;
		}

		const Triangle &operator*() const {
			return _tri;
		}

		bool operator==(const iterator &rhs) const {
			return _idx == rhs._idx;
		}

		bool operator!=(const iterator &rhs) const {
			return _idx != rhs._idx;
		}

		iterator &operator--() {
			--_idx;
			updateTri();
			return *this;
		}

		iterator &operator++() {
			++_idx;
			updateTri();
			return *this;
		}
	};

	iterator begin() {
		return iterator(&_vecVertices, &_vecIndices, 0);
	}

	iterator end() {
		return iterator(&_vecVertices, &_vecIndices, size());
	}
};

bool Mesh::sort(const glm::vec3 &cameraPos) {
	if (glm::all(glm::epsilonEqual(cameraPos, _lastCameraPos, glm::vec3(0.5f)))) {
		return false;
	}
	_lastCameraPos = cameraPos;
	core_trace_scoped(MeshSort);
	TriangleView triView(_vecVertices, _vecIndices);
	core::sort(triView.begin(), triView.end(), [&cameraPos](const Triangle &lhs, const Triangle &rhs) {
		return glm::distance2(lhs.center(), cameraPos) < glm::distance2(rhs.center(), cameraPos);
	});
	return true;
}

void Mesh::optimize() {
	if (isEmpty()) {
		return;
	}
	core_trace_scoped(MeshOptimize);
	meshopt_optimizeVertexCache(_vecIndices.data(), _vecIndices.data(), _vecIndices.size(), _vecVertices.size());
	meshopt_optimizeOverdraw(_vecIndices.data(), _vecIndices.data(), _vecIndices.size(), &_vecVertices.data()->position.x, _vecVertices.size(), sizeof(VoxelVertex), 1.05f);
	meshopt_optimizeVertexFetch(_vecVertices.data(), _vecIndices.data(), _vecIndices.size(), _vecVertices.data(), _vecVertices.size(), sizeof(VoxelVertex));
	const IndexArray oldIndices(_vecIndices);
	const size_t newSize =
		meshopt_simplify(_vecIndices.data(), oldIndices.data(), oldIndices.size(), &_vecVertices.data()->position.x,
						 _vecVertices.size(), sizeof(VoxelVertex), oldIndices.size() / 2, 0.1f, 0, nullptr);
	Log::debug("newSize: %i, oldsize: %i", (int)newSize, (int)oldIndices.size());
	_vecIndices.resize(newSize);
}

} // namespace voxel
