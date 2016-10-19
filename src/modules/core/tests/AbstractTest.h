/**
 * @file
 */

#include <gtest/gtest.h>

#include "core/App.h"
#include "core/EventBus.h"
#include "io/Filesystem.h"

namespace core {

class AbstractTest: public testing::Test {
private:
	class TestApp: public core::App {
		friend class AbstractTest;
	protected:
		AbstractTest* _test = nullptr;
	public:
		TestApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, AbstractTest* test);
		~TestApp();

		virtual AppState onInit() override;
		virtual AppState onCleanup() override;
	};

protected:
	TestApp *_testApp;

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
