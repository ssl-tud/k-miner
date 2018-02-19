// KernelSVFGBuilder.cpp adapted from:
//
//===- KernelSVFGBuilder.cpp -- SVFG builder in Kernel-------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013-2016>  <Yulei Sui>
// Copyright (C) <2013-2016>  <Jingling Xue>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//

#include "KernelModels/KernelSVFGBuilder.h"
#include "Checker/KernelCheckerAPI.h"
#include "SVF/MSSA/SVFG.h"
#include "Util/LockFinder.h"
#include "Util/DebugUtil.h"
#include "Util/SVFGAnalysisUtil.h"

using namespace llvm;
using namespace analysisUtil;
using namespace svfgAnalysisUtil;

static cl::opt<bool> RM_DEREF("rm-deref", cl::init(false),
		cl::desc("Removes the derefences of objects - results in better performance."));

void KernelSVFGBuilder::createSVFG(MemSSA* mssa, SVFG* graph) {

	svfg = graph;
	svfg->buildSVFG(mssa);
	BVDataPTAImpl* pta = mssa->getPTA();
	DBOUT(DGENERAL, outs() << pasMsg("\tCollect Global Variables\n"));

	collectGlobals(pta);

	if(RM_DEREF) {
		DBOUT(DGENERAL, outs() << pasMsg("\tRemove Dereference Direct SVFG Edge\n"));
		rmDerefDirSVFGEdges(pta);
	}

	DBOUT(DGENERAL, outs() << pasMsg("\tAdd Sink SVFG Nodes\n"));

	AddExtActualParmSVFGNodes();

	collectAllocationNodes();
	collectGlobalNodes();
	collectLocalNodes();
	collectUseNodes();
	collectLockNodes();
	collectNullStores();
	collectGlobalStores(pta);

//	if(pta->printStat())
//		svfg->performStat();
	svfg->dump("Kernel_SVFG",true);
}

void KernelSVFGBuilder::collectAllocationNodes() {
	PAG* pag = PAG::getPAG();

	for(const auto &it : pag->getCallSiteRets()) {
		CallSite cs = it.first;
		const Function* caller = cs.getCaller();
		const Function* callee = getCallee(cs);

		if(isPtrInDeadFunction(cs.getInstruction()) && !isAPIFunction(caller))
			continue;

		if(KernelCheckerAPI::getCheckerAPI()->isKAlloc(callee) && inAPIContext(caller)){
			const PAGNode* pagNode = pag->getCallSiteRet(cs);
			const SVFGNode* node = getSVFG()->getDefSVFGNode(pagNode);

			allocNodes.insert(node);
		}
	}

	for(const auto &it : pag->getCallSiteArgsMap()) {
		const Function* fun = getCallee(it.first);
		if(KernelCheckerAPI::getCheckerAPI()->isKDealloc(fun)) {
			const PAG::PAGNodeList& arglist = it.second;
			assert(!arglist.empty() && "no actual parameter at deallocation site?");
			// we only pick the first parameter of all the actual parameters
			const SVFGNode* snk = getSVFG()->getActualParmSVFGNode(arglist.front(),it.first);
			deallocNodes.insert(snk);
		}
	}
}

void KernelSVFGBuilder::collectLockNodes() {
	LockFinder *lf = new LockFinder(getSVFG());
	PAG* pag = PAG::getPAG();

	for(const auto &it : pag->getCallSiteArgsMap()) {
		const Function* fun = getCallee(it.first);

		if(KernelCheckerAPI::getCheckerAPI()->isKLock(fun)) {
			const PAG::PAGNodeList& arglist = it.second;
			assert(!arglist.empty() && "no actual parameter at deallocation site?");

			const SVFGNode* node = getSVFG()->getActualParmSVFGNode(arglist.front(),it.first);
			lockNodes.insert(node);
		} else if(KernelCheckerAPI::getCheckerAPI()->isKUnlock(fun)) {
			const PAG::PAGNodeList& arglist = it.second;
			assert(!arglist.empty() && "no actual parameter at deallocation site?");

			const SVFGNode* node = getSVFG()->getActualParmSVFGNode(arglist.front(),it.first);
			unlockNodes.insert(node);
		}
	}

	lf->findLocks(lockNodes, unlockNodes);
	lockObjNodes = lf->getLocks();

	delete lf;
}

