project(server)
set(SRCS
	Server.cpp Server.h
)
set(LUA_SRCS
	ai/blacksmith.lua
	ai/rabbit.lua
	ai/shared.lua
	ai/wolf.lua
	behaviourtrees.lua
	attributes.lua
	event/generic.lua
	events.lua
	map/map001.lua
)
set(FILES
	server/server-autoexec.cfg
)
set(MANIFESTS
	deployment/deployment.yaml.in
	deployment/postgres-config.yaml.in
	deployment/postgres-deployment.yaml.in
	deployment/postgres-service.yaml.in
	deployment/postgres-storage.yaml.in
	deployment/secrets.yaml.in
	deployment/service.yaml.in
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} LUA_SRCS ${LUA_SRCS})
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES backend)
engine_docker(${PROJECT_NAME})
engine_manifests(${PROJECT_NAME} ${MANIFESTS})
