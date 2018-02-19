#include "Checker/MemLeakChecker.h"
#include "Util/AnalysisUtil.h"
#include <llvm/Support/CommandLine.h>

using namespace llvm;
using namespace analysisUtil;
using namespace svfgAnalysisUtil;

char MemLeakChecker::ID = 0;

static RegisterPass<MemLeakChecker> MEMLEAKCHECKER("leak", "Memory-Leak Checker");

void MemLeakChecker::analyze(llvm::Module &module) {
	SrcSnkAnalysis::initialize(module);
	SVFGNodeSet sources = getAllocNodes();
	SVFGNodeSet sinks = getDeallocNodes();
	sinks.insert(getNullStoreNodes().begin(), getNullStoreNodes().end());
	numAnalyzedVars = sources.size();
	SrcSnkAnalysis::analyze(sources, sinks);
}

void MemLeakChecker::evaluate(SrcSnkAnalysisContext *curAnalysisCxt) {
	const ItemMap& sinkPaths = curAnalysisCxt->getSinkItems();
	curAnalysisCxt->AllPathReachableSolve();
	curAnalysisCxt->filterUnreachablePaths();

	for(const auto &it : sinkPaths) {
		const KSrcSnkDPItem &item = it.second;
		const SVFGNode* sink = item.getLoc();

		if(isDeallocNode(sink)) 
			return;
	}

	bool res = handleBug(curAnalysisCxt->getSource());
}

