cmake_minimum_required(VERSION 3.10)

include(GNUInstallDirs)

project(varcreate-bin
	VERSION 0.1
	DESCRIPTION "Variable Creation Utility"
)

find_package( Threads REQUIRED )

find_library( LIB_VARSERVER varserver REQUIRED )
find_library( LIB_VARCREATE varcreate REQUIRED )
find_library( LIB_RT rt REQUIRED )

add_executable( ${PROJECT_NAME}
	src/varcreate.c
)

target_include_directories( ${PROJECT_NAME}
	PRIVATE inc
)

target_link_libraries( ${PROJECT_NAME}
	${LIB_VARSERVER}
	${LIB_VARCREATE}
	${LIB_RT}
)

set_target_properties( ${PROJECT_NAME}
		       PROPERTIES OUTPUT_NAME varcreate
)

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
