find_package(Qt5Widgets)
find_package(Qt5Network)
find_package(Qt5Gui)

if (${Qt5Widgets_FOUND} AND ${Qt5Network_FOUND} AND ${Qt5Gui_FOUND})

project(rcon)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
qt5_add_resources(rcon_RESOURCES data/resources.qrc)

set(SRCS
	gui/action/Action.h
	gui/action/AddAction.h
	gui/action/DeleteAction.h
	gui/AICompressorProxy.h
	gui/dialog/AddDialog.h
	gui/dialog/IDialog.h
	gui/dialog/ConnectDialog.h
	gui/dialog/SettingsDialog.h
	gui/widget/IComboBox.h
	gui/AINodeStaticResolver.h
	gui/view/map/MapItem.h
	gui/view/map/MapView.h
	gui/view/tree/TreeViewCommon.h
	gui/view/tree/BehaviourTreeModel.h
	gui/view/tree/NodeTreeView.h
	gui/view/tree/BehaviourTreeModelItem.h
	gui/view/tree/NodeTreeItem.h
	gui/view/list/EntityListModel.h
	gui/view/list/EntityList.h
	gui/view/aggro/AggroTable.h
	gui/view/aggro/AggroTableModel.h
	gui/view/state/StateTableModel.h
	gui/view/state/StateTable.h
	gui/AIDebuggerWidget.h
	AIApplication.h
	AIDebugger.h
	common/Settings.h
	Version.h
	Main.cpp
	AIApplication.cpp
	AIDebugger.cpp
	rcon/RconAIApplication.h
	rcon/RconAIDebugger.h
	rcon/RconMapItem.h
	rcon/RconMapView.h
	gui/AIDebuggerWidget.cpp
	gui/AINodeStaticResolver.cpp
	gui/dialog/AddDialog.cpp
	gui/dialog/ConnectDialog.cpp
	gui/dialog/SettingsDialog.cpp
	gui/dialog/IDialog.cpp
	gui/view/list/EntityList.cpp
	gui/view/list/EntityListModel.cpp
	gui/view/map/MapView.cpp
	gui/view/map/MapItem.cpp
	gui/view/tree/BehaviourTreeModel.cpp
	gui/view/tree/BehaviourTreeModelItem.cpp
	gui/view/tree/NodeTreeItem.cpp
	gui/view/tree/NodeTreeView.cpp
	gui/view/tree/TreeViewCommon.cpp
	gui/view/state/StateTable.cpp
	gui/view/state/StateTableModel.cpp
	gui/view/aggro/AggroTable.cpp
	gui/view/aggro/AggroTableModel.cpp
	gui/widget/IComboBox.cpp
	data/resources.qrc
)

set_property(GLOBAL PROPERTY ${PROJECT_NAME}_NOUNITY TRUE)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES ai)
target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${AUTOGEN_TARGETS_FOLDER}
	${AUTOMOC_TARGETS_FOLDER}
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/gui
	${CMAKE_CURRENT_SOURCE_DIR}/gui/action
	${CMAKE_CURRENT_SOURCE_DIR}/gui/dialog
	${CMAKE_CURRENT_SOURCE_DIR}/gui/widget
	${CMAKE_CURRENT_SOURCE_DIR}/gui/view/map
	${CMAKE_CURRENT_SOURCE_DIR}/gui/view/tree
	${CMAKE_CURRENT_SOURCE_DIR}/gui/view/aggro
	${CMAKE_CURRENT_SOURCE_DIR}/gui/view/state
	${CMAKE_CURRENT_SOURCE_DIR}/gui/view/list
	${CMAKE_CURRENT_SOURCE_DIR}/common
)

qt5_use_modules(${PROJECT_NAME} Widgets Gui Network)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES network)

message(STATUS "Build the rcon")

else()

message(WARNING "Could not findl qt5 modules for network gui and widget support")

endif() # NO QT
