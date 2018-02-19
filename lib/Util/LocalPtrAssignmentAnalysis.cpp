#include "SABER/LocalPtrAssignmentAnalysis.h"

using namespace llvm;
using namespace svfgAnalysisUtil;

const SVFGNode* LPAAnalysis::findRecentAssignment(const SVFGNode *localPtr, const SVFGNode *recentNode) {
	ContextCond cxt;
	// TODO: Create an own item class for this analysis.
	ScopeDPItem item(localPtr->getId(), cxt, localPtr);
	initWorklist(item, recentNode);
	setCurSlice(localPtr);
	// Can't be parallelized, because the order of the items in the worklist is important.
	forwardTraverse(item);
	return findSrcOfStoreNode();
}

const SVFGNode* LPAAnalysis::findSrcOfStoreNode() {
	const SVFGNode *storeNode = _curSlice->getStoreNode();
	ContextCond cxt;
	ScopeDPItem item(storeNode->getId(), cxt, storeNode);
	backwardTraverse(item);	
	return _curSlice->getGlobalNode();
}

const SVFGNode* LPAAnalysis::findSrcOfStoreNode(const SVFGNode *storeNode, const ScopeDPItem &item) {
	setCurSlice(storeNode);
	_curSlice->setVisitedFormalParams(item.getVisitedFormalParams());
	_curSlice->setStoreNode(getNode(item.getCurNodeID()));
	return findSrcOfStoreNode();
}

void LPAAnalysis::initWorklist(ScopeDPItem &item, const SVFGNode *recentNode) {
	const SVFGNode *localPtr = item.getRoot();
	SVFGEdgeList edgeList;

	// Collect the destination nodes of all the edges. (without the recentNode)
	for(auto edgeIter = localPtr->OutEdgeBegin(); edgeIter != localPtr->OutEdgeEnd(); ++edgeIter) {
		const SVFGEdge *edge = *edgeIter;
		const SVFGNode *dstNode = edge->getDstNode();
		
		if(!recentNode || dstNode->getId() != recentNode->getId())
			edgeList.push_back(*edgeIter);	
	}

	// Sorts the list of destination nodes in the order they are accessed in the program. 
	sortSVFGEdgeList(edgeList, recentNode);

	// Create the items to the corresponding destination nodes and push them to the worklist. 
	// If it is a functionEntry (formal parameter) handle it the correct way.
	for(auto edgeIter = edgeList.rbegin(); edgeIter != edgeList.rend(); ++edgeIter) {
		const SVFGEdge *edge = *edgeIter;
		const SVFGNode *dstNode = edge->getDstNode();
		ScopeDPItem newItem(item);
		newItem.setCurNodeID(dstNode->getId());
		newItem.setPrevNodeID(item.getCurNodeID());

		if (edge->isCallVFGEdge())
			handleFuncEntry(newItem, edge);

		// Is this possible?
		assert(edge->isRetVFGEdge() && "A local pointer was returned directly");

		pushIntoWorklist(newItem);
	}
}

bool LPAAnalysis::compare_inst_first(const SVFGEdge *edge1, const SVFGEdge *edge2) {
	const SVFGNode *dstNode1 = edge1->getDstNode();
	const SVFGNode *dstNode2 = edge2->getDstNode();
	const llvm::Function *srcFunc = edge1->getSrcNode()->getBB()->getParent();
	DominatorTree *dt = CFBuilder.getDT(srcFunc);
	LoopInfo *li = CFBuilder.getLoopInfo(srcFunc);
	const llvm::Instruction *inst1 = getInstruction(dstNode1);
	const llvm::Instruction *inst2 = getInstruction(dstNode2);
	
	return isPotentiallyReachable(inst1, inst2, dt, li);
}

