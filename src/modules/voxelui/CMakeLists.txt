set(LIB voxelui)
set(SRCS
	FileDialogOptions.h FileDialogOptions.cpp
	LUAApiWidget.h LUAApiWidget.cpp
	LUAScriptParameters.h LUAScriptParameters.cpp
	ScriptApi.h ScriptApi.cpp
	ScriptBrowserPanel.h ScriptBrowserPanel.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxelformat voxelgenerator ui http json)

set(TEST_SRCS
	tests/ScriptApiTest.cpp
)
gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
