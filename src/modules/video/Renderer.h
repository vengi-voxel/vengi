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

/**
 * @brief Query the current cached line width.
 *
 * Returns the value last set via @c lineWidth(). This is a cached value and
 * may not reflect driver-clamped values for deprecated features.
 *
 * @return Current (requested) line width.
 */
float currentLineWidth();

/**
 * @brief Set the GL clear color.
 *
 * Updates the clear color used by @c clear(ClearFlag::Color). Returns @c true
 * when the value changed and the backend issued the underlying GL call.
 *
 * @param clearColor RGBA clear color.
 * @return @c true if the clear color was changed, @c false if it was already set.
 */
bool clearColor(const glm::vec4 &clearColor);

/**
 * @brief Get the currently configured clear color.
 *
 * @return Reference to the cached RGBA clear color.
 */
const glm::vec4 &currentClearColor();

/**
 * @brief Set the drawing viewport in window coordinates.
 *
 * Updates internal cached viewport and issues the backend call to change
 * the mapping from normalized device coordinates to window coordinates.
 *
 * @param x Left (lower-left) x coordinate of the viewport.
 * @param y Left (lower-left) y coordinate of the viewport.
 * @param w Width of the viewport in pixels.
 * @param h Height of the viewport in pixels.
 * @return @c true if the viewport changed, @c false if the values were unchanged.
 */
bool viewport(int x, int y, int w, int h);

/**
 * @brief Retrieve the currently set scissor rectangle (logical coordinates).
 *
 * Values are the last ones passed to @c scissor().
 *
 * @param[out] x Lower-left x coordinate.
 * @param[out] y Lower-left y coordinate.
 * @param[out] w Width in pixels.
 * @param[out] h Height in pixels.
 */
void getScissor(int &x, int &y, int &w, int &h);

/**
 * @brief Retrieve the currently set viewport rectangle (logical coordinates).
 *
 * @param[out] x Lower-left x coordinate.
 * @param[out] y Lower-left y coordinate.
 * @param[out] w Width in pixels.
 * @param[out] h Height in pixels.
 */
void getViewport(int &x, int &y, int &w, int &h);

/**
 * @brief Given in screen coordinates
 * @note viewport() must have been called before
 */
bool scissor(int x, int y, int w, int h);

/**
 * @brief Enable the given renderer state.
 *
 * Enables a backend state such as depth test, scissor test, blending, etc.
 * Returns the previous boolean value of the state (true if it was already
 * enabled).
 *
 * @param state The State to enable.
 * @return @c true if the state was already enabled, @c false if it was toggled.
 */
bool enable(State state);

/**
 * @brief Query whether a renderer state is currently enabled.
 *
 * @param state The State to query.
 * @return @c true if enabled, @c false otherwise.
 */
bool currentState(State state);

/**
 * @brief Disable the given renderer state.
 *
 * Returns the previous state value (true if it was enabled before the call).
 *
 * @param state The State to disable.
 * @return @c true if the state was previously enabled, @c false otherwise.
 */
bool disable(State state);

/**
 * @brief Set the color write mask for the framebuffer.
 *
 * Controls whether red, green, blue and alpha channels are writable.
 */
void colorMask(bool red, bool green, bool blue, bool alpha);

/**
 * @brief Set the polygon face culling mode.
 *
 * Sets which face is considered front/back for culling. Passing
 * @c Face::Max is a no-op.
 *
 * @param face The face to cull.
 * @return @c true if the cull face was changed, @c false if unchanged or invalid.
 */
bool cullFace(Face face);

/**
 * @brief Get the currently configured cull face.
 *
 * @return Currently selected Face value.
 */
Face currentCullFace();

/**
 * @brief Set the depth comparison function.
 *
 * @param func The depth comparison to use (e.g. Less, Lequal).
 * @return @c true if the function changed, @c false if unchanged.
 */
bool depthFunc(CompareFunc func);

/**
 * @brief Get the current depth comparison function.
 *
 * @return Current CompareFunc value.
 */
CompareFunc getDepthFunc();

/**
 * @brief Retrieve the current blend state settings.
 *
 * Fills @p enabled, @p src, @p dest and @p func with the cached blend state
 * from the renderer.
 */
void getBlendState(bool &enabled, BlendMode &src, BlendMode &dest, BlendEquation &func);

/**
 * @brief Set RGB and alpha blending factors (single-mode version).
 *
 * Updates the cached blend factors and issues the backend blend call.
 *
 * @return @c true if the blend factors were changed, @c false if unchanged.
 */
bool blendFunc(BlendMode src, BlendMode dest);

/**
 * @brief Set separate blending factors for RGB and alpha channels.
 *
 * @return @c true if the blend factors were changed, @c false if unchanged.
 */
bool blendFuncSeparate(BlendMode srcRGB, BlendMode destRGB, BlendMode srcAlpha, BlendMode destAlpha);

/**
 * @brief Set the blend equation used when blending is enabled.
 *
 * @return @c true if the equation changed, @c false if unchanged.
 */
bool blendEquation(BlendEquation func);

/**
 * @brief Set polygon rasterization mode for the given face (fill/line/point).
 *
 * Returns the previous polygon mode. Note: polygon modes are not available on
 * OpenGL ES; on GLES backends this call may be a no-op.
 */
PolygonMode polygonMode(Face face, PolygonMode mode);

/**
 * @brief Set polygon offset parameters used to add a constant depth offset.
 *
 * @param offset X is factor, Y is units as commonly used by glPolygonOffset.
 * @return @c true if the offset changed, @c false if unchanged.
 */
bool polygonOffset(const glm::vec2 &offset);

/**
 * @brief Set the point size for rendering GL points.
 *
 * @param size New point size in pixels.
 * @return @c true if the value changed, @c false if unchanged.
 */
bool pointSize(float size);

/**
 * @brief Get the texture bound to the given texture unit.
 *
 * @param unit Texture unit to query.
 * @return Id of the currently bound texture or InvalidId.
 */
Id currentTexture(TextureUnit unit);

} // namespace video
