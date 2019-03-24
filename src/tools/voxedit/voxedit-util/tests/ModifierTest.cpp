/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "../Modifier.h"

namespace voxedit {

class ModifierTest: public core::AbstractTest {
private:
	using Super = core::AbstractTest;
protected:
	Modifier _modifier;

public:
	void SetUp() override {
		Super::SetUp();
		ASSERT_TRUE(_modifier.init()) << "Initialization failed";
	}

	void TearDown() override {
		_modifier.shutdown();
		Super::TearDown();
	}
};

TEST_F(ModifierTest, testFoo) {
}

}
