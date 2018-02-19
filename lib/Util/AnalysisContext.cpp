#include "Util/AnalysisContext.h"
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/CFG.h>

using namespace llvm;
using namespace analysisUtil;
using namespace svfgAnalysisUtil;

bool AnalysisContext::isReachable(const NodeIDPair &edge1, const NodeIDPair &edge2, bool dominates) {
	const SVFGNode *parentNode1 = getSVFG()->getSVFGNode(edge1.second);
	const SVFGNode *parentNode2 = getSVFG()->getSVFGNode(edge2.second);
	const SVFGNode *childNode1 = getSVFG()->getSVFGNode(edge1.first);
	const SVFGNode *childNode2 = getSVFG()->getSVFGNode(edge2.first);
	const llvm::Instruction *inst1;
	const llvm::Instruction *inst2;
	const llvm::BasicBlock *BB1 = childNode1->getBB();
	const llvm::BasicBlock *BB2 = childNode2->getBB();
	const llvm::Function *callerFunc;

	//TODO RM
	if(getSVFGFuncName(parentNode1) != getSVFGFuncName(parentNode2)) {
		return false;
	}
	assert(getSVFGFuncName(parentNode1) == getSVFGFuncName(parentNode2) && "Different parent nodes");
	
	if(!BB1 || !BB2) 
		return false;

	if(getSVFGFuncName(childNode1) != getSVFGFuncName(childNode2)) {
		inst1 = getInstruction(parentNode1, childNode1);
		inst2 = getInstruction(parentNode2, childNode2);
	 	callerFunc = parentNode1->getBB()->getParent();
	} else {
		if(llvm::isa<StmtSVFGNode>(childNode1)) {
			inst1 = llvm::dyn_cast<StmtSVFGNode>(childNode1)->getInst();
		} else if(llvm::isa<ActualParmSVFGNode>(childNode1)) {
			inst1 = llvm::dyn_cast<ActualParmSVFGNode>(childNode1)->getCallSite().getInstruction();
		}  else 
			return false;

		if(llvm::isa<StmtSVFGNode>(childNode2)) {
			inst2 = llvm::dyn_cast<StmtSVFGNode>(childNode2)->getInst();
		} else if(llvm::isa<ActualParmSVFGNode>(childNode2)) {
			inst2 = llvm::dyn_cast<ActualParmSVFGNode>(childNode2)->getCallSite().getInstruction();
		} else 
			return false;

		callerFunc = inst1->getParent()->getParent();
	}

	if(!inst1 || !inst2) {
		return false;
	} else if(inst1->getParent()->getParent() != inst2->getParent()->getParent()) {
		return false;
	}

	if(!callerFunc)
		return false;

	DominatorTree *dt;
	LoopInfo *li;

	dt = CFBuilder.getDT(callerFunc);

	if(!dominates) {
		li = CFBuilder.getLoopInfo(callerFunc);
		return isPotentiallyReachable(inst1, inst2, dt, li);
	}
		
	return dt->dominates(inst1, inst2);
}

bool AnalysisContext::isReachable(const KSrcSnkDPItem &item1, const KSrcSnkDPItem &item2, bool dominates) {
	const VFPathCond::EdgeSet &curPath = item1.getCond().getVFEdges();
	const VFPathCond::EdgeSet &curSortedPath = item2.getCond().getVFEdges();

	bool foundEdge1 = false;
	bool foundEdge2 = false;
	int j1=curPath.size()-1; 
	int j2=curSortedPath.size()-1;
	int lastCommonNodeIndex1 = j1;
	int lastCommonNodeIndex2 = j2;
	std::string commonFunction = "";

	while(!foundEdge1 || !foundEdge2) {
		const NodeIDPair &edge1 = curPath[j1];
		const NodeIDPair &edge2 = curSortedPath[j2];
		const SVFGNode *parentNode1 = getSVFG()->getSVFGNode(edge1.second);
		const SVFGNode *parentNode2 = getSVFG()->getSVFGNode(edge2.second);
		const SVFGNode *childNode1 = getSVFG()->getSVFGNode(edge1.first);
		const SVFGNode *childNode2 = getSVFG()->getSVFGNode(edge2.first);
		std::string prevFunction1 = getSVFGFuncName(parentNode1);
		std::string prevFunction2 = getSVFGFuncName(parentNode2);
		std::string curFunction1 = getSVFGFuncName(childNode1);
		std::string curFunction2 = getSVFGFuncName(childNode2);

		if(j1 > 0 && j2 > 0 && edge1.first == edge2.first) {
			commonFunction = curFunction1;
			j1--; j2--;
			continue;
		} else if(j1 > 0 && j2 > 0 && !foundEdge1 && !foundEdge2 && curFunction1 == curFunction2) {
			commonFunction = curFunction1;
			j1--; j2--;
			continue;
		} else if(commonFunction == prevFunction1 && isRet(edge1.second, edge1.first)) {
			return false;
		} else if(commonFunction == prevFunction2 && isRet(edge2.second, edge2.first)) {
			return false;
		} 

		if(commonFunction == prevFunction1 && !foundEdge1 && isCall(edge1.second, edge1.first))
			lastCommonNodeIndex1 = j1;

		if(commonFunction == prevFunction2 && !foundEdge2 && isCall(edge2.second, edge2.first))
			lastCommonNodeIndex2 = j2;
		else if(commonFunction == curFunction2 && foundEdge2)
			lastCommonNodeIndex2 = 0;

		if(j1 == 0) {
			foundEdge1 = true;

			if(commonFunction == curFunction1)
				lastCommonNodeIndex1 = 0;
		} else 
			j1--;
		
		if(j2 == 0) {
			foundEdge2 = true;

			if(commonFunction == curFunction2)
				lastCommonNodeIndex2 = 0;
		} else
			j2--;

	}

	if(isReachable(curPath[lastCommonNodeIndex1], curSortedPath[lastCommonNodeIndex2], dominates)) {
		return true;
	}

	const SVFGNode *parentNode1 = getSVFG()->getSVFGNode(curPath[lastCommonNodeIndex1].second);

	return false;
}

