cmake_minimum_required(VERSION 3.8.2)
set(CMAKE_CONFIGURATION_TYPES Debug Release)

# Project name
project(Mirage CXX)

# OS Name
string(TOLOWER ${CMAKE_SYSTEM_NAME} OperatingSystemNameLowerCase)

# Root directory
set(ProjectRoot ${CMAKE_CURRENT_LIST_DIR}/../)
get_filename_component(ProjectRootAbsolute ${ProjectRoot} ABSOLUTE)

# Get local(non versioned) paths to dependencies (Note: These values will be inherited to subdirectories if the subdirectories do not specify their own paths)
include("${CMAKE_CURRENT_LIST_DIR}/CMakeLocalDependenciesPaths.txt" OPTIONAL)

# Externals paths
set(SharedExternalsPathAbsolute "${ProjectRootAbsolute}/external/windows/shared")
set(ConfigurationBasedExternalPathAbsolute "${ProjectRootAbsolute}/external/windows/$<CONFIGURATION>")

# Resource path
set(ResourcePathAbsolute "${ProjectRootAbsolute}/resources")

# License path
set(LicensePathAbsolute "${ProjectRootAbsolute}/license")

# Visual Studio settings
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /D _CRT_SECURE_NO_WARNINGS")
endif(MSVC)

# Include directory
file(RELATIVE_PATH IncludeDirectoryRelative ${Mirage_SOURCE_DIR} ${ProjectRootAbsolute}/include)
get_filename_component(IncludeDirectoryAbsolute ${IncludeDirectoryRelative} ABSOLUTE)

# Debug library directory
file(RELATIVE_PATH DebugLibraryDirectoryRelative ${Mirage_SOURCE_DIR} ${ProjectRootAbsolute}/lib/debug/x64)
get_filename_component(DebugLibraryDirectoryAbsolute ${DebugLibraryDirectoryRelative} ABSOLUTE)

# Release library directory
file(RELATIVE_PATH ReleaseLibraryDirectoryRelative ${Mirage_SOURCE_DIR} ${ProjectRootAbsolute}/lib/release/x64)
get_filename_component(ReleaseLibraryDirectoryAbsolute ${ReleaseLibraryDirectoryRelative} ABSOLUTE)

# Shared library directory
file(RELATIVE_PATH SharedLibraryDirectoryRelative ${Mirage_SOURCE_DIR} ${ProjectRootAbsolute}/lib/shared/x64)
get_filename_component(SharedLibraryDirectoryAbsolute ${SharedLibraryDirectoryRelative} ABSOLUTE)

# Find SDL2
set(SDL2LibraryDirectoryAbsolute "${SharedLibraryDirectoryAbsolute}/SDL2")

find_library(SDL2Library NAMES SDL2 PATHS ${SDL2LibraryDirectoryAbsolute})
set(SDL2Libraries ${SDL2Library})
message("[${PROJECT_NAME}] - Found SDL2 base libs at path ${SDL2Library}")

find_library(SDL2imageLibrary NAMES SDL2_image PATHS ${SDL2LibraryDirectoryAbsolute})
list(APPEND SDL2Libraries ${SDL2imageLibrary})
message("[${PROJECT_NAME}] - Found SDL2 image libs at path ${SDL2imageLibrary}")

find_library(SDL2_ttfLibrary NAMES SDL2_ttf PATHS ${SDL2LibraryDirectoryAbsolute})
list(APPEND SDL2Libraries ${SDL2_ttfLibrary})
message("[${PROJECT_NAME}] - Found SDL2_ttf libs at path ${SDL2_ttfLibrary}")

# Find SDL_FontCache
set(SDL_FontCacheDebugLibraryDirectoryAbsolute "${DebugLibraryDirectoryAbsolute}/SDL_FontCache")
set(SDL_FontCacheReleaseLibraryDirectoryAbsolute "${ReleaseLibraryDirectoryAbsolute}/SDL_FontCache")

find_library(SDL_FontCacheDebugLibrary NAMES SDL_FontCache PATHS ${SDL_FontCacheDebugLibraryDirectoryAbsolute})
find_library(SDL_FontCacheReleaseLibrary NAMES SDL_FontCache PATHS ${SDL_FontCacheReleaseLibraryDirectoryAbsolute})
message("[${PROJECT_NAME}] - Found SDL_FontCache debug libs at path ${SDL_FontCacheDebugLibrary}")
message("[${PROJECT_NAME}] - Found SDL_FontCache release libs at path ${SDL_FontCacheReleaseLibrary}")