void KernelSVFGBuilder::collectGlobalNodes() {
	PAG* pag = PAG::getPAG();

	for(auto pagIter = pag->begin(); pagIter != pag->end(); ++pagIter) {
		const PAGNode* pagNode = pagIter->second;
		
		if (!isa<DummyValPN>(pagNode) && !isa<DummyObjPN>(pagNode) && !isa<GepObjPN>(pagNode)) {
			if(const ObjPN* objPN = llvm::dyn_cast<ObjPN>(pagNode)) {
				const MemObj* memObj = objPN->getMemObj();
				if(memObj->isGlobalObj()) 
					globalNodes.insert(getSVFG()->getDefSVFGNode(pagNode));
				if(memObj->isHeap()) 
					globalNodes.insert(getSVFG()->getDefSVFGNode(pagNode));
			}
		} 
	}
}

void KernelSVFGBuilder::collectLocalNodes() {
	PAG* pag = PAG::getPAG();

	for(auto pagIter = pag->begin(); pagIter != pag->end(); ++pagIter) {
		const PAGNode* pagNode = pagIter->second;
		
		if (!isa<DummyValPN>(pagNode) && !isa<DummyObjPN>(pagNode) && !isa<GepObjPN>(pagNode)) {
			if(const ObjPN* objPN = llvm::dyn_cast<ObjPN>(pagNode)) {
				const MemObj* memObj = objPN->getMemObj();
				if(memObj->isStack()) 
					localNodes.insert(getSVFG()->getDefSVFGNode(pagNode));
			}
		} 
	}
}

void KernelSVFGBuilder::collectUseNodes() {
	SVFGNodeSet visitedParentNodes;

	for(SVFG::iterator it = svfg->begin(), eit = svfg->end(); it!=eit; ++it) {
		const SVFGNode* node = it->second;

		if(!node->hasOutgoingEdge() && visitedParentNodes.find(node) == visitedParentNodes.end()) {
			if(isa<LoadSVFGNode>(node) || isa<GepSVFGNode>(node)) {
				if(isa<GepSVFGNode>(node))
					node = (*node->getInEdges().begin())->getSrcNode();
				useNodes.insert(node);

				for(const auto &iter : node->getInEdges())
					visitedParentNodes.insert(iter->getSrcNode());
			} 
		}
	}
}

void KernelSVFGBuilder::collectNullStores() {
	const SVFGNode *nullNode = svfg->getSVFGNode(0);

	for(const auto &iter : nullNode->getOutEdges()) {
		nullStoreNodes.insert(iter->getDstNode());
	}
}

void KernelSVFGBuilder::collectGlobalStores(BVDataPTAImpl* pta) {
	if(globalStoreNodes.size() > 0)
		return;

	for(SVFG::iterator it = svfg->begin(), eit = svfg->end(); it!=eit; ++it) {
		const SVFGNode* node = it->second;

		if(const StmtSVFGNode* stmtNode = dyn_cast<StmtSVFGNode>(node)) {
			if(isa<StoreSVFGNode>(stmtNode)) {
				if(accessGlobal(pta,stmtNode->getPAGDstNode())) {
					globalStoreNodes.insert(stmtNode);
				}
			} else if(isa<LoadSVFGNode>(stmtNode)) {
				if(accessGlobal(pta,stmtNode->getPAGSrcNode())) {
					globalStoreNodes.insert(stmtNode);
				}
			}
		}
	}
}

void KernelSVFGBuilder::collectGlobals(BVDataPTAImpl* pta) {
	PAG* pag = svfg->getPAG();
	NodeVector worklist;
	for(PAG::iterator it = pag->begin(), eit = pag->end(); it!=eit; it++) {
		PAGNode* pagNode = it->second;
		if(isa<DummyValPN>(pagNode) || isa<DummyObjPN>(pagNode))
			continue;
		if(const Value* val = pagNode->getValue()) {
			if(isa<GlobalVariable>(val)) {
				worklist.push_back(it->first);
			}
		}
	}

	NodeToPTSSMap cachedPtsMap;
	while(!worklist.empty()) {
		NodeID id = worklist.back();
		worklist.pop_back();
		globs.set(id);
		PointsTo& pts = pta->getPts(id);
		for(PointsTo::iterator it = pts.begin(), eit = pts.end(); it!=eit; ++it) {
			globs |= CollectPtsChain(pta,*it,cachedPtsMap);
		}
	}
}

