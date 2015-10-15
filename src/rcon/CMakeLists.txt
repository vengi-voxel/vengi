set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets)
find_package(Qt5Network)
find_package(Qt5Gui)
find_package(Qt5LinguistTools)

if (${Qt5Widgets_FOUND} AND ${Qt5Network_FOUND} AND ${Qt5Gui_FOUND} AND ${Qt5LinguistTools_FOUND})

qt5_add_resources(rcon_RESOURCES data/resources.qrc)

fips_begin_app(rcon windowed)
	fips_files(
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

	fips_include_directories(
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

	if (FIPS_LINUX)
		check_function_exists(__atomic_fetch_add_4 HAVE___ATOMIC_FETCH_ADD_4)
		if (NOT HAVE___ATOMIC_FETCH_ADD_4)
			check_library_exists(atomic __atomic_fetch_add_4 "" HAVE_LIBATOMIC)
			if (HAVE_LIBATOMIC)
				fips_libs("atomic")
			endif()
		endif()
	endif()

	fips_deps(simpleai)
fips_end_app()
qt5_use_modules(rcon Widgets Gui Network)

endif() # NO QT
