#ifndef LOCK_FINDER_H
#define LOCK_FINDER_H

#include "Util/KCFSolver.h"
#include "KernelModels/KernelContext.h"
#include "Util/SVFGAnalysisUtil.h"
#include "Util/AnalysisUtil.h"

typedef KCFSolver<SVFG*, CxtDPItem> CxtSolver;

class LockFinder : public CxtSolver {
public:
	typedef std::set<const SVFGNode*> SVFGNodeSet;	
	typedef std::set<CxtDPItem> DPItemSet;	
	typedef std::map<const SVFGNode*, DPItemSet> SVFGNodeToDPItemsMap;

	LockFinder(SVFG *svfg) { 
		setGraph(svfg);	
		kernelCxt = KernelContext::getKernelContext();
	}

	virtual ~LockFinder() { }

	void findLocks(SVFGNodeSet &lockFuncs, SVFGNodeSet &unlockFuncs){
		if(lockObjects.empty()) {
			for(const auto &iter : lockFuncs) {
				ContextCond cxt;
				CxtDPItem item(iter->getId(), cxt);
				backwardTraverse(item);
			}

			for(const auto &iter : unlockFuncs) {
				ContextCond cxt;
				CxtDPItem item(iter->getId(), cxt);
				backwardTraverse(item);
			}
		}
	}

	SVFGNodeSet getLocks() const {
		return lockObjects;
	}

	SVFGNodeSet getLockCxtNodes() const {
		return lockCxtNodes;
	}

protected:
	/**
	 * Checks if the functions is either in the system call or the driver context.
	 */
	bool inAPIContext(std::string funcName) {
		return kernelCxt->inAPIContext(funcName);
	}

	virtual void backwardProcess(const CxtDPItem& item) {
		const SVFGNode *node = getNode(item.getCurNodeID());
		std::string funcName = svfgAnalysisUtil::getSVFGFuncName(node);
		if(isa<AddrSVFGNode>(node) && inAPIContext(funcName))
			lockObjects.insert(node);
		lockCxtNodes.insert(node);
	}

	virtual void backwardpropagate(const CxtDPItem& item, GEDGE* edge) {
		const SVFGNode *srcNode = edge->getSrcNode();
		CxtDPItem newItem(item);
		newItem.setCurNodeID(edge->getSrcID());

		if (edge->isCallVFGEdge()) {
			CallSiteID csId = 0;
			if(const CallDirSVFGEdge* callEdge = dyn_cast<CallDirSVFGEdge>(edge))
				csId = callEdge->getCallSiteId();
			else
				csId = cast<CallIndSVFGEdge>(edge)->getCallSiteId();

			if (newItem.matchContext(csId) == false)
				return;
		} else if (edge->isRetVFGEdge()) {
			CallSiteID csId = 0;
			if(const RetDirSVFGEdge* callEdge = dyn_cast<RetDirSVFGEdge>(edge))
				csId = callEdge->getCallSiteId();
			else
				csId = cast<RetIndSVFGEdge>(edge)->getCallSiteId();
			newItem.pushContext(csId);
		}

		if(backwardVisited(srcNode, newItem))
			return;
			
		addBackwardVisited(srcNode, newItem);

		pushIntoWorklist(newItem);
	}

	bool backwardVisited(const SVFGNode* node, const CxtDPItem& item) {
		SVFGNodeToDPItemsMap::iterator it = nodeToDPItemsMap.find(node);
		if(it!=nodeToDPItemsMap.end())
			return it->second.find(item)!=it->second.end();
		else
			return false;
	}

	void addBackwardVisited(const SVFGNode* node, const CxtDPItem& item) {
		nodeToDPItemsMap[node].insert(item);
	}

	void clearVisitedMap() {
		nodeToDPItemsMap.clear();
	}

private:
	// Contains all nodes that are lock-objects.
	SVFGNodeSet lockObjects;
	SVFGNodeSet lockCxtNodes;

	KernelContext *kernelCxt;

	// Nodes that were backward visited.
	SVFGNodeToDPItemsMap nodeToDPItemsMap;
};


#endif // LOCK_FINDER_H
