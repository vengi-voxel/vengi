set(SRCS
	TestApp.cpp TestApp.h
	TestMeshApp.cpp TestMeshApp.h
	ui/TestAppWindow.h
)
set(LIB testcore)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core frontend video ui)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
check_ui_files(${LIB})
copy_data_files(${LIB})
add_dependencies(${LIB} copy-data-${LIB})
