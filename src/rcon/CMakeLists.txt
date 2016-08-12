find_package(Qt5Widgets)
find_package(Qt5Network)
find_package(Qt5Gui)

if (${Qt5Widgets_FOUND} AND ${Qt5Network_FOUND} AND ${Qt5Gui_FOUND})

project(rcon)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
qt5_add_resources(rcon_RESOURCES data/resources.qrc)

set(SRCS
	Main.cpp
	AIApplication.cpp
	AIDebugger.cpp
	gui/AICompressorProxy.h
	gui/action/AddAction.h
	gui/action/DeleteAction.h
	gui/AIDebuggerWidget.cpp
	gui/AINodeStaticResolver.cpp
	gui/dialog/AddDialog.cpp
	gui/dialog/ConnectDialog.cpp
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
	gui/widget/ZoomFrame.cpp
	gui/widget/ZoomWidget.cpp
	gui/widget/IComboBox.cpp
	gui/widget/IGraphicsView.cpp
	data/resources.qrc
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} ai)
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

else()

message(STATUS "Could not findl qt5 modules for network gui and widget support")

endif() # NO QT
