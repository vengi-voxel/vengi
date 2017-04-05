find_package(Qt5Widgets)

if(Qt5Widgets_FOUND)

project(profiler)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT DEFINED EASY_PROGRAM_VERSION_MAJOR)
    set(EASY_PROGRAM_VERSION_MAJOR 1)
    set(EASY_PROGRAM_VERSION_MINOR 0)
    set(EASY_PROGRAM_VERSION_PATCH 0)
    set(EASY_PRODUCT_VERSION_STRING "1.0.0")
endif(NOT DEFINED EASY_PROGRAM_VERSION_MAJOR)

add_definitions(
	-DEASY_PROFILER_VERSION_MAJOR=${EASY_PROGRAM_VERSION_MAJOR}
	-DEASY_PROFILER_VERSION_MINOR=${EASY_PROGRAM_VERSION_MINOR}
	-DEASY_PROFILER_VERSION_PATCH=${EASY_PROGRAM_VERSION_PATCH}
)

set(SRCS
    main.cpp
    blocks_graphics_view.h
    blocks_graphics_view.cpp
    blocks_tree_widget.h
    blocks_tree_widget.cpp
    descriptors_tree_widget.h
    descriptors_tree_widget.cpp
    easy_chronometer_item.h
    easy_chronometer_item.cpp
    easy_graphics_item.h
    easy_graphics_item.cpp
    easy_graphics_scrollbar.h
    easy_graphics_scrollbar.cpp
    easy_qtimer.h
    easy_qtimer.cpp
    globals.h
    globals.cpp
    globals_qobjects.cpp
    main_window.h
    main_window.cpp
    tree_widget_item.h
    tree_widget_item.cpp
    tree_widget_loader.h
    tree_widget_loader.cpp
    resources.qrc
    resources.rc
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)

if (MINGW)
    add_definitions(
        -D_WIN32_WINNT=0x0600
        -DSTRSAFE_NO_DEPRECATE
    )
    set (SPECIAL_LIB ${SPECIAL_LIB} ws2_32 psapi)
endif (MINGW)

if(UNIX)
    set(SPECIAL_LIB ${SPECIAL_LIB} pthread)
endif()

target_link_libraries(${PROJECT_NAME} easy_profiler ${SPECIAL_LIB})
target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${AUTOGEN_TARGETS_FOLDER}
	${AUTOMOC_TARGETS_FOLDER}
)

qt5_use_modules(${PROJECT_NAME} Widgets)

endif() # no QT
