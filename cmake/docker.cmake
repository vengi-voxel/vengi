set(DOCKER_BUILD_ARGS "--pull" CACHE STRING "Docker cli arguments for building an image")
set(DOCKER_RUN_ARGS "-it" CACHE STRING "Docker cli arguments for running an image")
set(DOCKER_DELETE_IUMAGE_ARGS "" CACHE STRING "Docker cli arguments for deleting an image")
set(DOCKER_PUSH_ARGS "" CACHE STRING "Docker cli arguments for pushing an image")
set(DOCKER_EXECUTABLE "docker" CACHE STRING "The docker cli to use for the docker target")
set(DOCKERCOMPOSE_BUILD_ARGS "" CACHE STRING "Docker-compose cli arguments for biilding the services")
set(DOCKERCOMPOSE_UP_ARGS "" CACHE STRING "Docker-compose cli arguments for starting the services")
set(DOCKERCOMPOSE_EXECUTABLE "docker-compose" CACHE STRING "The docker-compose cli to use for the docker-compose target")

#
# Adds a docker target to a project if a Dockerfile (copied) or Dockerfile.in (template) is in the directory
# The working directory is the root dir of the project.
#
# The DOCKER_IMAGE_NAME_TAG variable that is set here, must be in sync with the kubernetes manifests
#
macro(engine_docker NAME)
	if (NOT MSVC)
	set(DOCKERFILE_SRC)
	set(DOCKERCOMPOSEFILE_SRC)
	set(DOCKERFILE_TARGET ${CMAKE_CURRENT_BINARY_DIR}/Dockerfile)
	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile.in)
		set(DOCKERFILE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile.in)
		configure_file(${DOCKERFILE_SRC} ${DOCKERFILE_TARGET} @ONLY)
	elseif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile)
		set(DOCKERFILE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile)
		configure_file(${DOCKERFILE_SRC} ${DOCKERFILE_TARGET} COPYONLY)
	endif()

	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/docker-compose.yml.in)
		set(DOCKERCOMPOSEFILE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/docker-compose.yml.in)
		configure_file(${DOCKERCOMPOSEFILE_SRC} docker-compose.yml @ONLY)
	elseif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/docker-compose.yml)
		set(DOCKERCOMPOSEFILE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/docker-compose.yml)
		configure_file(${DOCKERCOMPOSEFILE_SRC} docker-compose.yml COPYONLY)
	endif()

	if (DOCKER_REGISTRY)
		set(DOCKER_IMAGE_NAME_TAG "${DOCKER_REGISTRY}/${NAME}:${ROOT_PROJECT_VERSION}")
	else()
		set(DOCKER_IMAGE_NAME_TAG "${NAME}:${ROOT_PROJECT_VERSION}")
	endif()

	if (DOCKERFILE_SRC AND DOCKER_EXECUTABLE)
		add_custom_target(${NAME}-docker
			COMMAND
				${DOCKER_EXECUTABLE}
				build
				${DOCKER_BUILD_ARGS}
				-t ${DOCKER_IMAGE_NAME_TAG}
				.
				-f - < ${DOCKERFILE_TARGET}
			DEPENDS ${DOCKERFILE_TARGET} ${DOCKERFILE_SRC}
			SOURCES ${DOCKERFILE_SRC}
			USES_TERMINAL
			COMMENT "Docker image name of ${NAME} is ${DOCKER_IMAGE_NAME_TAG}"
			VERBATIM
			WORKING_DIRECTORY "${ROOT_DIR}"
		)

		add_custom_target(${NAME}-docker-run
			COMMAND
				${DOCKER_EXECUTABLE}
				run
				${DOCKER_RUN_ARGS}
				${DOCKER_IMAGE_NAME_TAG}
			USES_TERMINAL
			VERBATIM
			DEPENDS ${NAME}-docker
		)

		add_custom_target(${NAME}-docker-push
			COMMAND
				${DOCKER_EXECUTABLE}
				push
				${DOCKER_PUSH_ARGS}
				${DOCKER_IMAGE_NAME_TAG}
			COMMAND
				${DOCKER_EXECUTABLE}
				rmi
				${DOCKER_DELETE_IUMAGE_ARGS}
				${DOCKER_IMAGE_NAME_TAG}
			VERBATIM
			USES_TERMINAL
		)

		if (DOCKERCOMPOSE_EXECUTABLE AND DOCKERCOMPOSEFILE_SRC)
			add_custom_target(${NAME}-docker-compose-build
				COMMAND
					${DOCKERCOMPOSE_EXECUTABLE}
					build
					${DOCKERCOMPOSE_BUILD_ARGS}
				WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
				VERBATIM
				USES_TERMINAL
				DEPENDS ${DOCKERCOMPOSEFILE_SRC} ${DOCKERFILE_TARGET} ${DOCKERFILE_SRC}
			)
			add_custom_target(${NAME}-docker-compose-up
				COMMAND
					${DOCKERCOMPOSE_EXECUTABLE}
					up
					${DOCKERCOMPOSE_UP_ARGS}
				WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
				VERBATIM
				USES_TERMINAL
				DEPENDS ${DOCKERCOMPOSEFILE_SRC} ${DOCKERFILE_TARGET} ${DOCKERFILE_SRC}
			)
		endif()

	endif()
	endif(NOT MSVC)
endmacro()
