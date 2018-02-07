cmake_minimum_required(VERSION 3.8.2)
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
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /D _CRT_SECURE_NO_WARNINGS")
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
set(MEngineIncludes ${IncludeDirectoryAbsolute}/MEngine)
set(MEngineDebugLibraryDirectoryAbsolute "${DebugLibraryDirectoryAbsolute}/MEngine")
set(MEngineReleaseLibraryDirectoryAbsolute "${ReleaseLibraryDirectoryAbsolute}/MEngine")
message("[${PROJECT_NAME}] - Assuming MEngine includes are located at path ${MEngineIncludes}")
message("[${PROJECT_NAME}] - Assuming MEngine debug libs are located at path ${MEngineDebugLibraryDirectoryAbsolute}")
message("[${PROJECT_NAME}] - Assuming MEngine release libs are located at path ${MEngineReleaseLibraryDirectoryAbsolute}")

find_library(MEngineDebugLibrary NAMES MEngine PATHS ${MEngineDebugLibraryDirectoryAbsolute})
find_library(MEngineReleaseLibrary NAMES MEngine PATHS ${MEngineReleaseLibraryDirectoryAbsolute})
message("[${PROJECT_NAME}] - Found MUtility debug libs at path ${MEngineDebugLibrary}")
message("[${PROJECT_NAME}] - Found MUtility release libs at path ${MEngineReleaseLibrary}")

# Find SDL2
set(SDL2LibraryDirectoryAbsolute "${SharedLibraryDirectoryAbsolute}/SDL2")
message("[${PROJECT_NAME}] - Assuming SDL2 libs are located at path ${SDL2LibraryDirectoryAbsolute}")

find_library(SDL2Library NAMES SDL2 SDL2 PATHS ${SDL2LibraryDirectoryAbsolute})
set(SDL2Libraries ${SDL2Library})
message("[${PROJECT_NAME}] - Found SDL2 base libs at path ${SDL2Library}")

find_library(SDL2imageLibrary NAMES SDL2_image PATHS ${SDL2LibraryDirectoryAbsolute})
list(APPEND SDL2Libraries ${SDL2imageLibrary})
message("[${PROJECT_NAME}] - Found SDL2 image libs at path ${SDL2imageLibrary}")

# Find MUtility
set(MUtilityIncludes "${IncludeDirectoryAbsolute}/MUtility")
set(MUtilityDebugLibraryDirectoryAbsolute "${DebugLibraryDirectoryAbsolute}/MUtility")
set(MUtilityReleaseLibraryDirectoryAbsolute "${ReleaseLibraryDirectoryAbsolute}/MUtility")
message("[${PROJECT_NAME}] - Assuming MUtility includes are located at path ${MUtilityIncludes}")
message("[${PROJECT_NAME}] - Assuming MUtility debug libs are located at path ${MUtilityDebugLibraryDirectoryAbsolute}")
message("[${PROJECT_NAME}] - Assuming MUtility release libs are located at path ${MUtilityReleaseLibraryDirectoryAbsolute}")

find_library(MUtilityDebugLibrary NAMES MUtility PATHS ${MUtilityDebugLibraryDirectoryAbsolute})
find_library(MUtilityReleaseLibrary NAMES MUtility PATHS ${MUtilityReleaseLibraryDirectoryAbsolute})
message("[${PROJECT_NAME}] - Found MUtility debug libs at path ${MUtilityDebugLibrary}")
message("[${PROJECT_NAME}] - Found MUtility release libs at path ${MUtilityReleaseLibrary}")

# Find Tubes
set(TubesIncludes ${IncludeDirectoryAbsolute}/Tubes)
set(TubesDebugLibraryDirectoryAbsolute "${DebugLibraryDirectoryAbsolute}/Tubes")
set(TubesReleaseLibraryDirectoryAbsolute "${ReleaseLibraryDirectoryAbsolute}/Tubes")
message("[${PROJECT_NAME}] - Assuming Tubes includes are located at path ${TubesIncludes}")
message("[${PROJECT_NAME}] - Assuming Tubes debug libs are located at path ${TubesDebugLibraryDirectoryAbsolute}")
message("[${PROJECT_NAME}] - Assuming Tubes release libs are located at path ${TubesReleaseLibraryDirectoryAbsolute}")

find_library(TubesDebugLibrary NAMES Tubes PATHS ${TubesDebugLibraryDirectoryAbsolute})
find_library(TubesReleaseLibrary NAMES Tubes PATHS ${TubesReleaseLibraryDirectoryAbsolute})
message("[${PROJECT_NAME}] - Found Tubes debug libs at path ${TubesDebugLibrary}")
message("[${PROJECT_NAME}] - Found Tubes release libs at path ${TubesReleaseLibrary}")