void LPAAnalysis::sortSVFGEdgeList(SVFGEdgeList &edgeList, const SVFGNode *recentNode) {
	const llvm::Instruction *recentInst = getInstruction(recentNode);
	const SVFGEdge *firstEdge = edgeList.front();
	const llvm::Function *srcFunc = firstEdge->getSrcNode()->getBB()->getParent();
	DominatorTree *dt = CFBuilder.getDT(srcFunc);
	LoopInfo *li = CFBuilder.getLoopInfo(srcFunc);

	// Removes all edges with destination nodes which are after the recentNode.
	if(recentNode) {
		for(auto edgeIter = edgeList.begin(); edgeIter != edgeList.end(); ++edgeIter) {
			const SVFGEdge *edge = *edgeIter;
			const SVFGNode *dstNode = edge->getDstNode();
			const llvm::Instruction *curInst = getInstruction(dstNode);

			if(!isPotentiallyReachable(curInst, recentInst, dt, li))
				edgeIter = edgeList.erase(edgeIter);
		}
	}

//	edgeList.sort(compare_inst_first);
	edgeList.sort([&](const SVFGEdge *edge1, const SVFGEdge *edge2) {
		const SVFGNode *dstNode1 = edge1->getDstNode();
		const SVFGNode *dstNode2 = edge2->getDstNode();
		const llvm::Function *srcFunc = edge1->getSrcNode()->getBB()->getParent();
		DominatorTree *dt = CFBuilder.getDT(srcFunc);
		LoopInfo *li = CFBuilder.getLoopInfo(srcFunc);
		const llvm::Instruction *inst1 = getInstruction(dstNode1);
		const llvm::Instruction *inst2 = getInstruction(dstNode2);
		
		return isPotentiallyReachable(inst1, inst2, dt, li);
		});
}

//TODO: outsource
const llvm::Instruction* LPAAnalysis::getInstruction(const SVFGNode* node) {
	if(const StmtSVFGNode* stmtNode = dyn_cast<StmtSVFGNode>(node))
		return stmtNode->getPAGEdge()->getInst(); 

	if(const InterPHISVFGNode* phiNode = dyn_cast<InterPHISVFGNode>(node))
		return phiNode->getCallInst();

	return nullptr;
}

void LPAAnalysis::handleFuncEntry(ScopeDPItem &funcEntryItem, const SVFGEdge *edge) {
	const SVFGNode* curNode = edge->getSrcNode();
	const SVFGNode* nextNode = edge->getDstNode();
	CallSiteID csId = 0;

	if(const CallDirSVFGEdge* callEdge = dyn_cast<CallDirSVFGEdge>(edge))
		csId = callEdge->getCallSiteId();
	else
		csId = cast<CallIndSVFGEdge>(edge)->getCallSiteId();

	// Stores the InterPHISVFGNode (Formal Parameter) and the index of the corresponding PAGSrcNode.
	// This has to be handled for each formal parameter we reach, because we can't merge them laiter.
	if(const InterPHISVFGNode *phiNode = dyn_cast<InterPHISVFGNode>(nextNode)) {
		if(phiNode->isFormalParmPHI()) {
			if(const StmtSVFGNode *stmtNode = dyn_cast<StmtSVFGNode>(curNode)) {
				const PAGNode *curPagDstNode = stmtNode->getPAGDstNode();
				int i=0;
				
				for(auto iter = phiNode->opVerBegin(); 
						iter != phiNode->opVerEnd(); ++iter, i++) {
					if(curPagDstNode->getId() == iter->second->getId()) {
						funcEntryItem.pushVisitedFormalParam(phiNode, i);
						break;
					}
				}
			}
		}
	}
}

void LPAAnalysis::forwardProcess(const ScopeDPItem& item) {
	_curSlice->addToForwardSlice(getNode(item.getCurNodeID()));
}

void LPAAnalysis::forwardTraverse(ScopeDPItem& it) {
	// Can't be parallelized, because the order of the items in the worklist is important.
	while (!isWorklistEmpty()) {
		ScopeDPItem item = popFromWorklist();

		if(!forwardChecks(item))
			continue;

		forwardProcess(item);

		const SVFGNode *curNode = getNode(item.getCurNodeID()); 

		if(isa<StoreSVFGNode>(curNode) && !isParamStore(curNode)) {
			// We found the store node we were looking for.
			_curSlice->setVisitedFormalParams(item.getVisitedFormalParams());
			_curSlice->setStoreNode(getNode(item.getCurNodeID()));
			break;
		}

		GNODE* v = getNode(getNodeIDFromItem(item));
		child_iterator EI = GTraits::child_begin(v);
		child_iterator EE = GTraits::child_end(v);
		for (; EI != EE; ++EI) {
			forwardpropagate(item,*(EI.getCurrent()) );
		}
	}
}

