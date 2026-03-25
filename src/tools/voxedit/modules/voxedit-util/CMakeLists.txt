set(SRCS
	modifier/brush/AABBBrush.cpp modifier/brush/AABBBrush.h
	modifier/brush/BrushGizmo.h
	modifier/brush/Brush.cpp modifier/brush/Brush.h
	modifier/brush/LineBrush.cpp modifier/brush/LineBrush.h
	modifier/brush/NormalBrush.cpp modifier/brush/NormalBrush.h
	modifier/brush/PaintBrush.cpp modifier/brush/PaintBrush.h
	modifier/brush/PathBrush.cpp modifier/brush/PathBrush.h
	modifier/brush/PlaneBrush.cpp modifier/brush/PlaneBrush.h
	modifier/brush/ShapeBrush.cpp modifier/brush/ShapeBrush.h
	modifier/brush/StampBrush.cpp modifier/brush/StampBrush.h
	modifier/brush/TextBrush.cpp modifier/brush/TextBrush.h
	modifier/brush/SelectBrush.cpp modifier/brush/SelectBrush.h
	modifier/brush/ExtrudeBrush.cpp modifier/brush/ExtrudeBrush.h
	modifier/brush/TransformBrush.cpp modifier/brush/TransformBrush.h
	modifier/brush/RulerBrush.cpp modifier/brush/RulerBrush.h
	modifier/brush/SculptBrush.cpp modifier/brush/SculptBrush.h
	modifier/brush/TextureBrush.cpp modifier/brush/TextureBrush.h

	modifier/IModifierRenderer.h
	modifier/Modifier.h modifier/Modifier.cpp
	modifier/ModifierButton.h modifier/ModifierButton.cpp
	modifier/ModifierRenderer.h modifier/ModifierRenderer.cpp
	modifier/ModifierType.h
	modifier/ModifierVolumeWrapper.h
	modifier/ShapeType.h
	modifier/SceneModifiedFlags.h

	network/Client.h network/Client.cpp
	network/Server.h network/Server.cpp
	network/ClientNetwork.h network/ClientNetwork.cpp
	network/ServerNetwork.h network/ServerNetwork.cpp
	network/ProtocolIds.h
	network/ProtocolMessageFactory.h network/ProtocolMessageFactory.cpp
	network/SessionRecorder.h network/SessionRecorder.cpp
	network/SessionPlayer.h network/SessionPlayer.cpp

	network/protocol/NodeAddedMessage.h
	network/protocol/ClientListMessage.h
	network/protocol/ClientInfo.h
	network/protocol/CommandMessage.h
	network/protocol/InitSessionMessage.h
	network/protocol/NodeKeyFramesMessage.h
	network/protocol/NodeMovedMessage.h
	network/protocol/NodePaletteChangedMessage.h
	network/protocol/NodeNormalPaletteChangedMessage.h
	network/protocol/NodePropertiesMessage.h
	network/protocol/NodeIKConstraintMessage.h
	network/protocol/NodeRemovedMessage.h
	network/protocol/NodeRenamedMessage.h
	network/protocol/PingMessage.h
	network/protocol/SceneGraphAnimationMessage.h
	network/protocol/SceneStateMessage.h network/protocol/SceneStateMessage.cpp
	network/protocol/SceneStateRequestMessage.h
	network/protocol/VoxelModificationMessage.h
	network/protocol/LuaScriptsRequestMessage.h
	network/protocol/LuaScriptsListMessage.h
	network/protocol/LuaScriptCreateMessage.h
	network/protocol/LuaScriptExecMessage.h
	network/protocol/LogMessage.h
	network/protocol/ChatMessage.h

	network/handler/client/VoxelModificationHandler.h network/handler/client/VoxelModificationHandler.cpp
	network/handler/client/NodeAddedHandler.h network/handler/client/NodeAddedHandler.cpp
	network/handler/client/NodeKeyFramesHandler.h network/handler/client/NodeKeyFramesHandler.cpp
	network/handler/client/NodeMovedHandler.h network/handler/client/NodeMovedHandler.cpp
	network/handler/client/NodePaletteChangedHandler.h network/handler/client/NodePaletteChangedHandler.cpp
	network/handler/client/NodeNormalPaletteChangedHandler.h network/handler/client/NodeNormalPaletteChangedHandler.cpp
	network/handler/client/NodePropertiesHandler.h network/handler/client/NodePropertiesHandler.cpp
	network/handler/client/NodeIKConstraintHandler.h network/handler/client/NodeIKConstraintHandler.cpp
	network/handler/client/NodeRemovedHandler.h network/handler/client/NodeRemovedHandler.cpp
	network/handler/client/NodeRenamedHandler.h network/handler/client/NodeRenamedHandler.cpp
	network/handler/client/SceneStateRequestHandler.h network/handler/client/SceneStateRequestHandler.cpp
	network/handler/client/SceneStateHandlerClient.h network/handler/client/SceneStateHandlerClient.cpp
	network/handler/client/SceneGraphAnimationHandler.h network/handler/client/SceneGraphAnimationHandler.cpp
	network/handler/client/LogMessageHandler.h network/handler/client/LogMessageHandler.cpp
	network/handler/client/ChatMessageHandler.h network/handler/client/ChatMessageHandler.cpp
	network/handler/client/ClientListHandler.h network/handler/client/ClientListHandler.cpp

	network/handler/server/CommandHandlerServer.h
	network/handler/server/InitSessionHandler.h network/handler/server/InitSessionHandler.cpp
	network/handler/server/LuaScriptCreateHandler.h network/handler/server/LuaScriptCreateHandler.cpp
	network/handler/server/LuaScriptExecHandler.h network/handler/server/LuaScriptExecHandler.cpp
	network/handler/server/LuaScriptsRequestHandler.h network/handler/server/LuaScriptsRequestHandler.cpp
	network/handler/server/SceneStateHandlerServer.h network/handler/server/SceneStateHandlerServer.cpp
	network/handler/server/BroadcastHandler.h network/handler/server/BroadcastHandler.cpp
	network/handler/server/ChatHandler.h network/handler/server/ChatHandler.cpp

	SceneManager.h SceneManager.cpp
	AxisUtil.h AxisUtil.cpp
	Config.h
	CommandCompleter.h

	LUAApiListener.h LUAApiListener.cpp

	ISceneRenderer.h
	SceneRenderer.h SceneRenderer.cpp

	ModelNodeSettings.h

	Clipboard.h Clipboard.cpp
)

