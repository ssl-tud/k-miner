#include "KernelModels/KernelContextFactory.h"
#include "KernelModels/SyscallBuilder.h"
#include "KernelModels/DriverBuilder.h"
#include "KernelModels/Systemcall.h"

static cl::opt<std::string> SYSCALL("syscall", cl::init(""),
		cl::desc("The analysis will start with this function."));
static cl::opt<std::string> DRIVER("driver", cl::init(""),
		cl::desc("The analysis will start with the API-functions of this driver."));

using namespace llvm;
using namespace analysisUtil;

void KernelContextFactory::initialize() {
	initKernelContext();

	if(SYSCALL != "" && DRIVER == "") {
		apiBuilder = new SyscallBuilder(module);
		rawAPIList.push_back(SYSCALL);
	} else if (SYSCALL == "" && DRIVER != "") {
		apiBuilder = new DriverBuilder(module);
		rawAPIList.push_back(DRIVER);
	} else {
		errs() << "Warning: A system call or driver has to be selected!\n";
		return;
	}

	// The creation of the initcall contexts only has to be done once.
	initcallFactory = InitcallFactory::createInitcallFactory(module);
	kernelCxt->setInitcalls(initcallFactory->getInitcalls());
	//TODO integrate both into initcall.h
	kernelCxt->setNonDefVarFuncs(initcallFactory->getNonDefVarFuncs());
	kernelCxt->setNonDefVarGVs(initcallFactory->getNonDefVarGVs());
}


bool KernelContextFactory::updateContext() {
	std::string cxtRootName = getNextCxtRoot();
	
	if(cxtRootName == "")
		return false;

	apiBuilder->build(cxtRootName);
	kernelCxt->setAPI(apiBuilder->getContext());

	// Set statistis of the context 
	cxtStat.setCxtName(kernelCxt->getAPI()->getName());
	cxtStat.setCxtType(kernelCxt->getAPI()->getContextTypeName());
	KMinerStat::createKMinerStat()->setContextStat(cxtStat);

	return true;
}

void KernelContextFactory::initKernelContext() {
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

	filterNonStructTypes(module, nondefvars);

	numNonDefVars = nondefvars.size();

	KernelContext::initKernelOriginals(numFuncs, numGlobalVars, numNonDefVars);
	kernelCxt = KernelContext::createKernelContext();
}

std::string KernelContextFactory::getNextCxtRoot() {
	std::string funcName;

	if(rawAPIList.empty())
		return "";
       
	funcName = rawAPIList.back();
	rawAPIList.pop_back();

	return funcName;
}
