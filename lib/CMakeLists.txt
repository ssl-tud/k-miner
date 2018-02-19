add_subdirectory(SVF)

# Due to a mutual dependencies, all the sub projects of the SVG are merged here
# Otherwise it is impossible to load the dependencies in opt
# NOTE: if the SVF should be linked into opt, we should probably use the individual sub projects here, rather than the combined project 
set(SOURCES
    KernelModels/KernelExtPass.cpp
    KernelModels/KernelSVFGBuilder.cpp
    KernelModels/Initcall.cpp
    KernelModels/KernelContextFactory.cpp
    KernelModels/InitcallFactory.cpp
    KernelModels/ContextBuilder.cpp
    KernelModels/SyscallBuilder.cpp
    KernelModels/DriverBuilder.cpp
    KernelModels/KernelPartitioner.cpp
    KernelModels/KernelContext.cpp
    Checker/UseAfterReturnChecker.cpp
    Checker/UseAfterReturnCheckerLite.cpp
    Checker/SrcSnkAnalysis.cpp
    Checker/KernelCheckerAPI.cpp
    Checker/DoubleLockChecker.cpp
    Checker/DoubleFreeChecker.cpp
    Checker/UseAfterFreeChecker.cpp
    Checker/MemLeakChecker.cpp
    Util/AnalysisContext.cpp
    Util/SyscallAPI.cpp
    Util/KMinerStat.cpp
    Util/DirRetAnalysis.cpp
    Util/PtrCallSetAnalysis.cpp
    Util/PAGAnalysis.cpp
    Util/ReportPass.cpp
    Util/KernelAnalysisUtil.cpp
    Util/InstrumentationUtil.cpp
    Util/DebugUtil.cpp
    Util/SVFGAnalysisUtil.cpp
    Util/CallGraphAnalysis.cpp
    Util/Bug.cpp)

add_llvm_loadable_module(Kminer ${SOURCES})
add_llvm_Library(LLVMKminer ${SOURCES})


if(DEFINED IN_SOURCE_BUILD)
    add_dependencies(Kminer intrinsics_gen)
    add_dependencies(LLVMKminer intrinsics_gen)
endif()
