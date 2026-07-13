cmake_minimum_required( VERSION 3.20 )

project( posix VERSION 1.0.0 )

if( CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME )
	set( PROJECT_IS_TOP_LEVEL TRUE )
else()
	set( PROJECT_IS_TOP_LEVEL FALSE )
endif()

include( FetchContent )

add_library( posix INTERFACE )

target_compile_features( posix
	INTERFACE
		cxx_std_20
)

target_include_directories( posix
	INTERFACE
		include
)

if( CMAKE_HOST_UNIX )
	find_package( Threads REQUIRED )
	target_link_libraries( posix INTERFACE Threads::Threads )
endif()

if( PROJECT_IS_TOP_LEVEL )

	FetchContent_Declare( catch 
		GIT_REPOSITORY
			https://github.com/catchorg/Catch2.git
		GIT_TAG
			v2.13.7
	)

	FetchContent_Makeavailable( catch )

	enable_testing()

	find_program( HEAD_COMMAND head REQUIRED )
	find_program( ENV_COMMAND env REQUIRED )

	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/test_main.in
		${CMAKE_CURRENT_BINARY_DIR}/test_main.cpp
	)

	add_executable( runtest
		tests.cpp
		file.cpp
		fstream.cpp
		pipe.cpp
		${CMAKE_CURRENT_BINARY_DIR}/test_main.cpp
	)

	target_link_libraries( runtest
		PRIVATE
			posix
			Catch2
	)

	target_compile_options( runtest
		PRIVATE
			-Wall
			-Wpedantic
			-Wconversion
	)

	add_test(
		NAME
			runtest
		COMMAND
			runtest
	)

endif()