# List include directories
set(IncludeDirectoryList
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

# Set include directories
set_property(TARGET ${PROJECT_NAME} PROPERTY INCLUDE_DIRECTORIES ${IncludeDirectoryList})

# Set output directory
set(OutputDirectoryRelative ${ProjectRoot}bin/${OperatingSystemNameLowerCase})
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OutputDirectoryRelative})
get_filename_component(OutputDirectoryAbsolute ${OutputDirectoryRelative} ABSOLUTE)

# Make sure that the linker language gets set to C++
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

# Link debug libraries to the executable
target_link_libraries(${PROJECT_NAME} debug ${MEngineDebugLibrary} debug ${SDL2Libraries} debug ${MUtilityDebugLibrary} debug ${TubesDebugLibrary})

# Link release libraries to the executable
target_link_libraries(${PROJECT_NAME} optimized ${MEngineReleaseLibrary} optimized ${SDL2Libraries} optimized ${MUtilityReleaseLibrary} optimized ${TubesReleaseLibrary})

# Add postbuild scripts that moves DLL files to the directory of the executable
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${ConfigurationBasedExternalPathAbsolute} ${OutputDirectoryAbsolute}/$<CONFIGURATION>)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${SharedExternalsPathAbsolute} ${OutputDirectoryAbsolute}/$<CONFIGURATION>)

# Add postbuild scripts that moves resources to the directory of the executable
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${ResourcePathAbsolute} ${OutputDirectoryAbsolute}/$<CONFIGURATION>/resources)

# ---------- Dependencies ----------

# Get local(non versioned) paths to dependencies (Note: These values will be inherited to subdirectories if the subdirectories do not specify their own paths)
include("${CMAKE_CURRENT_LIST_DIR}/CMakeLocalDependenciesPaths.txt" OPTIONAL)

if(MengineRootPath)
	message("[${PROJECT_NAME}] - Found MEngine project at path ${MengineRootPath}")
	set(MEngineCMakePath "${MengineRootPath}/project")
	set(MEngineIncludeCopyPath "${MengineRootPath}/source/interface")
	set(MEngineDebugLibCopyPath "${MengineRootPath}/output/Debug")
	set(MEngineReleaseLibCopyPath "${MengineRootPath}/output/Release")
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${MEngineIncludeCopyPath} ${MEngineIncludes})
   	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy_directory ${MEngineDebugLibCopyPath} ${MEngineDebugLibraryDirectoryAbsolute})
	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy_directory ${MEngineReleaseLibCopyPath} ${MEngineReleaseLibraryDirectoryAbsolute})
	if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
		add_subdirectory(${MEngineCMakePath} ${MEngineCMakePath})
	endif(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
endif(MengineRootPath)

if(MUtilityRootPath)
	message("[${PROJECT_NAME}] - Found MUtility project at path ${MUtilityRootPath}")
	set(MUtilityCMakePath "${MUtilityRootPath}/project")
	set(MUtilityIncludeCopyPath "${MUtilityRootPath}/source/interface")
	set(MUtilityDebugLibCopyPath "${MUtilityRootPath}/output/Debug")
	set(MUtilityReleaseLibCopyPath "${MUtilityRootPath}/output/Release")
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${MUtilityIncludeCopyPath} ${MUtilityIncludes})
	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy_directory ${MUtilityDebugLibCopyPath} ${MUtilityDebugLibraryDirectoryAbsolute})
	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy_directory ${MUtilityReleaseLibCopyPath} ${MUtilityReleaseLibraryDirectoryAbsolute})
	if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
		add_subdirectory(${MUtilityCMakePath} ${MUtilityCMakePath})
	endif(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
endif(MUtilityRootPath)

if(TubesRootPath)	
	message("[${PROJECT_NAME}] - Found Tubes project at path ${TubesRootPath}")
	set(TubesCMakePath "${TubesRootPath}/project")
	set(TubesIncludeCopyPath "${TubesRootPath}/source/interface")
	set(TubesDebugLibCopyPath "${TubesRootPath}/output/Debug")
	set(TubesReleaseLibCopyPath "${TubesRootPath}/output/Release")
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${TubesIncludeCopyPath} ${TubesIncludes})
	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy_directory ${TubesDebugLibCopyPath} ${TubesDebugLibraryDirectoryAbsolute})
    add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy_directory ${TubesReleaseLibCopyPath} ${TubesReleaseLibraryDirectoryAbsolute})
    if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    	add_subdirectory(${TubesCMakePath} ${TubesCMakePath})
    endif(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
endif(TubesRootPath)

if(SDL2LibPath)
	message("[${PROJECT_NAME}] - Found SDL2lib copy path at ${SDL2LibPath}")
	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy_directory ${SDL2LibPath} ${SDL2LibraryDirectoryAbsolute})
endif(SDL2LibPath)

if(SDL2DLLPath)
	message("[${PROJECT_NAME}] - Found SDL2DLL copy path at ${SDL2DLLPath}")
	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy_directory ${SDL2DLLPath} ${SharedExternalsPathAbsolute})
endif(SDL2DLLPath)

# --- DEBUG COPY PASTE ---
#message("|||--- TEST ---||| ${var}")