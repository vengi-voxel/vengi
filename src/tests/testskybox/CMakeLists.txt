project(testskybox)
set(SRCS
	TestSkybox.h TestSkybox.cpp
)
set(FILES
	testskybox/test_bk.png
	testskybox/test_dn.png
	testskybox/test_ft.png
	testskybox/test_lf.png
	testskybox/test_rt.png
	testskybox/test_up.png
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore)