void SrcSnkAnalysisContext::addToFlowSensSinkPaths(NodeIDPair key, const KSrcSnkDPItem &item) {
	SVFG *svfg = const_cast<SVFG*>(getSVFG()); 
	const SVFGNode *curSrc = getSVFG()->getSVFGNode(key.first);
	bool newPathAdded = false;

	if(fsSinkPaths.empty()) {
		fsSinkPaths.push_back(key);
		return;
	}

	for(int i=0; i < fsSinkPaths.size(); i++) {
		NodeIDPair sortedKey = fsSinkPaths[i];
		const SVFGNode *curSortedNode = getSVFG()->getSVFGNode(sortedKey.first);
		const KSrcSnkDPItem &item2 = sinkPaths.at(sortedKey);

		if(curSrc->getId() == curSortedNode->getId())
			continue;

		std::string path1 = getFuncPathStr(svfg ,item.getCond().getVFEdges(), false); 
		std::string path2 = getFuncPathStr(svfg,item2.getCond().getVFEdges(), false); 

		if(isReachable(item, item2)) {
			auto iter3 = fsSinkPaths.begin();
			std::advance(iter3, i);
			fsSinkPaths.insert(iter3, key);	
			newPathAdded = true;
			if(!isNullStoreNode(curSrc) || isNullStoreNode(curSortedNode) || !isReachable(item2, item))
				break;
			i++;
		}
	}

	if(!newPathAdded)
		fsSinkPaths.push_back(key);
}

void SrcSnkAnalysisContext::sortPaths() {

//	outs() << "Sort paths: " << sinkPaths.size() << "\n";
	for(const auto &iter : sinkPaths) {
		NodeIDPair key = iter.first;
		const KSrcSnkDPItem &item = iter.second;
		const SVFGNode *curSrc = getSVFG()->getSVFGNode(key.first);

		if(!isNullStoreNode(curSrc))
			addToFlowSensSinkPaths(key, item);
	}

	for(const auto &iter : sinkPaths) {
		NodeIDPair key = iter.first;
		const KSrcSnkDPItem &item = iter.second;
		const SVFGNode *curSrc = getSVFG()->getSVFGNode(key.first);

		if(isNullStoreNode(curSrc))
			addToFlowSensSinkPaths(key, item);
	}

//	SVFG *svfg = const_cast<SVFG*>(getSVFG()); 
//	outs() << "===================\n";
//	outs() << "ALL PATHS:\n";
//	for(const auto &iter : sinkPaths) {
//		const SVFGNode *srcNode = getSVFG()->getSVFGNode(iter.second.getLoc()->getId());
//		std::string path = getFuncPathStr(svfg,iter.second.getCond().getVFEdges(), false); 
//		if(isDealloc(srcNode))
//			outs() << "Free= " << path << "\n";
////		else
////			outs() << "Use= " << path << "\n";
//	}
//	outs() << "-------------------\n";
//	for(const auto &iter : fsSinkPaths) {
//		const SVFGNode *srcNode = getSVFG()->getSVFGNode(iter.first);
//		NodeIDPair curKey = iter;
//		const KSrcSnkDPItem &item = sinkPaths.at(curKey);
//		std::string path = getFuncPathStr(svfg,item.getCond().getVFEdges(), false); 
//
//		if(isDealloc(srcNode))
//			outs() << "Free= " << srcNode->getId() << " " << getSVFGSourceLoc(srcNode) << "\n";
////		else if(isUse(srcNode))
////			outs() << "Use= " << srcNode->getId() << " " << getSVFGSourceLoc(srcNode) << "\n";
//		else if(isNull(srcNode))
//			outs() << "Null= " << srcNode->getId() << " " << getSVFGSourceLoc(srcNode) << "\n";
////		}
//	}

//	outs() << "\n";
}

void SrcSnkAnalysisContext::filterUnreachablePaths() {
	for(auto it = sinkPaths.begin(); it != sinkPaths.end();) {
		NodeIDPair key = it->first;	
		const KSrcSnkDPItem &item = it->second;
		const SVFGNode* sink = item.getLoc();

		if(getVFCond(sink) == getFalseCond())
			sinkPaths.erase(it++);	
		else
			++it;			
	}
}