bool LPAAnalysis::forwardChecks(ScopeDPItem &item) {
	const SVFGNode* curNode = getNode(item.getCurNodeID());
	const SVFGNode* prevNode = getNode(item.getPrevNodeID());
	const StmtSVFGNode *curStmtNode = dyn_cast<StmtSVFGNode>(curNode);
	const StmtSVFGNode *prevStmtNode = dyn_cast<StmtSVFGNode>(prevNode);

	// The node was already forwarded.
	if(_curSlice->inForwardSlice(curNode))
		return false;

	if(isa<StoreSVFGNode>(curNode)) {
		const SVFGNode *svfgSrcNode = getSVFGNodeDef(graph(), curNode);

		// The Ptr is set to NULL.
		if(svfgSrcNode->getId() == 0)
			return false;
	}

	// We are searching for an assignment of our local pointer and 
	// not for one of its structure elements.
	if(isa<GepSVFGNode>(curNode))
		return false;

	// only StmtSVFGNodes to MSSAPHISVFGNodes is valid.
	if(isa<MSSAPHISVFGNode>(curNode) && !isa<StmtSVFGNode>(prevNode))
		return false;

	if(curStmtNode && prevStmtNode) {
		const PAGNode *curPagSrcNode = curStmtNode->getPAGSrcNode();
		const PAGNode *curPagDstNode = curStmtNode->getPAGDstNode();
		const PAGNode *prevPagSrcNode = prevStmtNode->getPAGSrcNode();
		const PAGNode *prevPagDstNode = prevStmtNode->getPAGDstNode();

		if(isa<StoreSVFGNode>(curNode)) {
			if(isa<LoadSVFGNode>(prevNode))
				if(prevPagDstNode->getId() != curPagDstNode->getId())
					return false;
		}

	}

	return true;
}

void LPAAnalysis::forwardpropagate(const ScopeDPItem& item, SVFGEdge* edge) {
        ScopeDPItem newItem(item);
        newItem.setCurNodeID(edge->getDstID());
	newItem.setPrevNodeID(item.getCurNodeID());

	/// perform context sensitive reachability
	// push context for calling
	if (edge->isCallVFGEdge()) {
		CallSiteID csId = 0;

		if(const CallDirSVFGEdge* callEdge = dyn_cast<CallDirSVFGEdge>(edge))
			csId = callEdge->getCallSiteId();
		else
			csId = cast<CallIndSVFGEdge>(edge)->getCallSiteId();

		newItem.pushContext(csId);
	}
	// match context for return
	else if (edge->isRetVFGEdge()) {
		CallSiteID csId = 0;

		if(const RetDirSVFGEdge* callEdge = dyn_cast<RetDirSVFGEdge>(edge))
			csId = callEdge->getCallSiteId();
		else
			csId = cast<RetIndSVFGEdge>(edge)->getCallSiteId();

		if (!newItem.matchContext(csId))
			return;
	}

        pushIntoWorklist(newItem);
}

void LPAAnalysis::backwardProcess(const ScopeDPItem& item) {
	const SVFGNode *curNode = getNode(item.getCurNodeID());
	_curSlice->addToBackwardSlice(curNode);
}

void LPAAnalysis::backwardTraverse(ScopeDPItem& it) {
	pushIntoWorklist(it);

	while (!isWorklistEmpty()) {
		ScopeDPItem item = popFromWorklist();

		if(!backwardChecks(item))
			continue;

		backwardProcess(item);

		const SVFGNode *curNode = getNode(item.getCurNodeID());

		if(isa<AddrSVFGNode>(curNode)) {
			auto globalNodesIter = globalAddrSVFGNodes.find(curNode);
			auto localNodesIter = localAddrSVFGNodes.find(curNode);

			// We found the global variable we were looking for
			if(globalNodesIter != globalAddrSVFGNodes.end()) {
				_curSlice->setGlobalNode(*globalNodesIter);
				break;
			} else if(localNodesIter != localAddrSVFGNodes.end()) {
				// We found a local pointer and we have to check if it points to a global variable.
				if(isPointer(*localNodesIter)) {
					const SVFGNode *recentNode = graph()->getSVFGNode(item.getPrevNodeID());
					findRecentAssignment(*localNodesIter, recentNode); 
					break;
				}
			}
		}

		GNODE* v = getNode(getNodeIDFromItem(item));
		inv_child_iterator EI = InvGTraits::child_begin(v);
		inv_child_iterator EE = InvGTraits::child_end(v);

		for (; EI != EE; ++EI) {
			backwardpropagate(item,*(EI.getCurrent()) );
		}
	}
}

