/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/App.h"
#include "core/EventBus.h"
#include "io/Filesystem.h"
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <vector>

namespace core {

inline ::std::ostream& operator<<(::std::ostream& os, const glm::mat4& mat) {
	return os << "mat4x4[" << glm::to_string(mat) << "]";
}

inline ::std::ostream& operator<<(::std::ostream& os, const glm::mat3& mat) {
	return os << "mat3x3[" << glm::to_string(mat) << "]";
}

template<typename T>
inline ::std::ostream& operator<<(::std::ostream& os, const glm::tvec4<T>& vec) {
	return os << "vec4[" << glm::to_string(vec) << "]";
}

template<typename T>
inline ::std::ostream& operator<<(::std::ostream& os, const glm::tvec3<T>& vec) {
	return os << "vec3[" << glm::to_string(vec) << "]";
}

template<typename T>
inline ::std::ostream& operator<<(::std::ostream& os, const glm::tvec2<T>& vec) {
	return os << "vec2[" << glm::to_string(vec) << "]";
}

template<typename T>
inline ::std::ostream& operator<<(::std::ostream& os, const glm::tvec1<T>& vec) {
	return os << "vec1[" << glm::to_string(vec) << "]";
}

class AbstractTest: public testing::Test {
private:
	class TestApp: public core::App {
		friend class AbstractTest;
	protected:
		AbstractTest* _test = nullptr;
	public:
		TestApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, AbstractTest* test);
		~TestApp();

		virtual AppState onInit() override;
		virtual AppState onCleanup() override;
	};

protected:
	TestApp *_testApp;

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
