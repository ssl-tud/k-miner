#include "KernelModels/KernelPartitioner.h"
#include "Util/InstrumentationUtil.h"
#include "Util/CallGraphAnalysis.h"
#include "SVF/MemoryModel/PAGBuilder.h"

using namespace llvm;
using namespace analysisUtil;
using namespace instrumentationUtil;

char KernelPartitioner::ID = 0;

static RegisterPass<KernelPartitioner> KERNELPARTITIONER("kernel-partitioner", "KernelPartitioner");

extern cl::opt<unsigned int> FUNCTIONLIMIT;

void KernelPartitioner::initialize(llvm::Module &module) {
	this->module = &module;
	KernelContextObj *api = kernelCxt->getAPI();

	// Collect the statistical infos before the final partitioning.
	partStat.setNumIO(kernelCxt->getAllInitcallNames().size());
	partStat.setNumIOFuncs(kernelCxt->getAllInitcallFuncs().size());
	partStat.setNumIOGVs(kernelCxt->getAllInitcallGlobalVars().size());
	partStat.setNumIONDGVs(kernelCxt->getAllInitcallNonDefVars().size());
	partStat.setIOCGDepth(0);
	partStat.setNumAOFuncs(api->getFunctions().size());
	partStat.setNumAOGVs(api->getGlobalVars().size());
	partStat.setAOCGDepth(0);
	partStat.setNumKOFuncs(module.getFunctionList().size());
	partStat.setNumKOGVs(module.getGlobalList().size());

	outs() << "____________________________________________________\n";
	outs() << "Module before partitioning:\n\n";
	outs() << "Num relevant Functions: " << module.size() << "\n";
	outs() << "Num relevant GlobalVars: " << module.getGlobalList().size() << "\n";
}

void KernelPartitioner::finalize() {
	const auto &globalvars = module->getGlobalList();
	KernelContextObj *api = kernelCxt->getAPI();

	StringSet nondefvars;

	for(const auto &iter : globalvars) {
		if(iter.hasName())
			nondefvars.insert(iter.getName());	
	}

	analysisUtil::filterNonStructTypes(*module, nondefvars);

	// Collect the statistical infos after the final partitioning.
	partStat.setNumIR(kernelCxt->getAllInitcallNames().size());
	partStat.setNumIRFuncs(kernelCxt->getAllInitcallFuncs().size());
	partStat.setNumIRGVs(kernelCxt->getAllInitcallGlobalVars().size());
	partStat.setNumIRNDGVs(kernelCxt->getAllInitcallNonDefVars().size());
	partStat.setIRCGDepth(0);
	partStat.setNumARFuncs(api->getFunctions().size());
	partStat.setNumARGVs(api->getGlobalVars().size());
	partStat.setARCGDepth(0);
	partStat.setNumKRFuncs(module->getFunctionList().size());
	partStat.setNumKRGVs(module->getGlobalList().size());
	partStat.setFuncLimit(FUNCTIONLIMIT);
	KMinerStat::createKMinerStat()->setPartitionerStat(partStat);

	outs() << "____________________________________________________\n";
	outs() << "Module after partitioning:\n\n";
	outs() << "Num relevant Functions: " << module->size() << "\n";
	outs() << "Num relevant GlobalVars: " << module->getGlobalList().size() << "\n";

}

void KernelPartitioner::dumpPAGStat(llvm::Module &module) {
	PAGBuilder pagBuilder;
        SymbolTableInfo* symTable = SymbolTableInfo::Symbolnfo();
        symTable->buildMemModel(module);

//	PAG *pag  = pagBuilder.build(module);	

	u32_t numOfFunction = 0;
	u32_t numOfGlobal = 0;
	u32_t numOfStack = 0;
	u32_t numOfHeap = 0;
	u32_t total_num_ptr = symTable->valSyms().size();
//	u32_t total_num_obj = pag->getObjectNodeNum();
//	u32_t total_num_fieldObj = pag->getFieldObjNodeNum();
	
	std::set<SymID> memObjSet;

	for(const auto &iter : symTable->idToObjMap()) {
//		if(ObjPN* obj = dyn_cast<ObjPN>(node)) {
			const MemObj* mem = iter.second;//obj->getMemObj();
			if (memObjSet.insert(mem->getSymId()).second == false)
				continue;
			if(mem->isBlackHoleOrConstantObj())
				continue;
			if(mem->isFunction())
				numOfFunction++;
			if(mem->isGlobalObj())
				numOfGlobal++;
			if(mem->isStack())
				numOfStack++;
			if(mem->isHeap())
				numOfHeap++;
//		}
	}

	outs() << "Num Funcs= " << numOfFunction << "\n";
	outs() << "Num Global= " << numOfGlobal << "\n";
	outs() << "Num Stack= " << numOfStack << "\n";
	outs() << "Num Heap= " << numOfHeap << "\n";
	outs() << "Num total ptr= " << total_num_ptr << "\n";

	PAG::releasePAG();
}

