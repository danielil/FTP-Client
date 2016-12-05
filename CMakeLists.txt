cmake_minimum_required( VERSION 2.8 )
project( FTPClient )

set( CMAKE_CXX_FLAGS "-std=c++1z -Wall -Wextra -Werror -pedantic" )

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++" )
	set( CMAKE_SHARED_LINKER_FLAGS -lc++experimental )
endif()

set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

ADD_DEFINITIONS( -DUNICODE )
ADD_DEFINITIONS( -D_UNICODE )

include_directories( Includes )

file( GLOB_RECURSE SOURCES Sources/*.cpp )
add_executable( FTPClient ${SOURCES} )