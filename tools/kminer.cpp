#include "Checker/UseAfterReturnChecker.h"
#include "Checker/DoubleLockChecker.h"
#include "Checker/DoubleFreeChecker.h"
#include "Checker/UseAfterFreeChecker.h"
#include "Checker/MemLeakChecker.h"
#include "Checker/UseAfterReturnCheckerLite.h"
#include "KernelModels/KernelExtPass.h"
#include "KernelModels/KernelContextFactory.h"
#include "KernelModels/KernelContext.h"
#include "KernelModels/Driver.h"
#include "KernelModels/KernelPartitioner.h"
#include "Util/SyscallAPI.h"
#include "Util/KMinerStat.h"
#include "Util/DebugUtil.h"
#include "Util/AnalysisUtil.h"
#include "Util/BasicTypes.h"
#include "Util/ReportPass.h"

#include <llvm/Support/CommandLine.h>	
#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/IR/LegacyPassManager.h>	
#include <llvm/Support/Signals.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/IR/LLVMContext.h>		
#include <llvm/Support/SourceMgr.h> 	
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <omp.h>

using namespace llvm;
using namespace analysisUtil;

static cl::opt<std::string> InputFilename(cl::Positional,
		cl::desc("<input bitcode>"), cl::init("-"));

static cl::opt<bool> LEAKCHECKER("leak", cl::init(false),
		cl::desc("Memory Leak Detection"));

static cl::opt<bool> USEAFTERFREECHECKER("use-after-free", cl::init(false),
		cl::desc("UseAfterFree Detection"));

static cl::opt<bool> USEAFTERRETURNCHECKER("use-after-return", cl::init(false),
		cl::desc("UseAfterReturn Detection"));

static cl::opt<bool> USEAFTERRETURNCHECKERLITE("use-after-return-lite", cl::init(false),
		cl::desc("UseAfterReturn Detection (less accurate but faster)"));

static cl::opt<bool> DLOCKCHECKER("double-lock", cl::init(false),
		cl::desc("Double Lock Detection"));

static cl::opt<bool> FILECHECKER("fileck", cl::init(false),
		cl::desc("File Open/Close Detection"));

static cl::opt<bool> DFREECHECKER("double-free", cl::init(false),
		cl::desc("Double Free Detection"));

static cl::opt<bool> DUMPSYSCALLSTAT("dump-syscallstat", cl::init(false),
		cl::desc("Prints the systemcall analysis statistics"));

static cl::opt<unsigned int> NUMTHREADS("num-threads", cl::init(1),
		cl::desc("The number of threads the UseAfterReturnAnalysis will run on."));

/**
 * Kernel allocation functions shouldn't be analyzed. They will be removed
 * and replaced with a declaration. The analysis will handle the kernel
 * allocations as a normal user-space malloc.
 */
void setupModule(llvm::Module &module) {
	llvm::legacy::PassManager Passes;
	Passes.add(new KernelExtPass());
	Passes.run(module);
}

/**
 * Determines the number of functions, global variables and non defined variables
 * and adds them to the KernelContext.
 */
void initializeKernelContext(const llvm::Module &module) {
	const auto &functions = module.getFunctionList();
	const auto &globalvars = module.getGlobalList();
	uint32_t numFuncs = functions.size();
	uint32_t numGlobalVars = globalvars.size();
	uint32_t numNonDefVars = 0;
	StringSet nondefvars;

	for(const auto &iter : globalvars) {
		if(iter.hasName())
			nondefvars.insert(iter.getName());	
	}

	analysisUtil::filterNonStructTypes(module, nondefvars);

	numNonDefVars = nondefvars.size();

	KernelContext::initKernelOriginals(numFuncs, numGlobalVars, numNonDefVars);
}

/**
 * Initializing processes.
 */
void preprocessing(llvm::Module &module) {
	outs() << "\n\nPre-Processing:\n";
	outs() << "====================================================\n";

	omp_set_num_threads(NUMTHREADS);
	setupModule(module);

	KMinerStat *kminerStat = KMinerStat::createKMinerStat();
	kminerStat->setModuleName(module.getName());
	kminerStat->setNumThreads(NUMTHREADS);
}

/**
 * Finalizing processes.
 */
void postprocessing(llvm::Module &module) {
	outs() << "\n\nPost-Processing:\n";
	outs() << "====================================================\n";

	KMinerStat::releaseKMinerStat();
}

/**
 * Analyzes all the given system calls with the desired bug-detectors.
 */
void startAnalysis(llvm::Module &module) {
	preprocessing(module);
	KernelContextFactory *kernelCxtFactory = new KernelContextFactory(module);

	while(kernelCxtFactory->updateContext()) {
		llvm::legacy::PassManager Passes;

		Passes.add(new KernelPartitioner());

		if(USEAFTERRETURNCHECKER)
			Passes.add(new UseAfterReturnChecker(NUMTHREADS));
		if(USEAFTERRETURNCHECKERLITE)
			Passes.add(new UseAfterReturnCheckerLite(NUMTHREADS));
		if(LEAKCHECKER)
			Passes.add(new MemLeakChecker());
		if(USEAFTERFREECHECKER)
			Passes.add(new UseAfterFreeChecker());
		if(DFREECHECKER)
			Passes.add(new DoubleFreeChecker());
		if(DLOCKCHECKER)
			Passes.add(new DoubleLockChecker());

		Passes.add(new ReportPass());

		Passes.run(module);
	}

	delete kernelCxtFactory;
	postprocessing(module);
}

int main(int argc, char ** argv) {

	sys::PrintStackTraceOnErrorSignal();
	llvm::PrettyStackTraceProgram X(argc, argv);

	LLVMContext &Context = getGlobalContext();

	std::string OutputFilename;

	cl::ParseCommandLineOptions(argc, argv, "Software Bug Check\n");
	sys::PrintStackTraceOnErrorSignal();

	PassRegistry &Registry = *PassRegistry::getPassRegistry();

	initializeCore(Registry);
	initializeScalarOpts(Registry);
	initializeIPO(Registry);
	initializeAnalysis(Registry);
	initializeTransformUtils(Registry);
	initializeInstCombine(Registry);
	initializeInstrumentation(Registry);
	initializeTarget(Registry);

	SMDiagnostic Err;

	// Load the input module...
	std::unique_ptr<Module> M1 = parseIRFile(InputFilename, Err, Context);

	if (!M1) {
		Err.print(argv[0], errs());
		return 1;
	}

	startAnalysis(*M1.get());

	return 0;
}
