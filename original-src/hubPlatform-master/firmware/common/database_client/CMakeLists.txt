project (dbclient)

include_directories(
    ${FIRMWARE_COMMON_DIR}
)

link_directories( ${LIBRARIES_DIR} )

file ( GLOB SRC_FILES
    *.cpp *.h
    )

HUB_FIND_SHARED_LIBRARY_IN_TOOLCHAIN( lmdbLIB lmdb )

list( REMOVE_ITEM SRC_FILES dbid.cpp )

add_library( dbid dbid.cpp )
add_library( dbid_lib STATIC dbid.cpp )

add_library( ${PROJECT_NAME} ${SRC_FILES} )
target_link_libraries(${PROJECT_NAME}
						${lmdbLIB}
						config_reader
						json_utils
						$<${LIBC_HAS_NSL}:${nslLIB}>
                        dbid
)


# bindings
set( storage "storage" )

set( additionalDependencies
    config_reader
    dbclient
)

HUB_ADD_LUA_BINDING( ${storage} "${additionalDependencies}" )

set( PACKAGE_NAME firmware-common-${PROJECT_NAME})
install( TARGETS ${PROJECT_NAME} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )
install( TARGETS dbid DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )

set( PACKAGE_NAME firmware-common-lua_${storage})
install( TARGETS lua_${storage} DESTINATION ${PACKAGE_NAME}/opt/firmware/lib )

# fake db implementation
if( WITH_TESTS )

	file( GLOB SRC_FILES
    fake_db/*.cpp fake_db/*.h
    dbqueryparser.cpp
    dbupdate.cpp
    dbid.cpp
    idbclient.cpp
	)
    
	add_library( fake_db ${SRC_FILES} )
	target_link_libraries( fake_db
    json_utils
    log_utils
    test_utils
    # we need this dependency...
    service_interfaces
	)
	
	# as a temp solution
	set_target_properties( fake_db PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${TESTS_DIR} )
endif()

# tests
set(allDbClientTestLibraries service_interfaces ${lib_name_rt} ${lib_name_atomic} git_version)
HUB_ADD_TESTS_WITH_EXTRA_LIBS(${PROJECT_NAME} "${allDbClientTestLibraries}")