# Find MUtility
set(MUtilityLocalIncludePath ${IncludeDirectoryAbsolute}/MUtility)
set(MUtilityLocalDebugLibPath "${DebugLibraryDirectoryAbsolute}/MUtility")
set(MUtilityLocalReleaseLibPath "${ReleaseLibraryDirectoryAbsolute}/MUtility")

if(MUtilityRootPath)
	message("[${PROJECT_NAME}] - Found MUtility project at path ${MUtilityRootPath}")
	set(MUtilityCMakePath "${MUtilityRootPath}/project")
	set(MUtilityIncludePath "${MUtilityRootPath}/source/interface")
	set(MUtilityDebugLibPath "${MUtilityRootPath}/output/Debug")
	set(MUtilityReleaseLibPath "${MUtilityRootPath}/output/Release")

	set(MUtilityIncludes ${MUtilityIncludePath})
	set(MUtilityDebugLibs ${MUtilityDebugLibPath})
	set(MUtilityReleaseLibs ${MUtilityReleaseLibPath})
else(MUtilityRootPath)
	set(MUtilityIncludes ${MUtilityLocalIncludePath})
	set(MUtilityDebugLibs ${MUtilityLocalDebugLibPath})
	set(MUtilityReleaseLibs ${MUtilityLocalReleaseLibPath})
endif(MUtilityRootPath)

find_library(MUtilityDebugLibrary NAMES MUtility PATHS ${MUtilityDebugLibs})
message("[${PROJECT_NAME}] - Found MUtility debug libs at path ${MUtilityDebugLibrary}")
find_library(MUtilityReleaseLibrary NAMES MUtility PATHS ${MUtilityReleaseLibs})
message("[${PROJECT_NAME}] - Found MUtility release libs at path ${MUtilityReleaseLibrary}")

# Find MEngine
set(MEngineLocalIncludePath ${IncludeDirectoryAbsolute}/MEngine)
set(MEngineLocalDebugLibPath "${DebugLibraryDirectoryAbsolute}/MEngine")
set(MEngineLocalReleaseLibPath "${ReleaseLibraryDirectoryAbsolute}/MEngine")

if(MengineRootPath)
	message("[${PROJECT_NAME}] - Found MEngine project at path ${MengineRootPath}")
	set(MEngineCMakePath "${MengineRootPath}/project")
	set(MEngineIncludePath "${MengineRootPath}/source/interface")
	set(MEngineDebugLibPath "${MengineRootPath}/output/Debug")
	set(MEngineReleaseLibPath "${MengineRootPath}/output/Release")

	set(MEngineIncludes ${MEngineIncludePath})
	set(MEngineDebugLibs ${MEngineDebugLibPath})
	set(MEngineReleaseLibs ${MEngineReleaseLibPath})
else(MengineRootPath)
	set(MEngineIncludes ${MEngineLocalIncludePath})
	set(MEngineDebugLibs ${MEngineLocalDebugLibPath})
	set(MEngineReleaseLibs ${MEngineLocalReleaseLibPath})
endif(MengineRootPath)

find_library(MEngineDebugLibrary NAMES MEngine PATHS ${MEngineDebugLibs})
message("[${PROJECT_NAME}] - Found MEngine debug libs at path ${MEngineDebugLibrary}")
find_library(MEngineReleaseLibrary NAMES MEngine PATHS ${MEngineReleaseLibs})
message("[${PROJECT_NAME}] - Found MEngine release libs at path ${MEngineReleaseLibrary}")

# Find Tubes
set(TubesLocalIncludePath ${IncludeDirectoryAbsolute}/Tubes)
set(TubesLocalDebugLibPath "${DebugLibraryDirectoryAbsolute}/Tubes")
set(TubesLocalReleaseLibPath "${ReleaseLibraryDirectoryAbsolute}/Tubes")

if(TubesRootPath)
	message("[${PROJECT_NAME}] - Found Tubes project at path ${TubesRootPath}")
	set(TubesCMakePath "${TubesRootPath}/project")
	set(TubesIncludePath "${TubesRootPath}/source/interface")
	set(TubesDebugLibPath "${TubesRootPath}/output/Debug")
	set(TubesReleaseLibPath "${TubesRootPath}/output/Release")

	set(TubesIncludes ${TubesIncludePath})
	set(TubesDebugLibs ${TubesDebugLibPath})
	set(TubesReleaseLibs ${TubesReleaseLibPath})
