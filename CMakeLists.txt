cmake_minimum_required(VERSION 3.4.3)

# To support both in- and out-of-source builds,
# we check for the presence of the add_llvm_loadable_module command.
# - if this command is not present, we are building out-of-source
if(NOT COMMAND add_llvm_loadable_module)
    if (DEFINED ENV{LLVM_DIR})
        # We need to match the build environment for LLVM:
        # In particular, we need C++11 and the -fno-rtti flag
        set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_FLAGS "-fno-rtti")
	set(CMAKE_BUILD_TYPE Debug)
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
	#set(CMAKE_LINK_FLAGS "-L/home/simon/bachelor-thesis-simon-schmitt/KMiner/lib/ -lSVF")

        find_package(LLVM REQUIRED CONFIG)

	set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
	find_package(Threads REQUIRED)
	if(CMAKE_USE_PTHREADS_INIT)
		set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-pthread")
	endif()
	
	find_package(OpenMP)
	if (OPENMP_FOUND)
		set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
		#set (CMAKE_EXE_LINKER_FLAGS  "${CMAKE_LINK_FLAGS}")
	endif()

	#list(APPEND CMAKE_MODULE_PATH "${/home/simon/SVF/cmake/modules/}")
	#find_package(TBB)

	#add_definitions(${TBB_DEFINITIONS})
	#include_directories(${TBB_INCLUDE})

        list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
        include(AddLLVM)

        add_definitions(${LLVM_DEFINITIONS})
        include_directories(${LLVM_INCLUDE_DIRS})

    else()
        message(FATAL_ERROR "\
WARNING: The LLVM_DIR var was not set (required for an out-of-source build)!\n\
Please set this to environment variable to point to the LLVM build directory\
(e.g. on linux: export LLVM_DIR=/path/to/llvm/build/dir)")
    endif()
else()
        set(IN_SOURCE_BUILD 1)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/include/SVF
	${CMAKE_CURRENT_SOURCE_DIR}/include/SVF/CUDD
                    ${CMAKE_CURRENT_BINARY_DIR}/include
		    ${CMAKE_CURRENT_BINARY_DIR}/include/SVF
		    ${CMAKE_CURRENT_BINARY_DIR}/include/SVF/CUDD
		    )

add_subdirectory(lib)
add_subdirectory(tools)

