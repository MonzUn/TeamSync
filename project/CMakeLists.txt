cmake_minimum_required(VERSION 3.0.2)
set(CMAKE_CONFIGURATION_TYPES Debug Release)

# Project name
project(TeamSync CXX)

# OS Name
string(TOLOWER ${CMAKE_SYSTEM_NAME} OperatingSystemNameLowerCase)

# Root directory
set(ProjectRoot ${CMAKE_CURRENT_LIST_DIR}/../)
get_filename_component(ProjectRootAbsolute ${ProjectRoot} ABSOLUTE)

# Externals paths
set(SharedExternalsPathAbsolute "${ProjectRootAbsolute}/external/windows/shared")
set(ConfigurationBasedExternalPathAbsolute "${ProjectRootAbsolute}/external/windows/$<CONFIGURATION>")

# Resource path
set(ResourcePathAbsolute "${ProjectRootAbsolute}/resources")

# Visual Studio settings
if(MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /D \"_CRT_SECURE_NO_WARNINGS\"")
endif(MSVC)

# Include directory
file(RELATIVE_PATH IncludeDirectoryRelative ${TeamSync_SOURCE_DIR} ${ProjectRootAbsolute}/include)
get_filename_component(IncludeDirectoryAbsolute ${IncludeDirectoryRelative} ABSOLUTE)

# Debug library directory
file(RELATIVE_PATH DebugLibraryDirectoryRelative ${TeamSync_SOURCE_DIR} ${ProjectRootAbsolute}/lib/debug/x64)
get_filename_component(DebugLibraryDirectoryAbsolute ${DebugLibraryDirectoryRelative} ABSOLUTE)

# Release library directory
file(RELATIVE_PATH ReleaseLibraryDirectoryRelative ${TeamSync_SOURCE_DIR} ${ProjectRootAbsolute}/lib/release/x64)
get_filename_component(ReleaseLibraryDirectoryAbsolute ${ReleaseLibraryDirectoryRelative} ABSOLUTE)

# Shared library directory
file(RELATIVE_PATH SharedLibraryDirectoryRelative ${TeamSync_SOURCE_DIR} ${ProjectRootAbsolute}/lib/shared/x64)
get_filename_component(SharedLibraryDirectoryAbsolute ${SharedLibraryDirectoryRelative} ABSOLUTE)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ProjectRoot}/bin/${OperatingSystemNameLowerCase}/)
get_filename_component(OutputDirectoryAbsolute ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ABSOLUTE)

# Find MEngine
find_path(MEngineInclude MEngine/mengine.h PATHS ${IncludeDirectoryAbsolute})
set(MEngineIncludes ${MEngineInclude}/MEngine)

set(MEngineDebugLibraryDirectoryAbsolute "${DebugLibraryDirectoryAbsolute}/MEngine")
find_library(MEngineDebugLibrary
	NAMES MEngine
	PATHS ${MEngineDebugLibraryDirectoryAbsolute})
set(MEngineDebugLibraries ${MEngineDebugLibrary})

set(MEngineReleaseLibraryDirectoryAbsolute "${ReleaseLibraryDirectoryAbsolute}/MEngine")
find_library(MEngineReleaseLibrary
	NAMES MEngine
	PATHS ${MEngineReleaseLibraryDirectoryAbsolute})
set(MEngineReleaseLibraries ${MEngineReleaseLibrary})

# Find SDL2
find_path(SDL2Include SDL2/SDL.h PATHS ${IncludeDirectoryAbsolute})
set(SDL2Includes ${SDL2Include}/SDL2)

set(SDL2LibraryDirectoryAbsolute "${SharedLibraryDirectoryAbsolute}/SDL2")
find_library(SDL2Library
	NAMES SDL2 SDL2
	PATHS ${SDL2LibraryDirectoryAbsolute})
set(SDL2Libraries ${SDL2Library})

find_library(SDL2imageLibrary
	NAMES SDL2_image
	PATHS ${SDL2LibraryDirectoryAbsolute})
list(APPEND SDL2Libraries ${SDL2imageLibrary})

# Find MUtility
find_path(MUtilityInclude MUtility/MUtilityByte.h PATHS ${IncludeDirectoryAbsolute})
set(MUtilityIncludes ${MUtilityInclude}/MUtility)

set(MUtilityDebugLibraryDirectoryAbsolute "${DebugLibraryDirectoryAbsolute}/MUtility")
find_library(MUtilityDebugLibrary
	NAMES MUtility
	PATHS ${MUtilityDebugLibraryDirectoryAbsolute})
set(MUtilityDebugLibraries ${MUtilityDebugLibrary})

set(MUtilityReleaseLibraryDirectoryAbsolute "${ReleaseLibraryDirectoryAbsolute}/MUtility")
find_library(MUtilityReleaseLibrary
	NAMES MUtility
	PATHS ${MUtilityReleaseLibraryDirectoryAbsolute})