NodeBS& KernelSVFGBuilder::CollectPtsChain(BVDataPTAImpl* pta,NodeID id, NodeToPTSSMap& cachedPtsMap) {
	PAG* pag = svfg->getPAG();

	NodeID baseId = pag->getBaseObjNode(id);
	NodeToPTSSMap::iterator it = cachedPtsMap.find(baseId);
	if(it!=cachedPtsMap.end())
		return it->second;
	else {
		PointsTo& pts = cachedPtsMap[baseId];
		pts |= pag->getFieldsAfterCollapse(baseId);

		WorkList worklist;
		for(PointsTo::iterator it = pts.begin(), eit = pts.end(); it!=eit; ++it)
			worklist.push(*it);

		while(!worklist.empty()) {
			NodeID nodeId = worklist.pop();
			PointsTo& tmp = pta->getPts(nodeId);
			for(PointsTo::iterator it = tmp.begin(), eit = tmp.end(); it!=eit; ++it) {
				pts |= CollectPtsChain(pta,*it,cachedPtsMap);
			}
		}
		return pts;
	}

}

bool KernelSVFGBuilder::accessGlobal(BVDataPTAImpl* pta,const PAGNode* pagNode) {

	NodeID id = pagNode->getId();
	PointsTo pts = pta->getPts(id);
	pts.set(id);

	return pts.intersects(globs);
}

void KernelSVFGBuilder::rmDerefDirSVFGEdges(BVDataPTAImpl* pta) {

	for(SVFG::iterator it = svfg->begin(), eit = svfg->end(); it!=eit; ++it) {
		const SVFGNode* node = it->second;

		if(const StmtSVFGNode* stmtNode = dyn_cast<StmtSVFGNode>(node)) {
			/// for store, connect the RHS/LHS pointer to its def
			if(isa<StoreSVFGNode>(stmtNode)) {
				const SVFGNode* def = svfg->getDefSVFGNode(stmtNode->getPAGDstNode());
				SVFGEdge* edge = svfg->getSVFGEdge(def,stmtNode,SVFGEdge::IntraDirect);
				assert(edge && "Edge not found!");
				svfg->removeSVFGEdge(edge);

				if(accessGlobal(pta,stmtNode->getPAGDstNode())) {
					globalStoreNodes.insert(stmtNode);
				} 
			} else if(isa<LoadSVFGNode>(stmtNode)) {
				const SVFGNode* def = svfg->getDefSVFGNode(stmtNode->getPAGSrcNode());
				SVFGEdge* edge = svfg->getSVFGEdge(def,stmtNode,SVFGEdge::IntraDirect);
				assert(edge && "Edge not found!");
				svfg->removeSVFGEdge(edge);

				if(accessGlobal(pta,stmtNode->getPAGSrcNode())) {
					globalStoreNodes.insert(stmtNode);
				}
			}

		}
	}
}


void KernelSVFGBuilder::AddExtActualParmSVFGNodes() {
	PAG* pag = PAG::getPAG();
	for(PAG::CSToArgsListMap::iterator it = pag->getCallSiteArgsMap().begin(),
			eit = pag->getCallSiteArgsMap().end(); it!=eit; ++it) {
		const Function* fun = getCallee(it->first);
		if(KernelCheckerAPI::getCheckerAPI()->isKDealloc(fun)
				|| KernelCheckerAPI::getCheckerAPI()->isKLock(fun)
				|| KernelCheckerAPI::getCheckerAPI()->isKUnlock(fun)) {
			PAG::PAGNodeList& arglist =	it->second;
			const PAGNode* pagNode = arglist.front();
			svfg->addActualParmSVFGNode(pagNode,it->first);
			svfg->addIntraDirectVFEdge(svfg->getDefSVFGNode(pagNode)->getId(),svfg->getActualParmSVFGNode(pagNode,it->first)->getId());
		}
	}
}

