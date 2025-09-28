/**
 * @file
 * @defgroup Video Video
 * @{
 * The video subsystem implements rendering and window management.
 * @}
 */

#pragma once

#include "RendererInterface.h"
#include <type_traits>

namespace video {

namespace _priv {

template <typename DATATYPE>
struct to_type {
	typedef typename core::remove_reference<typename core::remove_pointer<DATATYPE>::type>::type type;
};

} // namespace _priv

DataType mapIndexTypeBySize(size_t size);

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702) // unreachable code
#endif

/**
 * @brief Maps data types to GL enums
 */
template <typename DATATYPE>
constexpr inline DataType mapType() {
	static_assert(std::is_fundamental<typename _priv::to_type<DATATYPE>::type>::value,
				  "Given datatype is not fundamental");

	constexpr size_t size = sizeof(typename _priv::to_type<DATATYPE>::type);
	static_assert(size != 8u, "Long types are not supported");
	static_assert(size != 12u, "Invalid data type given (size 12)");
	static_assert(size != 16u, "Invalid data type given (size 16)");
	static_assert(size != 36u, "Invalid data type given (size 36)");
	static_assert(size != 64u, "Invalid data type given (size 64)");
	static_assert(size == 1u || size == 2u || size == 4u, "Only datatypes of size 1, 2 or 4 are supported");
	if (std::is_floating_point<typename _priv::to_type<DATATYPE>::type>()) {
		// if constexpr (size == 4u) {
		return DataType::Float;
		// }
		// return DataType::Double;
	}

	constexpr bool isUnsigned = std::is_unsigned<typename _priv::to_type<DATATYPE>::type>();
	if constexpr (size == 1u) {
		if constexpr (isUnsigned) {
			return DataType::UnsignedByte;
		}
		return DataType::Byte;
	}
	if constexpr (size == 2u) {
		if constexpr (isUnsigned) {
			return DataType::UnsignedShort;
		}
		return DataType::Short;
	}

	static_assert(size <= 4u, "No match found");
	if constexpr (isUnsigned) {
		return DataType::UnsignedInt;
	}
	return DataType::Int;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

template <> constexpr inline DataType mapType<glm::ivec2>() {
	return mapType<typename glm::ivec2::value_type>();
}

template <> constexpr inline DataType mapType<glm::vec2>() {
	return mapType<typename glm::vec2::value_type>();
}

template <> constexpr inline DataType mapType<glm::ivec3>() {
	return mapType<typename glm::ivec3::value_type>();
}

template <> constexpr inline DataType mapType<glm::vec3>() {
	return mapType<typename glm::vec3::value_type>();
}

template <> constexpr inline DataType mapType<glm::ivec4>() {
	return mapType<typename glm::ivec4::value_type>();
}

template <> constexpr inline DataType mapType<glm::vec4>() {
	return mapType<typename glm::vec4::value_type>();
}

struct RenderState {
	int limits[core::enumVal(video::Limit::Max)] = {};
	inline int limiti(video::Limit limit) const {
		return limits[core::enumVal(limit)];
	}
	float flimits[core::enumVal(video::Limit::Max)] = {};
	inline float limit(video::Limit limit) const {
		return flimits[core::enumVal(limit)];
	}
	double specs[core::enumVal(video::Spec::Max)] = {};
	inline int specificationi(video::Spec spec) const {
		return (int)(specification(spec) + 0.5);
	}

	inline double specification(video::Spec spec) const {
		return specs[core::enumVal(spec)];
	}

	bool features[core::enumVal(video::Feature::Max)] = {};
	inline bool supports(video::Feature feature) const {
		return features[core::enumVal(feature)];
	}
};

RenderState &renderState();

void construct();

template <class IndexType> inline void drawElements(Primitive mode, size_t numIndices, void *offset = nullptr) {
	drawElements(mode, numIndices, mapType<IndexType>(), offset);
}

inline void drawElements(Primitive mode, size_t numIndices, size_t indexSize, void *offset = nullptr) {
	drawElements(mode, numIndices, mapIndexTypeBySize(indexSize), offset);
}

inline bool hasFeature(Feature feature) {
	return renderState().supports(feature);
}

bool useFeature(Feature feature);
void disableDebug();
void deleteFramebuffer(Id &id);
void deleteTexture(Id &id);
void deleteRenderbuffer(Id &id);
void deleteBuffer(Id &id);
Id genBuffer();
Id genTexture(const TextureConfig &cfg);
Id genVertexArray();
Id genRenderbuffer();
Id genFramebuffer();

IdPtr genFenc();
/**
 * @return @c true if the given fence was signaled in the given timeout
 */
bool checkFence(IdPtr id, uint64_t timeout);
void deleteFence(IdPtr& id);

inline int limiti(Limit l) {
	return renderState().limiti(l);
}

inline float limit(Limit l) {
	return renderState().limit(l);
}

inline int specificationi(Spec l) {
	return renderState().specificationi(l);
}

inline double specification(Spec l) {
	return renderState().specification(l);
}

/**
 * @brief Checks whether a given hardware limit is exceeded.
 */
bool checkLimit(int amount, Limit l);

} // namespace video
