#include "Checker/DoubleLockChecker.h"
#include "Util/AnalysisUtil.h"
#include <llvm/Support/CommandLine.h>

using namespace llvm;
using namespace analysisUtil;
using namespace svfgAnalysisUtil;

char DoubleLockChecker::ID = 0;

static RegisterPass<DoubleLockChecker> DOUBLELOCKCHECKER("double-lock", "Double-Lock Checker");


void DoubleLockChecker::analyze(llvm::Module &module) {
	SrcSnkAnalysis::initialize(module);
	SVFGNodeSet sources = getLockObjNodes();
	SVFGNodeSet sinks = getLockNodes();
	sinks.insert(getUnlockNodes().begin(), getUnlockNodes().end());
	numAnalyzedVars = sources.size();
	SrcSnkAnalysis::analyze(sources, sinks);
}

void DoubleLockChecker::evaluate(SrcSnkAnalysisContext *curAnalysisCxt) {
	const NodeIDPairVec &fsSinkPaths = curAnalysisCxt->getFlowSensSinkPaths(); 

	curAnalysisCxt->AllPathReachableSolve();
	curAnalysisCxt->filterUnreachablePaths();
	curAnalysisCxt->sortPaths();

	if(fsSinkPaths.size() == 0)
		return;

	for(int i=0; i < fsSinkPaths.size()-1; i++) {
		NodeIDPair curKey = fsSinkPaths[i];
		const SVFGNode *curNode = getGraph()->getSVFGNode(curKey.first);
		const KSrcSnkDPItem &item1 = curAnalysisCxt->getSinkItems().at(curKey);
		DPItemSet nonCriticalPaths;

		for(int j=i+1; j < fsSinkPaths.size(); j++) {
			NodeIDPair nextKey = fsSinkPaths[j];
			const SVFGNode *nextNode = getGraph()->getSVFGNode(nextKey.first);
			const KSrcSnkDPItem &item2 = curAnalysisCxt->getSinkItems().at(nextKey);

			if(item1.getFieldCxt() != item2.getFieldCxt())
				continue;

			if(isLockNode(curNode) && isLockNode(nextNode)) {
				std::string path1 = getFuncPathStr(getGraph(),item1.getCond().getVFEdges(), false);
				std::string path2 = getFuncPathStr(getGraph(),item2.getCond().getVFEdges(), false);

				if(nonCriticalPaths.find(&item2) == nonCriticalPaths.end() &&
				   curAnalysisCxt->isReachable(item1, item2)) {
					handleBug(curAnalysisCxt->getSource(), item1, item2);
				}
			} else if(isLockNode(curNode) && isUnlockNode(nextNode)) {
				if(!curAnalysisCxt->isReachable(item1, item2)) 
					continue;

				// All locks that could have been reached are no more critical.
				for(int z=j+1; z < fsSinkPaths.size(); z++) {
					NodeIDPair tmpKey = fsSinkPaths[z];
					const SVFGNode *tmpNode = getGraph()->getSVFGNode(tmpKey.first);
					const KSrcSnkDPItem &tmpItem = curAnalysisCxt->getSinkItems().at(tmpKey);

					if(curAnalysisCxt->isReachable(item2, tmpItem, true)) {
						nonCriticalPaths.insert(&tmpItem);
					}
				}

			} 
		}
	}
}
