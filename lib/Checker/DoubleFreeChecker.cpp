#include "Checker/DoubleFreeChecker.h"
#include "Util/AnalysisUtil.h"
#include <llvm/Support/CommandLine.h>

using namespace llvm;
using namespace analysisUtil;
using namespace svfgAnalysisUtil;

char DoubleFreeChecker::ID = 0;

static RegisterPass<DoubleFreeChecker> DOUBLEFREECHECKER("double-free", "Double-Free Checker");

void DoubleFreeChecker::analyze(llvm::Module &module) {
	SrcSnkAnalysis::initialize(module);
	SVFGNodeSet sources = getAllocNodes();
	SVFGNodeSet sinks = getDeallocNodes();
	sinks.insert(getNullStoreNodes().begin(), getNullStoreNodes().end());
	numAnalyzedVars = sources.size();
	SrcSnkAnalysis::analyze(sources, sinks);
}

void DoubleFreeChecker::evaluate(SrcSnkAnalysisContext *curAnalysisCxt) {
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

			if(isDeallocNode(curNode) && isDeallocNode(nextNode)) {
				std::string path1 = getFuncPathStr(getGraph(),item1.getCond().getVFEdges(), false);
				std::string path2 = getFuncPathStr(getGraph(),item2.getCond().getVFEdges(), false);

				if(curAnalysisCxt->isReachable(item1, item2)) {
//					if(item1.getLoc()->getId() == item2.getLoc()->getId())
//						if(item1.getHammingDistance(item2) < 3)
//						     continue; 

					bool res = handleBug(curAnalysisCxt->getSource(), item1, item2);
				}
			} else if(isDeallocNode(curNode) && isNullStoreNode(nextNode)) {
				if(curAnalysisCxt->isReachable(item1, item2)) {
					break; 
				}
			}
		}
	}
}

