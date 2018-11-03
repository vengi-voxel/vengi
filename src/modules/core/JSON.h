/**
 * @file
 */

#pragma once

#include "json.hpp"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace core {

using json = ::nlohmann::json;

}

namespace glm {
template<typename T, qualifier Q = defaultp>
void to_json(::core::json& j, const vec<2, T, Q>& p) {
	j = ::core::json{{"x", p.x}, {"y", p.y}};
}

template<typename T, qualifier Q = defaultp>
void from_json(const ::core::json& j, vec<2, T, Q>& p) {
	if (j.is_array() && j.size() == 2) {
		p.x = j[0].get<T>();
		p.y = j[1].get<T>();
		return;
	}
	p.x = j.at("x").get<T>();
	p.y = j.at("y").get<T>();
}

template<typename T, qualifier Q = defaultp>
void to_json(::core::json& j, const vec<3, T, Q>& p) {
	j = ::core::json{{"x", p.x}, {"y", p.y}, {"z", p.z}};
}

template<typename T, qualifier Q = defaultp>
void from_json(const ::core::json& j, vec<3, T, Q>& p) {
	if (j.is_array() && j.size() == 3) {
		p.x = j[0].get<T>();
		p.y = j[1].get<T>();
		p.z = j[2].get<T>();
		return;
	}
	p.x = j.at("x").get<T>();
	p.y = j.at("y").get<T>();
	p.z = j.at("z").get<T>();
}

template<typename T, qualifier Q = defaultp>
void to_json(::core::json& j, const vec<4, T, Q>& p) {
	j = ::core::json{{"x", p.x}, {"y", p.y}, {"z", p.z}, {"w", p.w}};
}

template<typename T, qualifier Q = defaultp>
void from_json(const ::core::json& j, vec<4, T, Q>& p) {
	if (j.is_array() && j.size() == 4) {
		p.x = j[0].get<T>();
		p.y = j[1].get<T>();
		p.z = j[2].get<T>();
		p.w = j[3].get<T>();
		return;
	}
	p.x = j.at("x").get<T>();
	p.y = j.at("y").get<T>();
	p.z = j.at("z").get<T>();
	p.z = j.at("w").get<T>();
}

}
