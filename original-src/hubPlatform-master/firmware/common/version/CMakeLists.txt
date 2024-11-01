get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project ( git_${PROJECT_ID} )

include_directories(
		${FIRMWARE_COMMON_DIR}
		${FIRMWARE_COMMON_GENERATED_DIR}
)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug AND NOT ${HUB_TYPE} MATCHES "local")
	set(HA_BRANCH "")
	set(HA_COMMIT "")
	set(HA_BUILD_DATE "")
	set(HA_BUILDER "")
else()
	execute_process (
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			COMMAND bash -c "git status|head -n1| cut -f 3 -d ' '"
			OUTPUT_VARIABLE HA_BRANCH
	)
	string(REGEX REPLACE "\n$" "" HA_BRANCH "${HA_BRANCH}")

	execute_process (
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			COMMAND bash -c "git describe --tags --abbrev=24 --always --dirty='-dirty'"
			OUTPUT_VARIABLE HA_COMMIT
	)
	string(REGEX REPLACE "\n$" "" HA_COMMIT "${HA_COMMIT}")

	option( FORCE_VERSION OFF)

	execute_process (
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			COMMAND bash -c "date +'%Y-%m-%dT%H:%M:%S%z' | tr -d '\n'"
			OUTPUT_VARIABLE HA_BUILD_DATE
	)

	execute_process (
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			COMMAND bash -c "echo \"$USER\"@`hostname` | tr -d '\n'"
			OUTPUT_VARIABLE HA_BUILDER
	)
endif()

CONFIGURE_ARCH_DEPENDENT_FILES( CURRENT_GENERATED_DIR ha_version.cpp )


set( SRC_FILES ${CURRENT_GENERATED_DIR}/ha_version.cpp)
add_library( ${PROJECT_NAME} STATIC ${SRC_FILES} )
target_include_directories(${PROJECT_NAME} PRIVATE ${CURRENT_GENERATED_DIR})


set( PACKAGE_NAME firmware-common-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
