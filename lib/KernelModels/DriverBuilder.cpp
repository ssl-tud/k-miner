#include "KernelModels/DriverBuilder.h"
#include "SVF/Util/AnalysisUtil.h"

using namespace llvm;
using namespace analysisUtil;

void DriverBuilder::build(std::string cxtRootName) {
	outs() << "____________________________________________________\n";
	outs() << "DRIVERHANDLING:\n\n";

	// Will be removed by the ContextBuilder.
	Driver *driver = new Driver(cxtRootName);

	// Find the functions inside the driver file. (API)
	defineDriverAPI(driver);

	createContext(driver);
	optimizeContext();
}

void DriverBuilder::optimizeContext() {
	std::unique_ptr<llvm::Module> optModule = CloneModule(&module);
	anderOpt(*optModule.get(), context);
	flowOpt(*optModule.get(), context);
	minimizeContext(*optModule.get(), context);
	optModule.reset();
}

void DriverBuilder::defineDriverAPI(Driver *driver) {
	std::string driverName = driver->getName();
	StringSet api;
	const auto &funcList = module.getFunctionList();
	std::vector<const llvm::Function*> funcVec;

	for(const auto &iter : funcList) 
		funcVec.push_back(&iter);

	#pragma omp parallel for schedule(static, 100)
	for(int i=0; i < funcVec.size(); ++i) {
		const llvm::Function *F = funcVec.at(i);
		std::string fileName = getSourceFileNameOfFunction(F);
		std::string funcName = F->getName();

		if(fileName.find(driverName) != std::string::npos) {
			// The function has to be non-static or a function pointer has to exist
			if(F->hasExternalLinkage() || F->hasAddressTaken())
				if(!ExtAPI::getExtAPI()->is_ext(F) && 
				   !analysisUtil::isIntrinsicDbgFun(F))
					#pragma omp critical
					api.insert(funcName);	
		}
	}

	LocalCallGraphAnalysis *LCGA = new LocalCallGraphAnalysis(pta);

	// Only add functions to the api that are not called by another api function.
	for(const auto &iter : api) {
		LCGA->analyze(iter, false, false, 100);
		const StringSet &caller = LCGA->getBackwardFuncSlice();
		bool found = false;

		for(const auto &iter2 : caller) {
			if(iter != iter2 && api.find(iter2) != api.end())
				found = true;
		}
				
		if(!found) {
			driver->addToContextRoot(iter);
		} 
	}
	
	delete LCGA;
}
