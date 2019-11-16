project(server)
set(SRCS
	Server.cpp Server.h
)
# These lua scripts are matching up with the entity type enum
set(LUA_BEHAVIOURTREES
	ai/blacksmith.lua
	ai/knight.lua
	ai/rabbit.lua
	ai/shared.lua
	ai/skeleton.lua
	ai/wolf.lua
	ai/worker.lua
	ai/zombie.lua
	behaviourtrees.lua
)
set(LUA_ATTRIBUTES
	attrib/blacksmith.lua
	attrib/knight.lua
	attrib/rabbit.lua
	attrib/shared.lua
	attrib/skeleton.lua
	attrib/wolf.lua
	attrib/worker.lua
	attrib/zombie.lua
	attributes.lua
)
set(LUA_EVENTS
	event/generic.lua
	events.lua
)
set(LUA_MAPS
	map/map001.lua
)
set(LUA_SRCS ${LUA_BEHAVIOURTREES} ${LUA_ATTRIBUTES} ${LUA_EVENTS} ${LUA_MAPS})

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