set(LIB voxedit-util)
set(DEPENDENCIES ui voxelrender voxelformat voxelgenerator memento network sound)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${DEPENDENCIES})

set(TEST_SRCS
	tests/AbstractBrushTest.cpp tests/AbstractBrushTest.h
	tests/ClipboardTest.cpp
	tests/ExtrudeBrushTest.cpp
	tests/TransformBrushTest.cpp
	tests/SculptBrushTest.cpp
	tests/LineBrushTest.cpp
	tests/ModifierTest.cpp
	tests/ModifierVolumeWrapperTest.cpp
	tests/NormalBrushTest.cpp
	tests/PaintBrushTest.cpp
	tests/PathBrushTest.cpp
	tests/PlaneBrushTest.cpp
	tests/RulerBrushTest.cpp
	tests/ProtocolMessageFactoryTest.cpp
	tests/SceneManagerTest.cpp
	tests/SceneRendererTest.cpp
	tests/SelectBrushTest.cpp
	tests/ShapeBrushTest.cpp
	tests/StampBrushTest.cpp
	tests/TextBrushTest.cpp
	tests/TextureBrushTest.cpp
	tests/ClientTest.cpp
	tests/SessionRecorderTest.cpp
)
set(TEST_FILES
	shared/font.ttf
	voxedit/chr_knight.vengi
	tests/test-merge.vengi
	tests/test-merge2.vengi
	tests/test-palette-in.png
	tests/rgb_small.vox
)
gtest_suite_sources(tests
	${TEST_SRCS}
)
gtest_suite_deps(tests ${LIB} test-app)
gtest_suite_files(tests ${TEST_FILES})

# CMake Error: Xcode generator: Duplicate object ids not allowed
if (NOT CMAKE_GENERATOR STREQUAL "Xcode")
	gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
	gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
	gtest_suite_files(tests-${LIB} ${TEST_FILES})
	gtest_suite_deps(tests-${LIB} ${LIB} test-app)
	gtest_suite_end(tests-${LIB})
endif()

if (USE_BENCHMARKS)
	set(BENCHMARK_SRCS
		benchmarks/ModifierVolumeWrapperBenchmark.cpp
		benchmarks/SculptBrushBenchmark.cpp
		benchmarks/TransformBrushBenchmark.cpp
		benchmarks/ExtrudeBrushBenchmark.cpp
	)
	engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} NOINSTALL)
	engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark-app ${LIB})
endif()
