
#Creates test target for passed target
function(HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES PROJECT_NAME PROJECT_REQUIRED_LIBS PROJECT_REQUIRED_INCLUDES EXTRA_SOURCES)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
        if (WITH_TESTS)
            set( GTEST_DEPS_LIBS
                gmock_main
                gmock
                gtest )

            file(GLOB ${PROJECT_NAME}_TEST_SRC_FILES tests/*.cpp tests/*.h)
            add_executable(${PROJECT_NAME}_tests ${${PROJECT_NAME}_TEST_SRC_FILES} ${EXTRA_SOURCES})
            set_target_properties( ${PROJECT_NAME}_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${TESTS_DIR}" 
                    LINK_FLAGS "--enable-new-dtags -Wl,-rpath,${TESTS_DIR}" )

            target_link_libraries(${PROJECT_NAME}_tests
                    ${GTEST_DEPS_LIBS}
                    ${PROJECT_REQUIRED_LIBS}
                    ${lib_name_rt}
                    fake_db
            )

            get_target_property(target_type ${PROJECT_NAME} TYPE)
  
            if(NOT target_type STREQUAL "EXECUTABLE")
                target_link_libraries(${PROJECT_NAME}_tests ${PROJECT_NAME})
            endif()

            target_include_directories( ${PROJECT_NAME}_tests PRIVATE ${PROJECT_REQUIRED_INCLUDES} )

            add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME}_tests )
        endif(WITH_TESTS)
    endif()
endfunction(HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES)

#Creates default test target for passed target
function(HUB_ADD_TESTS PROJECT_NAME )
    HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES( "${PROJECT_NAME}" "" "" "" )
endfunction(HUB_ADD_TESTS)

function(HUB_ADD_TESTS_WITH_EXTRA_LIBS PROJECT_NAME PROJECT_REQUIRED_LIBS)
    HUB_ADD_TESTS_WITH_EXTRA_DEPENDENCIES( "${PROJECT_NAME}" "${PROJECT_REQUIRED_LIBS}" "" "" )
endfunction(HUB_ADD_TESTS_WITH_EXTRA_LIBS)

#Creates a lua binding target for a passed target
function( HUB_ADD_LUA_BINDING_WITH_EXTRA_DEPENDENCIES MODULE_NAME PROJECT_REQUIRED_LIBS PROJECT_REQUIRED_INCLUDES )
    if( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bindings" )
        file( GLOB SRC_FILES bindings/*.cpp )

        get_target_property( project_include_dirs ${PROJECT_NAME} INCLUDE_DIRECTORIES )

        include_directories( project_include_dirs ${PROJECT_REQUIRED_INCLUDES} )

        add_library( lua_${MODULE_NAME} MODULE ${SRC_FILES} )
        target_link_libraries( lua_${MODULE_NAME}
            lua_binding_interface
            log_utils
            ${PROJECT_REQUIRED_LIBS}
        )
    endif()
endfunction( HUB_ADD_LUA_BINDING_WITH_EXTRA_DEPENDENCIES )
    
#Creates a default lua binding target for a passed target
function( HUB_ADD_LUA_BINDING MODULE_NAME PROJECT_REQUIRED_LIBS )
    HUB_ADD_LUA_BINDING_WITH_EXTRA_DEPENDENCIES( "${MODULE_NAME}" "${PROJECT_REQUIRED_LIBS}" "" )
endfunction( HUB_ADD_LUA_BINDING )

#Configures architecture dependent files and returns output directory to first param
#usage: CONFIGURE_ARCH_DEPENDENT_FILES(CURRENT_GENERATED_DIR file1.h file2.h file3.cpp)
macro(CONFIGURE_ARCH_DEPENDENT_FILES CURRENT_GENERATED_DIR)
    file(RELATIVE_PATH PATH_FROM_ROOT "${SOURCES_ROOT}" "${CMAKE_CURRENT_SOURCE_DIR}")
    get_filename_component(${CURRENT_GENERATED_DIR} "${PATH_FROM_ROOT}" ABSOLUTE BASE_DIR "${GENERATED_DIR}")

    set(FILE_NAMES "${ARGN}")
    foreach(FILE_NAME IN LISTS FILE_NAMES)
        configure_file("${FILE_NAME}.in" "${${CURRENT_GENERATED_DIR}}/${FILE_NAME}")
    endforeach()
endmacro()

#Configures architecture dependent config files from CFG_PATH directory and returns output directory to first param.
#Usage: CONFIGURE_CFG_FILES(CURRENT_GENERATED_DIR CFG_PATH file1.h file2.h file3.cpp)
macro(CONFIGURE_CFG_FILES CURRENT_GENERATED_DIR CFG_PATH)
    file(RELATIVE_PATH PATH_FROM_ROOT "${SOURCES_ROOT}" "${CFG_PATH}")
    get_filename_component(${CURRENT_GENERATED_DIR} "${PATH_FROM_ROOT}" ABSOLUTE BASE_DIR "${GENERATED_DIR}")
    set(GENERATED_OUTPUT_PATH "${GENERATED_DIR}/${PATH_FROM_ROOT}")

    set(FILE_NAMES "${ARGN}")
    foreach(FILE_NAME IN LISTS FILE_NAMES)
        configure_file("${CFG_PATH}/${FILE_NAME}.in" "${GENERATED_OUTPUT_PATH}/${FILE_NAME}")
    endforeach()
endmacro()

macro( HUB_FIND_SHARED_LIBRARY LIB_VARIABLE_NAME LIB_PHYSICAL_NAME LIB_PATHS )
    find_library(
        ${LIB_VARIABLE_NAME}
        ${CMAKE_SHARED_LIBRARY_PREFIX}${LIB_PHYSICAL_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
        PATHS ${LIB_PATHS} NO_DEFAULT_PATH )
    message( STATUS "${COLOR_INFO}--->${COLOR_NOTICE} found library ${COLOR_VAR_NAME}${LIB_VARIABLE_NAME}${COLOR_VAR_NOTICE}=${COLOR_VAR_VAL}${${LIB_VARIABLE_NAME}}${COLOR_RESET}" )
endmacro( HUB_FIND_SHARED_LIBRARY )

macro( HUB_FIND_LIBRARY LIB_VARIABLE_NAME LIB_PHYSICAL_NAME LIB_PATHS )
    find_library(
            ${LIB_VARIABLE_NAME}
            ${LIB_PHYSICAL_NAME}
            PATHS ${LIB_PATHS} NO_DEFAULT_PATH )
    message( STATUS "${COLOR_INFO}--->${COLOR_NOTICE} found library ${COLOR_VAR_NAME}${LIB_VARIABLE_NAME}${COLOR_VAR_NOTICE}=${COLOR_VAR_VAL}${${LIB_VARIABLE_NAME}}${COLOR_RESET}" )
endmacro( HUB_FIND_LIBRARY )

macro( HUB_FIND_SHARED_LIBRARY_IN_ALL_PATHS LIB_VARIABLE_NAME LIB_PHYSICAL_NAME )
    HUB_FIND_SHARED_LIBRARY( "${LIB_VARIABLE_NAME}" "${LIB_PHYSICAL_NAME}" "${LIBRARIES_DIR}" )
endmacro( HUB_FIND_SHARED_LIBRARY_IN_ALL_PATHS )

macro( HUB_FIND_LIBRARY_IN_ALL_PATHS LIB_VARIABLE_NAME LIB_PHYSICAL_NAME )
    HUB_FIND_LIBRARY( "${LIB_VARIABLE_NAME}" "${LIB_PHYSICAL_NAME}" "${LIBRARIES_DIR}" )
endmacro( HUB_FIND_LIBRARY_IN_ALL_PATHS )

macro( HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN LIB_VARIABLE_NAME LIB_PHYSICAL_NAME )
    HUB_FIND_SHARED_LIBRARY( "${LIB_VARIABLE_NAME}" "${LIB_PHYSICAL_NAME}" "${TOOLCHAIN_LIBRARIES_DIR}" )
endmacro( HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN )

macro( HUB_FIND_LIBRARY_IN_TOOLCHAIN LIB_VARIABLE_NAME LIB_PHYSICAL_NAME )
    HUB_FIND_LIBRARY( "${LIB_VARIABLE_NAME}" "${LIB_PHYSICAL_NAME}" "${TOOLCHAIN_LIBRARIES_DIR}" )
endmacro( HUB_FIND_LIBRARY_IN_TOOLCHAIN )

macro( HUB_FIND_SHARED_LIBRARY_IN_FIRMWARE LIB_VARIABLE_NAME LIB_PHYSICAL_NAME )
    HUB_FIND_SHARED_LIBRARY( "${LIB_VARIABLE_NAME}" "${LIB_PHYSICAL_NAME}" "${FIRMWARE_LIBRARIES_DIR}" )
endmacro( HUB_FIND_SHARED_LIBRARY_IN_FIRMWARE )

macro(EXTRACT_PACKAGE_VERSION VARIABLE_NAME PACKAGE_NAME)
    execute_process(
        COMMAND ${SOURCES_ROOT}/misc/scripts/openwrt/pkgdeps.py --opkg_dir ${SOURCES_ROOT}/misc/opkg --hub_type ${HUB_TYPE} --build_number ${FIRMWARE_BUILD_NUMBER} --action get_version --package ${PACKAGE_NAME}
        OUTPUT_VARIABLE ${VARIABLE_NAME}
    )
endmacro()

macro(INSTALL_ADDON_SERVICE BINARY_NAME PACKAGE_NAME)
    # Disabled for procd service files. Commenting for reference.
    #install( PROGRAMS ${SOURCES_ROOT}/system/h2.1/initd_addon_script DESTINATION ${PACKAGE_NAME}/etc/init.d RENAME ${BINARY_NAME} )
    install( CODE "execute_process( COMMAND bash -c \"ln -s ./hz-daemon.sh $DESTDIR/${CMAKE_INSTALL_PREFIX}/${PACKAGE_NAME}/opt/firmware/bin/${BINARY_NAME}.sh\" )" )
endmacro()
