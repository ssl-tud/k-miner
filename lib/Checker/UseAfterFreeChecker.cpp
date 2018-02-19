#include "Checker/UseAfterFreeChecker.h"
#include "Util/AnalysisUtil.h"
#include <llvm/Support/CommandLine.h>

using namespace llvm;
using namespace analysisUtil;
using namespace svfgAnalysisUtil;

char UseAfterFreeChecker::ID = 0;

static RegisterPass<UseAfterFreeChecker> USEAFTERFREECHECKER("use-after-free", "Use-After-Free Checker");

void UseAfterFreeChecker::analyze(llvm::Module &module) {
	SrcSnkAnalysis::initialize(module);
	SVFGNodeSet sources = getAllocNodes();
	SVFGNodeSet sinks = getDeallocNodes();
	sinks.insert(getUseNodes().begin(), getUseNodes().end());
	sinks.insert(getNullStoreNodes().begin(), getNullStoreNodes().end());
	numAnalyzedVars = sources.size();
	SrcSnkAnalysis::analyze(sources, sinks);
}

void UseAfterFreeChecker::evaluate(SrcSnkAnalysisContext *curAnalysisCxt) {
	const NodeIDPairVec &fsSinkPaths = curAnalysisCxt->getFlowSensSinkPaths(); 

	curAnalysisCxt->AllPathReachableSolve();
	curAnalysisCxt->filterUnreachablePaths();
	curAnalysisCxt->sortPaths();

	if(fsSinkPaths.size() == 0)
		return;

	for(int i=0; i < fsSinkPaths.size()-1; i++) {
		NodeIDPair curKey = fsSinkPaths[i];
		const SVFGNode *curNode = getGraph()->getSVFGNode(curKey.first);

		for(int j=i+1; j < fsSinkPaths.size(); j++) {
			NodeIDPair nextKey = fsSinkPaths[j];
			const SVFGNode *nextNode = getGraph()->getSVFGNode(nextKey.first);
			const KSrcSnkDPItem &item1 = curAnalysisCxt->getSinkItems().at(curKey);
			const KSrcSnkDPItem &item2 = curAnalysisCxt->getSinkItems().at(nextKey);

			if(isDeallocNode(curNode) && isUseNode(nextNode)) {
				if(curAnalysisCxt->isReachable(item1, item2))  {
					bool res = handleBug(curAnalysisCxt->getSource(), item1, item2);

					if(res) break; // Only take the first use after a certain free.
				}
			} else if(isDeallocNode(curNode) && isNullStoreNode(nextNode)) {
				if(curAnalysisCxt->isReachable(item1, item2)) {
					break; 
				}
			}

		}
	}
}