else(TubesRootPath)
	set(TubesIncludes ${TubesLocalIncludePath})
	set(TubesDebugLibs ${TubesLocalDebugLibPath})
	set(TubesReleaseLibs ${TubesLocalReleaseLibPath})
endif(TubesRootPath)

find_library(TubesDebugLibrary NAMES Tubes PATHS ${TubesDebugLibs})
message("[${PROJECT_NAME}] - Found Tubes debug libs at path ${TubesDebugLibrary}")
find_library(TubesReleaseLibrary NAMES Tubes PATHS ${TubesReleaseLibs})
message("[${PROJECT_NAME}] - Found Tubes release libs at path ${TubesReleaseLibrary}")

# List include directories
set(IncludeDirectoryList
	${MUtilityIncludes}
	${MEngineIncludes}
	${TubesIncludes}
)
message("[${PROJECT_NAME}] - Include directories are:")
foreach(IncludeDir ${IncludeDirectoryList})
	message("	${IncludeDir}")
endforeach(IncludeDir)

# List source file directories
file(GLOB_RECURSE SOURCE_DIRECTORIES
	"../source/*.h"
	"../source/*.cpp"
)

# Create executable
if(MSVC) # TODODB: Check for windows platform instead
	add_executable(${PROJECT_NAME} WIN32 ${SOURCE_DIRECTORIES})
else(MSVC)
	add_executable(${PROJECT_NAME} ${SOURCE_DIRECTORIES})
endif(MSVC)

# Set include directories
set_property(TARGET ${PROJECT_NAME} PROPERTY INCLUDE_DIRECTORIES ${IncludeDirectoryList})

# Set output directory
set(OutputDirectoryRelative ${ProjectRoot}bin/${OperatingSystemNameLowerCase})
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OutputDirectoryRelative})
get_filename_component(OutputDirectoryAbsolute ${OutputDirectoryRelative} ABSOLUTE)

# Make sure that the linker language gets set to C++
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

# Link debug libraries to the executable
target_link_libraries(${PROJECT_NAME} debug ${SDL2Libraries} debug ${SDL_FontCacheDebugLibrary} debug ${MUtilityDebugLibrary} debug ${MEngineDebugLibrary} debug ${TubesDebugLibrary})

# Link release libraries to the executable
target_link_libraries(${PROJECT_NAME} optimized ${SDL2Libraries} optimized ${SDL_FontCacheReleaseLibrary} optimized ${MUtilityReleaseLibrary} optimized ${MEngineReleaseLibrary} optimized ${TubesReleaseLibrary})

# Add postbuild scripts that moves DLL files to the directory of the executable
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${ConfigurationBasedExternalPathAbsolute} ${OutputDirectoryAbsolute}/$<CONFIGURATION>)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${SharedExternalsPathAbsolute} ${OutputDirectoryAbsolute}/$<CONFIGURATION>)

# Add postbuild scripts
# Move resources to the directory of the executable
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${ResourcePathAbsolute} ${OutputDirectoryAbsolute}/$<CONFIGURATION>/resources)

# Move License folder to the directory of the executable
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${LicensePathAbsolute} ${OutputDirectoryAbsolute}/$<CONFIGURATION>/license)

if(MUtilityRootPath)
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${MUtilityIncludePath} ${MUtilityLocalIncludePath})
	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy_directory ${MUtilityDebugLibPath} ${MUtilityLocalDebugLibPath})
	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy_directory ${MUtilityReleaseLibPath} ${MUtilityLocalReleaseLibPath})
	if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
		add_subdirectory(${MUtilityCMakePath} ${MUtilityCMakePath})
	endif(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
endif(MUtilityRootPath)

if(MengineRootPath)
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${MEngineIncludePath} ${MEngineLocalIncludePath})
   	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy_directory ${MEngineDebugLibPath} ${MEngineLocalDebugLibPath})
	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy_directory ${MEngineReleaseLibPath} ${MEngineLocalReleaseLibPath})
	if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
		add_subdirectory(${MEngineCMakePath} ${MEngineCMakePath})
	endif(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
endif(MengineRootPath)

if(TubesRootPath)	
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${TubesIncludePath} ${TubesLocalIncludePath})
	add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy_directory ${TubesDebugLibPath} ${TubesLocalDebugLibPath})
    add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy_directory ${TubesReleaseLibPath} ${TubesLocalReleaseLibPath})
    if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    	add_subdirectory(${TubesCMakePath} ${TubesCMakePath})
    endif(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
endif(TubesRootPath)

# --- DEBUG COPY PASTE ---
#message("|||--- TEST ---||| ${var}")