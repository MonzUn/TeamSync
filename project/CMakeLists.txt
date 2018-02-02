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
add_executable(${PROJECT_NAME} ${SOURCE_DIRECTORIES})

# Set output directory
set(OutputDirectoryRelative ${ProjectRoot}bin/${OperatingSystemNameLowerCase})
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OutputDirectoryRelative})
get_filename_component(OutputDirectoryAbsolute ${OutputDirectoryRelative} ABSOLUTE)

# Make sure that the linker language gets set to C++
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

# Link debug libraries to the executable
target_link_libraries(${PROJECT_NAME} debug ${MEngineDebugLibraries} debug ${SDL2Libraries} debug ${MUtilityDebugLibraries} debug ${TubesDebugLibraries})

# Link release libraries to the executable
target_link_libraries(${PROJECT_NAME} optimized ${MEngineReleaseLibraries} optimized ${SDL2Libraries} optimized ${MUtilityReleaseLibraries} optimized ${TubesReleaseLibraries})

# Add postbuild scripts that moves DLL files to the directory of the executable
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${ConfigurationBasedExternalPathAbsolute} ${OutputDirectoryAbsolute}/$<CONFIGURATION>)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${SharedExternalsPathAbsolute} ${OutputDirectoryAbsolute}/$<CONFIGURATION>)

# Add postbuild scripts that moves resources to the directory of the executable
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${ResourcePathAbsolute} ${OutputDirectoryAbsolute}/$<CONFIGURATION>/resources)

# ---------- Dependencies ----------

# Get local(non versioned) paths to dependencies
include("${CMAKE_CURRENT_LIST_DIR}/CMakeLocalDependenciesPaths.txt" OPTIONAL)

if(MengineRootPath)
	message("[${PROJECT_NAME}] - Found MEngine at path ${MengineRootPath}")
	set(MEngineCMakePath "${MengineRootPath}/project")
	set(MEngineIncludeCopyPath "${MengineRootPath}/source/interface")
	set(MEngineDebugLibCopyPath "${MengineRootPath}/output/Debug")
	set(MEngineReleaseLibCopyPath "${MengineRootPath}/output/Release")
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${MEngineIncludeCopyPath} ${MEngineIncludes})
   	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${MEngineDebugLibCopyPath} ${MEngineDebugLibraryDirectoryAbsolute})
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${MEngineReleaseLibCopyPath} ${MEngineReleaseLibraryDirectoryAbsolute})
	if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
		add_subdirectory(${MEngineCMakePath} "${MEngineCMakePath}")
	endif(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
endif(MengineRootPath)

if(MUtilityRootPath)
	message("[${PROJECT_NAME}] - Found MUtility at path ${MUtilityRootPath}")
	set(MUtilityCMakePath "${MUtilityRootPath}/project")
	set(MUtilityIncludeCopyPath "${MUtilityRootPath}/source/interface")
	set(MUtilityDebugLibCopyPath "${MUtilityRootPath}/output/Debug")
	set(MUtilityReleaseLibCopyPath "${MUtilityRootPath}/output/Release")
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${MUtilityIncludeCopyPath} ${MUtilityIncludes})
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${MUtilityDebugLibCopyPath} ${MUtilityDebugLibraryDirectoryAbsolute})
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${MUtilityReleaseLibCopyPath} ${MUtilityReleaseLibraryDirectoryAbsolute})
	if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
		add_subdirectory(${MUtilityCMakePath} "${MUtilityCMakePath}")
	endif(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
endif(MUtilityRootPath)

if(TubesRootPath)	
	message("[${PROJECT_NAME}] - Found Tubes at path ${TubesRootPath}")
	set(TubesCMakePath "${TubesRootPath}/project")
	set(TubesIncludeCopyPath "${TubesRootPath}/source/interface")
	set(TubesDebugLibCopyPath "${TubesRootPath}/output/Debug")
	set(TubesReleaseLibCopyPath "${TubesRootPath}/output/Release")
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${TubesIncludeCopyPath} ${TubesIncludes})
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${TubesDebugLibCopyPath} ${TubesDebugLibraryDirectoryAbsolute})
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${TubesReleaseLibCopyPath} ${TubesReleaseLibraryDirectoryAbsolute})
    if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    	add_subdirectory(${TubesCMakePath} "${TubesCMakePath}")
    endif(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
endif(TubesRootPath)

if(SDL2LibPath)
	message("[${PROJECT_NAME}] - Found SDL2lib at path ${SDL2LibPath}")
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${SDL2LibPath} ${SDL2LibraryDirectoryAbsolute})
endif(SDL2LibPath)

if(SDL2DLLPath)
	message("[${PROJECT_NAME}] - Found SDL2DLL at path ${SDL2DLLPath}")
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${SDL2DLLPath} ${SharedExternalsPathAbsolute})
endif(SDL2DLLPath)

# --- DEBUG COPY PASTE ---
#message("|||--- TEST ---||| ${var}")