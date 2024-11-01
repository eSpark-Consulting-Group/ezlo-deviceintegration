cmake_minimum_required(VERSION 3.5)

get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_ID})

file(GLOB SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/core/packets/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/core/pipeline/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/core/pipeline/helpers/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/crc/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/logger/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/pipeline/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/serial_port/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/timer/*.cpp
)

add_library(${PROJECT_NAME}
	${SRC_FILES}
)

target_include_directories(${PROJECT_NAME}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES(${PROJECT_NAME} "" "${CMAKE_CURRENT_SOURCE_DIR}/src;${CMAKE_CURRENT_SOURCE_DIR}/tests" "")
