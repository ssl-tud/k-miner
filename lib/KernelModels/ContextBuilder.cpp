#include "KernelModels/ContextBuilder.h"

using namespace llvm;
using namespace analysisUtil;

cl::opt<unsigned int> FUNCTIONLIMIT("func-limit", cl::init(3000),
		cl::desc("Max number of functions that will be analyzed."));

void ContextBuilder::createContext(KernelContextObj *context) {
	outs() << "Create Context: " << context->getName() << "\n";
	outs() << "Reduction Level 0: " << module.getFunctionList().size() << " Functions\n";

	// Set the new context.
	setContext(context);

	// Define the call-graph analysis.
	LocalCallGraphAnalysis *LCGA = new LocalCallGraphAnalysis(pta);

	for(const auto &iter : context->getContextRoot()) {
		std::string rootFuncName = iter;

		// Approximate all functions and variables that a relevant to the given function.
		LCGA->analyze(rootFuncName, true, true, std::numeric_limits<uint32_t>::max());
		const StringSet &relevantFuncs = LCGA->getForwardFuncSlice();
		const StringSet &relevantGVs = LCGA->getRelevantGlobalVars();

		// Add functions and variables to the context.
		context->addFunctions(relevantFuncs);
		context->addGlobalVars(relevantGVs);
	}

	outs() << "Reduction Level 1: " << context->getFunctions().size() << " Functions\n";

	delete LCGA;
}

void ContextBuilder::anderOpt(llvm::Module &module, KernelContextObj *context) {
	analysisUtil::minimizeModule(module, context->getFunctions(), context->getGlobalVars());

	// Define the call graph and pointer analysis of the kernel.
	const AndersenWaveDiff *anderdiff = AndersenWaveDiff::createAndersenWaveDiff(module); 
	PTACallGraph *fpta = anderdiff->getPTACallGraph();
	LocalCallGraphAnalysis *LCGA = new LocalCallGraphAnalysis(fpta);

	for(const auto &iter : context->getContextRoot())  {
		std::string rootFuncName = iter;

		// Determine all functions and variables that a relevant to the given function.
		LCGA->analyze(rootFuncName, true, false, std::numeric_limits<uint32_t>::max());
		uint32_t maxCGDepth = LCGA->getActualDepth();
		const StringSet &relevantFuncs = LCGA->getForwardFuncSlice();
		const StringSet &relevantGVs = LCGA->getRelevantGlobalVars();

		// Update the contexts.
		context->setFunctions(relevantFuncs);
		context->setGlobalVars(relevantGVs);
		context->setMaxCGDepth(maxCGDepth);
	}

	outs() << "Reduction Level 2: " << context->getFunctions().size() << " Functions\n";

	delete LCGA;
	AndersenWaveDiff::releaseAndersenWaveDiff();
	PAG::releasePAG();
}

void ContextBuilder::flowOpt(llvm::Module &module, KernelContextObj *context) {
	analysisUtil::minimizeModule(module, context->getFunctions(), context->getGlobalVars());

	// Define the call graph and pointer analysis of the kernel.
	const FlowSensitive *fspta = FlowSensitive::createFSWPA(module);
	PTACallGraph *fpta = fspta->getPTACallGraph();
	LocalCallGraphAnalysis *LCGA = new LocalCallGraphAnalysis(fpta);

	for(const auto &iter : context->getContextRoot())  {
		std::string rootFuncName = iter;

		// Determine all functions and variables that a relevant to the given function.
		LCGA->analyze(rootFuncName, true, false, std::numeric_limits<uint32_t>::max());
		uint32_t maxCGDepth = LCGA->getActualDepth();
		const StringSet &relevantFuncs = LCGA->getForwardFuncSlice();
		const StringSet &relevantGVs = LCGA->getRelevantGlobalVars();

		// Update the contexts.
		context->setFunctions(relevantFuncs);
		context->setGlobalVars(relevantGVs);
		context->setMaxCGDepth(maxCGDepth);
	}

	outs() << "Reduction Level 3: " << context->getFunctions().size() << " Functions\n";

	delete LCGA;
	FlowSensitive::releaseFSWPA();
	PAG::releasePAG();
}

void ContextBuilder::minimizeContext(llvm::Module &module, KernelContextObj *context) {
	// The call graph of the API doesn't has to be reduced.
	if(context->getFunctions().size() < FUNCTIONLIMIT)
		return;

	analysisUtil::minimizeModule(module, context->getFunctions(), context->getGlobalVars());

	// Define the call graph and pointer analysis of the kernel.
	const AndersenWaveDiff *anderdiff = AndersenWaveDiff::createAndersenWaveDiff(module); 
	PTACallGraph *fpta = anderdiff->getPTACallGraph();
	LocalCallGraphAnalysis *LCGA = new LocalCallGraphAnalysis(fpta);

	uint32_t maxCGDepth = context->getMaxCGDepth();
	StringSet relevantFuncs;
	StringSet relevantGVs;

	// Reduce the depth of the call graph as long as the functions limit wont be exceeded.
	for(uint32_t depth = maxCGDepth; depth >= 0; depth--) {

		// Collect the contexts of all root functions.
		for(const auto &iter : context->getContextRoot())  {
			std::string rootFuncName = iter;

			LCGA->analyze(rootFuncName, true, false, depth);

			const StringSet &tmpFuncSet = LCGA->getForwardFuncSlice();
			const StringSet &tmpGVSet = LCGA->getRelevantGlobalVars();

			relevantFuncs.insert(tmpFuncSet.begin(), tmpFuncSet.end());
			relevantGVs.insert(tmpGVSet.begin(), tmpGVSet.end());
		}

		// If the current number of relevant functions doesn't exceed the max number of functions,
		// we dont have to decrease the callgraphDepth any further.
		if(relevantFuncs.size() < FUNCTIONLIMIT) {
			break;
		}
		
		relevantFuncs.clear();
		relevantGVs.clear();
	}	

	context->intersectFunctions(relevantFuncs);
	context->intersectGlobalVars(relevantGVs);

	delete LCGA;
	AndersenWaveDiff::releaseAndersenWaveDiff();
	PAG::releasePAG();
}