void KernelPartitioner::mergeContexts() {
	InitcallMap &initcalls = kernelCxt->getInitcalls();
	KernelContextObj *api = kernelCxt->getAPI();
	StringSet totalFunctions = api->getFunctions();
	StringSet totalGlobalVars = api->getGlobalVars();
	StringSet totalNonDefVars;

	// Removes the initcalls that are not relevant for the analysis.(doesn't change the module!)
	filterRelevantInitcalls(initcalls, api);

	// Collect the relevant values.
	for(const auto &iter : initcalls) {
		const Initcall &initcall = iter.second;

		totalFunctions.insert(initcall.getFunctions().begin(), initcall.getFunctions().end());
		totalGlobalVars.insert(initcall.getGlobalVars().begin(), initcall.getGlobalVars().end());
		totalNonDefVars.insert(initcall.getNonDefVars().begin(), initcall.getNonDefVars().end());
	}	

	// Is needed to get filenames and line numbers etc.
	if(!totalFunctions.empty())
		totalFunctions.insert("llvm.dbg.declare");

	analysisUtil::minimizeModule(*module, totalFunctions, totalGlobalVars);
}

void KernelPartitioner::filterRelevantInitcalls(InitcallMap &initcalls, const KernelContextObj *api) {
	const StringSet apiGlobalVars = api->getGlobalVars(); 

	for(auto iter = initcalls.begin(); iter != initcalls.end();) {
		std::string initcallName = iter->first; 
		Initcall &initcall = iter->second;
		StringSet interSet;
		const StringSet &initcallFuncs = initcall.getFunctions();
		const StringSet &initcallGlobalVars = initcall.getGlobalVars();
		const StringSet &initcallNonDefVars = initcall.getNonDefVars();

		// Find the non defined global variables used by the systemcall. 
		set_intersection(apiGlobalVars.cbegin(), apiGlobalVars.cend(), 
				 initcallNonDefVars.begin(), initcallNonDefVars.end(), 
				 std::inserter(interSet, interSet.begin()));

		if(interSet.size() == 0) {
			iter = initcalls.erase(iter); 
		} else {
			// Functions that might used by the initcall to define the non defined variables.
			StringSet relevantFuncs = kernelCxt->getNonDefVarFunctions(initcall, interSet);
			// Global variables that might used by the initcall to define the non defined variables.
			StringSet relevantGVs = kernelCxt->getNonDefVarGlobalVars(initcall, interSet);
			// The nondefined variables as well.
			relevantGVs.insert(interSet.begin(), interSet.end());

			initcall.setFunctions(relevantFuncs);
			// Info:: globalvars can potentially contain nondefvars that are not in nondefvars.
			// 	  thats because they are not relevant for the systemcall but for the nondefvars.
			initcall.setGlobalVars(relevantGVs);
			initcall.setNonDefVars(interSet);
			iter++;
		}
	}
}



void KernelPartitioner::handleAPIParameter() {

	KernelContextObj *api = kernelCxt->getAPI();

	for(const auto &iter : api->getContextRoot()) {
		llvm::Function *F = module->getFunction(iter);

		if(!F) continue;

		instrumentationUtil::initializeParameterLocal(F);
	}
}

void KernelPartitioner::addInitcallsToAPICxt() {
	if(!isa<Systemcall>(kernelCxt->getAPI()))
		return;

	KernelContextObj *api = kernelCxt->getAPI();
	const IntToStrSet initcallLevelMap =  kernelCxt->getInitcallLevelMap();
	llvm::LLVMContext &ctx = getGlobalContext();
	llvm::IntegerType *intType = IntegerType::getInt32Ty(ctx);

	for(const auto &iter : api->getContextRoot()) {
		llvm::Function *F = module->getFunction(iter);

		if(!F) continue;

		llvm::BasicBlock *entryBlock = &*F->begin();
		llvm::Instruction *entryInst = &*entryBlock->begin();

		// Insert each relevant initcall at the beginning of the systemcall.
		for(auto iter = initcallLevelMap.rbegin(); iter != initcallLevelMap.rend(); ++iter) { 
			const StringSet &initcalls = iter->second;

			for(auto iter2 = initcalls.begin(); iter2 != initcalls.end(); ++iter2) {
				std::string initcallName = *iter2;
				llvm::Function *initcallF = module->getFunction(initcallName);
				instrumentationUtil::addCallSite(initcallF, entryBlock, entryBlock->begin());
			}
		}
	}
}

