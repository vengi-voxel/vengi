/**
 * @file
 */

#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "core/ConsoleApp.h"
#include "core/EventBus.h"
#include "core/io/Filesystem.h"
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <vector>

namespace core {

template<typename T, glm::qualifier P = glm::defaultp>
inline ::std::string& operator+= (::std::string& in, const glm::tvec1<T, P>& vec) {
	in.append(glm::to_string(vec));
	return in;
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::std::string& operator+= (::std::string& in, const glm::tvec2<T, P>& vec) {
	in.append(glm::to_string(vec));
	return in;
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::std::string& operator+= (::std::string& in, const glm::tvec3<T, P>& vec) {
	in.append(glm::to_string(vec));
	return in;
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::std::string& operator+= (::std::string& in, const glm::tvec4<T, P>& vec) {
	in.append(glm::to_string(vec));
	return in;
}

inline ::std::string& operator+= (::std::string& in, const glm::mat4& mat) {
	in.append(glm::to_string(mat));
	return in;
}

inline ::std::string& operator+= (::std::string& in, const glm::mat3& mat) {
	in.append(glm::to_string(mat));
	return in;
}

inline ::std::ostream& operator<<(::std::ostream& os, const glm::mat4& mat) {
	return os << "mat4x4[" << glm::to_string(mat) << "]";
}

inline ::std::ostream& operator<<(::std::ostream& os, const glm::mat3& mat) {
	return os << "mat3x3[" << glm::to_string(mat) << "]";
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::std::ostream& operator<<(::std::ostream& os, const glm::tvec4<T, P>& vec) {
	return os << "vec4[" << glm::to_string(vec) << "]";
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::std::ostream& operator<<(::std::ostream& os, const glm::tvec3<T, P>& vec) {
	return os << "vec3[" << glm::to_string(vec) << "]";
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::std::ostream& operator<<(::std::ostream& os, const glm::tvec2<T, P>& vec) {
	return os << "vec2[" << glm::to_string(vec) << "]";
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::std::ostream& operator<<(::std::ostream& os, const glm::tvec1<T, P>& vec) {
	return os << "vec1[" << glm::to_string(vec) << "]";
}

class AbstractTest: public testing::Test {
private:
	class TestApp: public core::ConsoleApp {
		friend class AbstractTest;
	private:
		using Super = core::ConsoleApp;
	protected:
		AbstractTest* _test = nullptr;
	public:
		TestApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, AbstractTest* test);
		~TestApp();

		AppState onInit() override;
		AppState onCleanup() override;
	};

protected:
	TestApp *_testApp = nullptr;

	std::string toString(const std::string& filename) const;

	template<class T>
	std::string toString(const std::vector<T>& v) const {
		std::string str;
		str.reserve(4096);
		for (auto i = v.begin(); i != v.end();) {
			str += "'";
			str += *i;
			str += "'";
			if (++i != v.end()) {
				str += ", ";
			}
		}
		return str;
	}

	virtual void onCleanupApp() {
	}

	virtual bool onInitApp() {
		return true;
	}

public:
	virtual void SetUp() override;

	virtual void TearDown() override;
};

}

inline std::ostream& operator<<(std::ostream& stream, const glm::ivec2& v) {
	stream << "(x: " << v.x << ", y: " << v.y << ")";
	return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const glm::vec2& v) {
	stream << "(x: " << v.x << ", y: " << v.y << ")";
	return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const glm::ivec3& v) {
	stream << "(x: " << v.x << ", y: " << v.y << ", z: " << v.z << ")";
	return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const glm::vec3& v) {
	stream << "(x: " << v.x << ", y: " << v.y << ", z: " << v.z << ")";
	return stream;
}
