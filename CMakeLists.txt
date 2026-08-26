cmake_minimum_required(VERSION 3.20)
project(CppApplicationFramework VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_PLUGINS "Build example plugins" ON)
option(BUILD_QT6 "Build the optional Qt6/QML adapter and example" OFF)
option(ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(ENABLE_CLANG_TIDY "Run clang-tidy during compilation" OFF)
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(ENABLE_CLANG_TIDY)
	find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy REQUIRED)
	set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}")
endif()

if(ENABLE_SANITIZERS AND MSVC)
	message(FATAL_ERROR "ENABLE_SANITIZERS requires GCC or Clang")
endif()

add_subdirectory(core)
add_subdirectory(services)
add_subdirectory(runtime)
add_subdirectory(modules/example_module)
if(BUILD_QT6)
	add_subdirectory(apps/qt6_app)
endif()

if(BUILD_PLUGINS)
	add_subdirectory(plugins/example_plugin)
endif()
set(FRAMEWORK_COMPILE_TARGETS framework_services framework_runtime framework_example_module)
if(BUILD_PLUGINS)
	list(APPEND FRAMEWORK_COMPILE_TARGETS framework_example_plugin
		framework_invalid_api_plugin framework_invalid_abi_plugin
		framework_missing_descriptor_plugin framework_missing_create_plugin
		framework_missing_destroy_plugin framework_null_module_plugin)
	list(APPEND FRAMEWORK_COMPILE_TARGETS framework_malformed_null_descriptor_plugin
		framework_malformed_empty_id_plugin framework_malformed_empty_name_plugin
		framework_malformed_null_dependencies_plugin
		framework_malformed_duplicate_dependencies_plugin)
endif()
foreach(target IN LISTS FRAMEWORK_COMPILE_TARGETS)
	if(TARGET ${target})
		if(ENABLE_WARNINGS)
			if(MSVC)
				target_compile_options(${target} PRIVATE /W4 /WX /permissive-)
			else()
				target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
			endif()
		endif()
		if(ENABLE_SANITIZERS)
			target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
			target_link_options(${target} PRIVATE -fsanitize=address,undefined)
		endif()
	endif()
endforeach()