set(MUtilityReleaseLibraries ${MUtilityReleaseLibrary})

# Find Tubes
find_path(TubesInclude Tubes/Tubes.h PATHS ${IncludeDirectoryAbsolute})
set(TubesIncludes ${TubesInclude}/Tubes)

set(TubesDebugLibraryDirectoryAbsolute "${DebugLibraryDirectoryAbsolute}/Tubes")
find_library(TubesDebugLibrary
	NAMES Tubes
	PATHS ${TubesDebugLibraryDirectoryAbsolute})
set(TubesDebugLibraries ${TubesDebugLibrary})

set(TubesReleaseLibraryDirectoryAbsolute "${ReleaseLibraryDirectoryAbsolute}/Tubes")
find_library(TubesReleaseLibrary
	NAMES Tubes
	PATHS ${TubesReleaseLibraryDirectoryAbsolute})
set(TubesReleaseLibraries ${TubesReleaseLibrary})

include_directories(
	${MEngineIncludes}
	${MUtilityIncludes}
	${TubesIncludes}
)

# List source file directories
file(GLOB_RECURSE SOURCE_DIRECTORIES
	"../source/*.h"
	"../source/*.cpp"
)

# Create executable
add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_DIRECTORIES})

# Make sure that the linker language gets set to C++
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

# Link debug libraries to the executable
target_link_libraries(${CMAKE_PROJECT_NAME} debug ${MEngineDebugLibraries} debug ${SDL2Libraries} debug ${MUtilityDebugLibraries} debug ${TubesDebugLibraries}
)

# Link release libraries to the executable
target_link_libraries(${CMAKE_PROJECT_NAME} optimized ${MEngineReleaseLibraries} optimized ${SDL2Libraries} optimized ${MUtilityReleaseLibraries} optimized ${TubesReleaseLibraries}
)

# Add postbuild scripts that moves DLL files to the directory of the executable
add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
	POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${ConfigurationBasedExternalPathAbsolute}
        ${OutputDirectoryAbsolute}/$<CONFIGURATION>)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
	POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SharedExternalsPathAbsolute}
        ${OutputDirectoryAbsolute}/$<CONFIGURATION>)

# Add postbuild scripts that moves resources to the directory of the executable
add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
	POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${ResourcePathAbsolute}
        ${OutputDirectoryAbsolute}/$<CONFIGURATION>/resources)

# Get local(non versioned) paths to dependencies
include("CMakeLocalDependenciesPaths.txt" OPTIONAL)

# Add prebuilds scripts that copies local dependencies to their place within the project
if(SDL2LibCopyPath)
	add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
	PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SDL2LibCopyPath}
        ${SDL2LibraryDirectoryAbsolute})
endif(SDL2LibCopyPath)

if(SDL2DLLCopyPath)
	add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
	PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SDL2DLLCopyPath}
        ${SharedExternalsPathAbsolute})
endif(SDL2DLLCopyPath)

if(MEngineIncludeCopyPath)
	add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
	PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${MEngineIncludeCopyPath}
        ${MEngineIncludes})
endif(MEngineIncludeCopyPath)

if(MEngineDebugLibCopyPath)
	add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
	PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${MEngineDebugLibCopyPath}
        ${MEngineDebugLibraryDirectoryAbsolute})
endif(MEngineDebugLibCopyPath)

if(MEngineReleaseLibCopyPath)
	add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
	PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${MEngineReleaseLibCopyPath}
        ${MEngineReleaseLibraryDirectoryAbsolute})
endif(MEngineReleaseLibCopyPath)

if(MUtilityIncludeCopyPath)
	add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
	PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${MUtilityIncludeCopyPath}
        ${MUtilityIncludes})
endif(MUtilityIncludeCopyPath)

if(MUtilityDebugLibCopyPath)
	add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
	PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${MUtilityDebugLibCopyPath}
        ${MUtilityDebugLibraryDirectoryAbsolute})
endif(MUtilityDebugLibCopyPath)

if(MUtilityReleaseLibCopyPath)
	add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
	PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${MUtilityReleaseLibCopyPath}
        ${MUtilityReleaseLibraryDirectoryAbsolute})
endif(MUtilityReleaseLibCopyPath)

if(TubesIncludeCopyPath)
	add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
	PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${TubesIncludeCopyPath}
        ${TubesIncludes})
endif(TubesIncludeCopyPath)

if(TubesDebugLibCopyPath)
	add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
	PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${TubesDebugLibCopyPath}
        ${TubesDebugLibraryDirectoryAbsolute})
endif(TubesDebugLibCopyPath)

if(TubesReleaseLibCopyPath)
	add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
	PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${TubesReleaseLibCopyPath}
        ${TubesReleaseLibraryDirectoryAbsolute})
endif(TubesReleaseLibCopyPath)

# --- DEBUG COPY PASTE ---
#message("|||--- TEST ---||| ${var}")