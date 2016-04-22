#include <gtest/gtest.h>

#include "core/App.h"
#include "core/EventBus.h"
#include "io/Filesystem.h"

namespace core {

class AbstractTest: public testing::Test {
private:
	class TestApp: public core::App {
	public:
		TestApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus) :
				core::App(filesystem, eventBus, 10000) {
			init("engine", "test");
			while (_curState < AppState::Running) {
				core_trace_scoped("AppMainLoop");
				onFrame();
			}
		}

		~TestApp() {
			while (AppState::InvalidAppState != _curState) {
				core_trace_scoped("AppMainLoop");
				onFrame();
			}
		}
	};

	TestApp *_testApp;

public:
	void SetUp() override;

	void TearDown() override;
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