bool LPAAnalysis::backwardChecks(ScopeDPItem &item) {
	const SVFGNode *curNode = getNode(item.getCurNodeID());
	const SVFGNode *prevNode = getNode(item.getPrevNodeID());
	const StmtSVFGNode *curStmtNode = dyn_cast<StmtSVFGNode>(curNode);
	const StmtSVFGNode *prevStmtNode = dyn_cast<StmtSVFGNode>(prevNode);
	const InterPHISVFGNode *curPhiNode = dyn_cast<InterPHISVFGNode>(curNode);

	// The node was forwarded and we are searching for another path.
	if(_curSlice->inForwardSlice(curNode) || _curSlice->inBackwardSlice(curNode))
		return false;

	if(isa<NullPtrSVFGNode>(curNode)) 
		return false;

	// The Ptr is set to NULL.
	if(isa<StoreSVFGNode>(curNode)) {
		const SVFGNode *svfgSrcNode = getSVFGNodeDef(graph(), curNode);

		if(svfgSrcNode->getId() == 0)
			return false;
	} 

	if(curStmtNode && prevStmtNode) {
		const PAGNode *curPagSrcNode = curStmtNode->getPAGSrcNode();
		const PAGNode *curPagDstNode = curStmtNode->getPAGDstNode();
		const PAGNode *prevPagSrcNode = prevStmtNode->getPAGSrcNode();
		const PAGNode *prevPagDstNode = prevStmtNode->getPAGDstNode();

		if(isa<StoreSVFGNode>(curNode) && isa<StoreSVFGNode>(prevNode)) {
			if(!isParamStore(curNode))
				return false;
		} else if(isa<StoreSVFGNode>(curNode) && isa<LoadSVFGNode>(prevNode)) {
			if(curPagDstNode->getId() != prevPagSrcNode->getId() && !isParamStore(curNode))
				return false;
		} else if(isa<GepSVFGNode>(curNode) && isa<LoadSVFGNode>(prevNode)) {
			if(curPagDstNode->getId() != prevPagSrcNode->getId())
				return false;
		}
	} 
	
	// We backwarded over a InterPHISVFGNode and now have to check if the next node belongs to
	// the correct path. The DstPAGNode of the current SVFGNode has to match with the OpVers PAGNode at
	// the index of pos. This is the same index of the corresponding param we visited during forwarding.
	if(const InterPHISVFGNode *prevPhiNode = dyn_cast<InterPHISVFGNode>(prevNode)) {
		// It is a Return Node so move on.
		if(prevPhiNode->isActualRetPHI())
			return true;

		// There are no further parameter nodes we visited during forwarding. We
		// have to check all pathes that reach this formal parameter node.
		if(item.isVisitedFormalParamEmpty())
			return true;

		// Is this possible?
		assert((curStmtNode || curPhiNode) && 
				"The node before the InterPHISVFGNode is not a StmtNode nor a PhiNode");

		const PAGNode *curPagDstNode = curStmtNode ? curStmtNode->getPAGDstNode() : curPhiNode->getRes();
		const FormalParamToPos paramToPos = item.popVisitedFormalParam(); 
		const InterPHISVFGNode *visitedPhiNode = paramToPos.first;
		u32_t pos = paramToPos.second;

		assert(prevPhiNode->getId() == visitedPhiNode->getId() && "backwarded the wrong path");

		// We check if the function of the current Formal Parameter matches with the 
		// function of the corresponding FormalParameter we visited during forwarding. 
		// The DstPAGNode of the current SVFGNode has to match with the OpVers 
		// PAGNode at the index of pos. This is the same index of the corresponding 
		// param we visited during forwarding.
		if(prevPhiNode->getFun()->getName() == visitedPhiNode->getFun()->getName()) {
			const PAGNode* nextValidPagNode = prevPhiNode->getOpVer(pos);

			if(curPagDstNode->getId() == nextValidPagNode->getId())
				return true;
		}

		// The current SVFGNode don't belong to our path of execution but maybe on of the next nodes
		// we do, so we have to push the parameter and the index back to the list.
		item.pushVisitedFormalParam(paramToPos);
			
		return false;
	}

	return true;
}

void LPAAnalysis::backwardpropagate(const ScopeDPItem& item, SVFGEdge* edge) {
        ScopeDPItem newItem(item);
        newItem.setCurNodeID(edge->getSrcID());

	if(isa<CallIndSVFGEdge>(edge))
		return;

        pushIntoWorklist(newItem);
}

void LPAAnalysis::convertPAGNodesToSVFGNodes(PAGNodeSet &pagNodes, SVFGNodeSet &svfgNodes) {
	const SVFGNode *svfgNode;

	for(auto pagNodesIter = pagNodes.begin(); pagNodesIter != pagNodes.end(); ++pagNodesIter) {
//		svfgNode = svfg->getDefSVFGNode(*pagNodesIter);	
		svfgNode = graph()->getDefSVFGNode(*pagNodesIter);
		svfgNodes.insert(svfgNode);
	}
}

void LPAAnalysis::setCurSlice(const SVFGNode* src) {
	if(_curSlice!=nullptr) {
		delete _curSlice;
		_curSlice = nullptr;
	}

	_curSlice = new LPAAProgSlice(src, pathCondAllocator, graph());
}
