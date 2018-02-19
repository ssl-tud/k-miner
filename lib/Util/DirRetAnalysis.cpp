#include "Util/DirRetAnalysis.h"
#include "Util/DebugUtil.h"

using namespace llvm;

const SVFGNode* DirRetAnalysis::findDstOfReturn(const SVFGNode *srcNode) {
	setCurAnalysisCxt(srcNode);
	
	findAssignment(srcNode);

	findDst();

	return dra_curAnalysisCxt->getDstNode();
}

void DirRetAnalysis::findAssignment(const SVFGNode *srcNode) {
	ContextCond cxt;
	CxtTraceDPItem item(srcNode->getId(), cxt, srcNode, 0, false, false); 
	item.addToVisited(0, srcNode->getId());

	forwardTraverse(item);
}

void DirRetAnalysis::findDst() {
	const SVFGNode* assignmentNode = dra_curAnalysisCxt->getAssignmentNode();
	ContextCond cxt;
	CxtTraceDPItem item(assignmentNode->getId(), cxt, assignmentNode, 0, false, false); 

	backwardTraverse(item);
}

void DirRetAnalysis::handleAssignment(const SVFGNode* node, const CxtTraceDPItem &item) {
	dra_curAnalysisCxt->setAssignmentNode(node);
	dra_curAnalysisCxt->setForwardPath(item.getVisitedPath());
	dra_curAnalysisCxt->setForwardSlice(item.getCxtVisitedNodes());
}

void DirRetAnalysis::handleDst(const SVFGNode* node, const CxtTraceDPItem &item) {
	dra_curAnalysisCxt->setDstNode(node);
	dra_curAnalysisCxt->setBackwardPath(item.getVisitedPath());
	dra_curAnalysisCxt->setBackwardSlice(item.getCxtVisitedNodes());
}

inline void DirRetAnalysis::forwardProcess(const CxtTraceDPItem &item) {
	const SVFGNode* curNode = getNode(item.getCurNodeID());

	// This will be the first assignment. Now we can backward starting from this node.
	if(isa<StoreSVFGNode>(curNode))
		handleAssignment(curNode, item);
}

void DirRetAnalysis::forwardpropagate(const CxtTraceDPItem &item, GEDGE *edge) {
	const SVFGNode* curNode = getNode(item.getCurNodeID());
	const SVFGNode* nextNode = edge->getDstNode();
	CallSiteID csId = 0;
	CxtTraceDPItem newItem(item);
	newItem.setCurNodeID(edge->getDstID());
	newItem.setPrevNodeID(item.getCurNodeID());

	// We only forward to the first assignment.
	if(isa<StoreSVFGNode>(curNode)) {
		clearWorklist();
		return;
	}

	if (edge->isCallVFGEdge()) {
		if(const CallDirSVFGEdge* callEdge = dyn_cast<CallDirSVFGEdge>(edge))
			csId = callEdge->getCallSiteId();
		else
			csId = cast<CallIndSVFGEdge>(edge)->getCallSiteId();

		newItem.pushContext(csId);
	} else if (edge->isRetVFGEdge()) {
		if(const RetDirSVFGEdge* callEdge = dyn_cast<RetDirSVFGEdge>(edge))
			csId = callEdge->getCallSiteId();
		else
			csId = cast<RetIndSVFGEdge>(edge)->getCallSiteId();

		if (newItem.matchContext(csId) == false)
			return;

		// Replace all unkown contexts(=0) with the given one.
		// Has to be before the current node is added to the visitedset,
		// because this node will be in another context.
		newItem.resolveUnkownCxts(csId);
	}
	
	if(newItem.hasVisited(item.getCurCxt(), nextNode->getId()))
		return;
	
	newItem.addToVisited(item.getCurCxt(), nextNode->getId());

	pushIntoWorklist(newItem);
}

inline void DirRetAnalysis::backwardProcess(const CxtTraceDPItem &item) {
	const SVFGNode* curNode = getNode(item.getCurNodeID());

	if(isa<AddrSVFGNode>(curNode)) {
		handleDst(curNode, item);
		clearWorklist();
	}
}

void DirRetAnalysis::backwardpropagate(const CxtTraceDPItem& item, GEDGE* edge) {
	const SVFGNode* curNode = edge->getDstNode();
	const SVFGNode* nextNode = edge->getSrcNode();
	CallSiteID csId;

	CxtTraceDPItem newItem(item);
	newItem.setCurNodeID(edge->getSrcID());
	newItem.setPrevNodeID(item.getCurNodeID());

	if (edge->isCallVFGEdge()) {
		if(const CallDirSVFGEdge* callEdge = dyn_cast<CallDirSVFGEdge>(edge))
			csId = callEdge->getCallSiteId();
		else
			csId = cast<CallIndSVFGEdge>(edge)->getCallSiteId();

		if(!newItem.matchContext(csId))
			return;

	} else if (edge->isRetVFGEdge()) {
		if(const RetDirSVFGEdge* callEdge = dyn_cast<RetDirSVFGEdge>(edge))
			csId = callEdge->getCallSiteId();
		else
			csId = cast<RetIndSVFGEdge>(edge)->getCallSiteId();

		newItem.pushContext(csId);
	}

	csId = newItem.getCurCxt();

	if(dra_curAnalysisCxt->inForwardSlice(csId, nextNode->getId()))
		return;

	// If the current path contains the next node, we wont backward it. 
	if(newItem.hasVisited(csId, nextNode->getId()))
		return;

	newItem.addToVisited(csId, nextNode->getId());

	pushIntoWorklist(newItem);
}

void DirRetAnalysis::setCurAnalysisCxt(const SVFGNode* src) {
	if(dra_curAnalysisCxt!=nullptr) {
		delete dra_curAnalysisCxt;
		dra_curAnalysisCxt = nullptr;
	}

	dra_curAnalysisCxt = new DRAAnalysisContext(src, getGraph());
}
