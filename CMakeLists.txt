cmake_minimum_required(VERSION 3.15)

project(weapon-forge LANGUAGES CXX)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" WF_DEFAULT_VERSION LIMIT_COUNT 1)
set(WF_VERSION "${WF_DEFAULT_VERSION}" CACHE STRING "Plugin version string")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
	# Static runtime: no Visual C++ redistributable needed on the server
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# open.mp SDK (header-only), used by the open.mp component entry point
include(FetchContent)

set(WF_OMP_SDK_COMMIT "08cc69a32d96c0eb8fe063d26c78259e54ca853f" CACHE STRING "open.mp SDK commit")

FetchContent_Declare(omp_sdk
	GIT_REPOSITORY https://github.com/openmultiplayer/open.mp-sdk.git
	GIT_TAG ${WF_OMP_SDK_COMMIT}
	GIT_SUBMODULES_RECURSE TRUE
)

FetchContent_GetProperties(omp_sdk)

if(NOT omp_sdk_POPULATED)
	if(POLICY CMP0169)
		cmake_policy(SET CMP0169 OLD)
	endif()

	FetchContent_Populate(omp_sdk)
endif()

file(GLOB WF_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(weapon-forge MODULE ${WF_SOURCES})
set_target_properties(weapon-forge PROPERTIES PREFIX "")

string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" _wf_ver "${WF_VERSION}")
if(_wf_ver)
	math(EXPR WF_VERSION_NUMBER "${CMAKE_MATCH_1} * 10000 + ${CMAKE_MATCH_2} * 100 + ${CMAKE_MATCH_3}")
else()
	set(WF_VERSION_NUMBER 0)
endif()

target_compile_definitions(weapon-forge PRIVATE
	WF_VERSION="${WF_VERSION}"
	WF_VERSION_NUMBER=${WF_VERSION_NUMBER}
)

# Same configuration as the SDK's OMP-SDK target (its CMakeLists needs a newer CMake)
target_include_directories(weapon-forge SYSTEM PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/sdk/omp
	${omp_sdk_SOURCE_DIR}/include
	${omp_sdk_SOURCE_DIR}/lib/glm
	${omp_sdk_SOURCE_DIR}/lib/robin-hood-hashing/src/include
	${omp_sdk_SOURCE_DIR}/lib/span-lite/include
	${omp_sdk_SOURCE_DIR}/lib/string-view-lite/include
)

target_compile_definitions(weapon-forge PRIVATE
	GLM_FORCE_QUAT_DATA_WXYZ
	GLM_FORCE_SSE2
	NOMINMAX
	nssv_CONFIG_SELECT_STRING_VIEW=nssv_STRING_VIEW_NONSTD
	span_CONFIG_SELECT_SPAN=span_SPAN_NONSTD
)

if(WIN32)
	if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
		message(FATAL_ERROR "SA-MP/open.mp plugins must be 32-bit. Configure with: -A Win32")
	endif()

	target_sources(weapon-forge PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin.def")
	target_compile_definitions(weapon-forge PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)

	if(MSVC)
		target_compile_options(weapon-forge PRIVATE /W4 /permissive- /utf-8 $<$<CONFIG:Release>:/O2 /GL>)
		target_link_options(weapon-forge PRIVATE $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>)
	endif()
else()
	target_compile_options(weapon-forge PRIVATE -m32 -msse2 -mfpmath=sse -fPIC -fvisibility=hidden -Wall -Wextra -O2)
	target_link_options(weapon-forge PRIVATE -m32 -static-libstdc++ -static-libgcc -Wl,--no-undefined)
endif()
