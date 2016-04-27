#include "TestShared.h"
#include <stdlib.h>

class LocalEnv: public ::testing::Environment {
public:
	virtual ~LocalEnv() {
	}
	virtual void SetUp() override {
	}
	virtual void TearDown() override {
	}
};

extern "C" int main (int argc, char **argv)
{
	::testing::AddGlobalTestEnvironment(new LocalEnv);
	//::testing::GTEST_FLAG(throw_on_failure) = true;
	::testing::InitGoogleTest(&argc, argv);

#if AI_EXCEPTIONS
	try {
#endif
		return RUN_ALL_TESTS();

#if AI_EXCEPTIONS
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
#endif
}
