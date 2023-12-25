cmake_minimum_required(VERSION 3.18)

project(terml CXX)

add_library(terml STATIC
	include/terml.h

	source/terml_private.h
	source/terml_windows.h
	source/terml_linux.h
	source/terml_windows.cpp
	source/terml_linux.cpp
	source/terml.cpp
)

set_target_properties(terml
	PROPERTIES
		CXX_STANDARD 11
)

target_include_directories(terml
	PUBLIC
		include
	PRIVATE
		source
)
