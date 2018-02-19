#ifndef ANALYSIS_CONTEXT_H
#define ANALYSIS_CONTEXT_H

#include "Util/DebugUtil.h"
#include "Util/SVFGAnalysisUtil.h"
#include "SVF/MSSA/SVFG.h"
#include "SVF/SABER/ProgSlice.h"
#include "SVF/Util/WorkList.h"
#include <algorithm>   
#include <omp.h>

class AnalysisContext: public ProgSlice {
public:
	typedef std::pair<NodeID, NodeID> NodeIDPair;
	typedef std::list<std::string> StringList;
	typedef std::vector<NodeID> NodeIDVec;
	typedef std::pair<NodeIDPair, NodeIDPair> KeyPair;
	typedef std::list<KeyPair> KeyPairList;
	typedef std::vector<NodeIDPair> NodeIDPairVec;
	typedef std::map<unsigned int, NodeIDPairVec> IntToNodeIDPairVec;
	typedef std::map<const SVFGNode*, unsigned int> SVFGNodeCnt;
	typedef std::set<const SVFGNode*> SVFGNodeSet;

	AnalysisContext(const SVFGNode* src, PathCondAllocator *pa, const SVFG* svfg)
		: ProgSlice(src, pa, svfg), reachNull(0), start_t(omp_get_wtime()){ }

	virtual ~AnalysisContext() { }

	void setReachNull() {
		reachNull = true;
	}

	bool isReachNull() const {
		return reachNull;
	}

	bool isCall(const SVFGNode *src, const SVFGNode *dst) {
		return getSVFG()->getSVFGEdge(src, dst, SVFGEdge::SVFGEdgeK::DirCall) ||
			getSVFG()->getSVFGEdge(src, dst, SVFGEdge::SVFGEdgeK::IndCall);
	}	

	bool isRet(const SVFGNode *src, const SVFGNode *dst) {
		return getSVFG()->getSVFGEdge(src, dst, SVFGEdge::SVFGEdgeK::DirRet) ||
			getSVFG()->getSVFGEdge(src, dst, SVFGEdge::SVFGEdgeK::IndRet);
	}	

	bool isCall(NodeID src, NodeID dst) {
		const SVFGNode *srcNode = getSVFG()->getSVFGNode(src);
		const SVFGNode *dstNode = getSVFG()->getSVFGNode(dst);
		return isCall(srcNode, dstNode);
	}

	bool isRet(NodeID src, NodeID dst) {
		const SVFGNode *srcNode = getSVFG()->getSVFGNode(src);
		const SVFGNode *dstNode = getSVFG()->getSVFGNode(dst);
		return isRet(srcNode, dstNode);
	}

	const llvm::Instruction* getInstruction(const SVFGNode *node1, const SVFGNode *node2) {
		const llvm::Instruction *inst = nullptr;
		const SVFG *svfg = getSVFG();

		// Has to be checked first, because SVFGOPT might create something like: 
		// ENCHI --> LOAD and not always ENCHI --> ENCHI --> LOAD
		if(const SVFGEdge * edge = svfg->getSVFGEdge(node1, node2, SVFGEdge::SVFGEdgeK::IndCall)) {
			const CallIndSVFGEdge *callEdge = llvm::dyn_cast<CallIndSVFGEdge>(edge);
			inst = svfg->getCallSite(callEdge->getCallSiteId()).getInstruction();
		} else if(const SVFGEdge * edge = svfg->getSVFGEdge(node1, node2, SVFGEdge::SVFGEdgeK::DirCall)) {
			const CallDirSVFGEdge *callEdge = llvm::dyn_cast<CallDirSVFGEdge>(edge);
			inst = svfg->getCallSite(callEdge->getCallSiteId()).getInstruction();
		} else if(llvm::isa<StmtSVFGNode>(node2)) {
			inst = llvm::dyn_cast<StmtSVFGNode>(node2)->getInst();
		} else if(llvm::isa<ActualParmSVFGNode>(node2)) {
			inst = llvm::dyn_cast<ActualParmSVFGNode>(node2)->getCallSite().getInstruction();
		} //TODO Ret 

		return inst;
	}

	/**
	 * Checks if node1 may reaches node2.
	 */
	bool isReachable(const NodeIDPair &edge1, const NodeIDPair &edge2, bool dominates=false);

	/**
	 * Checks if item1 may reaches item2.
	 */
	bool isReachable(const KSrcSnkDPItem &item1, const KSrcSnkDPItem &item2, bool dominates=false);

	const StringList& getAPIPath() const {
		return apiPath;
	}

	void setAPIPath(const StringList &path) {
		apiPath = path;
	}

	double getStartTime() const {
		return start_t;
	}

private:
	PTACFInfoBuilder CFBuilder;
	bool reachNull;

	// Path from the systemcall to the local variable.
	StringList apiPath;

	double start_t;

};

/**
 * An extended AnalysisContext to capture dangling pointer.
 */
class UARLiteAnalysisContext : public AnalysisContext {
public:
	typedef std::list<CxtNodeIdVar> CxtNodeIdList;
	typedef std::map<NodeID, ContextCond> ContextCondMap;

	UARLiteAnalysisContext(const SVFGNode* src, const SVFG* graph)
		: AnalysisContext(src, nullptr, graph) {
	}

	virtual ~UARLiteAnalysisContext() { 
		//TODO
	}

	bool addOosNode(const SVFGNode *node) {
		// returns false if this out of scope node was already inserted.
		return oosNodes.insert(node).second;
	}
	
	const SVFGNodeSet& getOosNodes() const {
		return oosNodes;
	}

	bool addDanglingPtrNode(const SVFGNode *node) {
		// returns false if this danglingPtr node was already inserted.
		return dPtrNodes.insert(node).second;
	}
	
	const SVFGNodeSet& getDanglingPtrNodes() const {
		return dPtrNodes;
	}

	void setOosForwardPath(NodeID oosNodeId, const ContextCond &funcPath) {
		oosToForwardPath[oosNodeId] = funcPath;	
	}

	const ContextCond& getOosForwardPath(NodeID oosNodeId) const {
		return oosToForwardPath.at(oosNodeId);
	}

	void setDPtrBackwardPath(NodeID dPtrNodeId, const ContextCond &funcPath) {
		dPtrToBackwardPath[dPtrNodeId] = funcPath;	
	}

	const ContextCond& getDPtrBackwardPath(NodeID dPtrNodeId) const {
		return dPtrToBackwardPath.at(dPtrNodeId);
	}

	static inline bool classof(const UARLiteAnalysisContext*) {
		return true;
	}

	static inline bool classof(const AnalysisContext*) {
		return true;
	}

private:
	// Nodes where the path from a stackNode leaves the valid scope.
	SVFGNodeSet oosNodes;

	// Nodes where the path from a stackNode leaves the valid scope with direct return.
	SVFGNodeSet dirOosNodes;

	// All dangling pointer.
	SVFGNodeSet dPtrNodes;

	// Path from a local variable to the node (oos) leaving the valid scope.
	ContextCondMap oosToForwardPath;

	// Path from a oos to the dangling pointer.
	ContextCondMap dPtrToBackwardPath;
};

/**
 * An extended AnalysisContext to capture dangling pointer.
 */
class UARAnalysisContext : public AnalysisContext {
public:
	typedef std::set<const SVFGNode*> SVFGNodeSet;
	typedef std::set<NodeID> NodeIdSet;
	typedef std::list<UARDPItem> ItemList;
	typedef std::set<UARDPItem> ItemSet;
	typedef std::pair<NodeID, NodeID> CxtNodeIdVar;
	typedef std::map<CxtNodeIdVar, ItemSet> CxtNodeIdVarToItemSet;
	typedef std::pair<const ContextCond&, NodeID> CxtVar;
	typedef std::map<NodeID, CxtNodeIdSet> CxtNodeIdSetMap;
	typedef std::list<CxtNodeIdVar> CxtNodeIdList;
	typedef std::vector<CxtNodeIdList> PathVector;
	typedef std::map<NodeID, PathVector> PathSetMap;

	UARAnalysisContext(const SVFGNode* src, const SVFG* graph)
		: AnalysisContext(src, nullptr, graph) {
	}

	virtual ~UARAnalysisContext() { 
		//TODO
	}

	bool addOosNode(const SVFGNode *node) {
		// returns false if this out of scope node was already inserted.
		return oosNodes.insert(node).second;
	}
	
	const SVFGNodeSet& getOosNodes() const {
		return oosNodes;
	}

	bool addDirOosNode(const SVFGNode *node) {
		// returns false if this out of scope node was already inserted.
		return dirOosNodes.insert(node).second;
	}

	const SVFGNodeSet& getDirOosNodes() const {
		return dirOosNodes;
	}

	void addOosForwardSlice(NodeID oosNodeId, const CxtNodeIdSet &forwardSlice) {
		CxtNodeIdSet &set = oosToForwardSlice[oosNodeId];
		// Maybe we reached the out of scope node with different paths,
		// in that case we merge the forward slices.
		set.insert(forwardSlice.begin(), forwardSlice.end());
	}

	bool inOosForwardSlice(NodeID oosNodeId, NodeID cxt, NodeID svfgNodeId) const {
		CxtNodeIdSet set = oosToForwardSlice.at(oosNodeId);
		CxtNodeIdVar cxtVar(cxt, svfgNodeId);
		// If the node was added before entering the first function,
		// the context will be 0;
		CxtNodeIdVar cxtVar2(0, svfgNodeId);

		if(set.find(cxtVar) != set.end())
			return true;

		return set.find(cxtVar2) != set.end();
	}

	bool hasOosForwardSlice(NodeID oosNodeId) const {
		return oosToForwardSlice.find(oosNodeId) != oosToForwardSlice.end(); 
	}

	const CxtNodeIdSet& getOosForwardSlice(NodeID oosNodeId) const {
		return oosToForwardSlice.at(oosNodeId);	
	}
	
	void addOosForwardPath(NodeID oosNodeId, const CxtNodeIdList &path) {
		oosToForwardPaths[oosNodeId].push_back(path);	
	}

	void appendOosForwardPath(NodeID oosNodeId, const CxtNodeIdList &path) {
		PathVector &paths = oosToForwardPaths[oosNodeId];

		for(auto iter = paths.begin(); iter != paths.end(); ++iter) {
			CxtNodeIdList &origPath = *iter;
			origPath.insert(origPath.end(), path.begin(), path.end());	
		}
	}

	const PathVector& getOosForwardPaths(NodeID oosNodeId) const {
		return oosToForwardPaths.at(oosNodeId);
	}

	/**
	 * Since there are multiple forward-paths we have to find the one that has at least 
	 * the last store node identical to the backward path.
	 */
	bool getOosForwardPath(NodeID oosNodeId, const CxtNodeIdList &backwardPath, CxtNodeIdList& forwardPath) const {
		const PathVector &forwardPaths = oosToForwardPaths.at(oosNodeId);

		for(const auto &iter: forwardPaths) {
			auto fIterB = iter.rbegin(); 
			auto fIterE = iter.rend(); 
			auto bIterB = backwardPath.rbegin(); 
			auto bIterE = backwardPath.rend(); 

			for(; fIterB != fIterE && bIterB != bIterE; ++fIterB, ++bIterB) {
				const SVFGNode *fNode = getSVFG()->getSVFGNode(fIterB->second);
				const SVFGNode *bNode = getSVFG()->getSVFGNode(bIterB->second);

				if(fNode->getId() != bNode->getId())
					break;

				if(llvm::isa<StoreSVFGNode>(fNode) && llvm::isa<StoreSVFGNode>(bNode)) {
					forwardPath = iter;
					return true;
				}
			}
		}

		return false;
	}

	void addWaitingFuncEntryItem(NodeID funcEntryNodeId, CallSiteID csId, const UARDPItem &item) {
		CxtNodeIdVar cxtVar(csId, funcEntryNodeId);
		waitingFuncEntryToItems[cxtVar].insert(item);	
	}

	bool hasWaitingFuncEntry(NodeID funcEntryNodeId, CallSiteID csId) const {
		CxtNodeIdVar cxtVar(csId, funcEntryNodeId);
		return waitingFuncEntryToItems.find(cxtVar) != waitingFuncEntryToItems.end();
	}

	const ItemSet& getWaitingFuncEntryItems(NodeID funcEntryNodeId, CallSiteID csId) const {
		CxtNodeIdVar cxtVar(csId, funcEntryNodeId);
		return waitingFuncEntryToItems.at(cxtVar);
	}

	ItemSet getWaitingFuncEntryItems(NodeID funcEntryNodeId)  const {
		ItemSet waitingItems;

		for(auto iter = waitingFuncEntryToItems.begin(); iter != waitingFuncEntryToItems.end(); ++iter) {
			if(iter->first.first == funcEntryNodeId) {
				ItemSet tmpSet = iter->second;
				waitingItems.insert(tmpSet.begin(), tmpSet.end());
			}
		}
		return waitingItems;
	}

	NodeID getFuncEntryWithMostWaiters() const {
		long max = 0;
		NodeID funcEntry = 0;

		for(auto iter = waitingFuncEntryToItems.begin(); iter != waitingFuncEntryToItems.end(); ++iter) {
			if((*iter).second.size() > max) {
				max = (*iter).second.size();
				funcEntry = (*iter).first.first;
			}
		}

		return funcEntry;
	}

	long getNumWaiters(NodeID funcEntry) const {
		long numWaiters = 0;

		for(auto iter = waitingFuncEntryToItems.begin(); iter != waitingFuncEntryToItems.end(); ++iter) {
			if((*iter).first.first == funcEntry)
				numWaiters += (*iter).second.size();
		}

		return numWaiters;
	}

	void removeWaitingFuncEntryItems(NodeID funcEntryNodeId, CallSiteID csId) {
		CxtNodeIdVar cxtVar(csId, funcEntryNodeId);
		waitingFuncEntryToItems.erase(cxtVar);
	}

	void clearWaitingFuncEntryItems() {
		waitingFuncEntryToItems.clear();
	}

	void addFuncEntryToWorkSet(NodeID funcEntryNodeId) {
		funcWorkSet.insert(funcEntryNodeId);		
	}

	bool inFuncEntryWorkSet(NodeID funcEntryNodeId) {
		return funcWorkSet.find(funcEntryNodeId) != funcWorkSet.end();
	}

	void removeFuncEntryFromWorkSet(NodeID funcEntryNodeId) {
		funcWorkSet.erase(funcEntryNodeId);
	}

	const NodeIdSet& getFuncEntryWorkSet() const {
		return funcWorkSet;
	}

	void clearFuncEntryWorkSet() {
		funcWorkSet.clear();
	}	

	static inline bool classof(const UARAnalysisContext*) {
		return true;
	}

	static inline bool classof(const AnalysisContext*) {
		return true;
	}

private:
	// Nodes where the path from a stackNode leaves the valid scope.
	SVFGNodeSet oosNodes;

	// Nodes where the path from a stackNode leaves the valid scope with direct return.
	SVFGNodeSet dirOosNodes;

	// Nodes from a local variable to the node (oos) leaving the valid scope.
	CxtNodeIdSetMap oosToForwardSlice;

	// Path from a local variable to the node (oos) leaving the valid scope.
	PathSetMap oosToForwardPaths;

	// A map of ids of a function entry node with the corresponding callsiteId(context) 
	// and the corresponding item, which is waiting that the function is handled for its context.
	CxtNodeIdVarToItemSet waitingFuncEntryToItems;

	// Contains the functions we are currently working on.
	NodeIdSet funcWorkSet;

	double start_t;
};

/***
 * Captures the information of the SimplePtrAssignmentAnalysis.
 */
class DRAAnalysisContext : public AnalysisContext {
public:
	typedef std::pair<const ContextCond&, NodeID> CxtVar;
	typedef std::list<CxtNodeIdVar> CxtNodeIdList;
	typedef std::set<CxtNodeIdVar> CxtNodeIdSet;

	DRAAnalysisContext(const SVFGNode* src, const SVFG* graph)
		: AnalysisContext(src, nullptr, graph) {
	}

	virtual ~DRAAnalysisContext() { 
		//TODO
	}

	void setAssignmentNode(const SVFGNode *node) {
		assignmentNode = node;
	}

	const SVFGNode* getAssignmentNode() const {
		return assignmentNode;
	}

	void setDstNode (const SVFGNode *node) {
		dstNode = node;
	}

	const SVFGNode* getDstNode() const {
		return dstNode;
	}

	void setForwardPath(const CxtNodeIdList &path) {
		forwardPath = path;
	}

	const CxtNodeIdList& getForwardPath() const {
		return forwardPath;
	}

	void setBackwardPath(const CxtNodeIdList &path) {
		backwardPath = path;
	}

	const CxtNodeIdList& getBackwardPath() const {
		return backwardPath;
	}

	void setForwardSlice(const CxtNodeIdSet &slice) {
		forwardSlice = slice;
	}

	const CxtNodeIdSet& getForwardSlice() const {
		return forwardSlice;
	}

	bool inForwardSlice(NodeID cxt, NodeID id) const {
		CxtNodeIdVar cxtVar(cxt, id);
		return forwardSlice.find(cxtVar) != forwardSlice.end();
	}

	void setBackwardSlice(const CxtNodeIdSet &slice) {
		backwardSlice = slice;
	}

	const CxtNodeIdSet& getBackwardSlice() const {
		return backwardSlice;
	}

	bool inBackwardSlice(NodeID cxt, NodeID id) const {
		CxtNodeIdVar cxtVar(cxt, id);
		return backwardSlice.find(cxtVar) != backwardSlice.end();
	}

private:
	CxtNodeIdList forwardPath;
	CxtNodeIdList backwardPath;
	CxtNodeIdSet forwardSlice;
	CxtNodeIdSet backwardSlice;
	const SVFGNode *assignmentNode;
	const SVFGNode *dstNode;
};


class SrcSnkAnalysisContext : public AnalysisContext {
public:
	typedef std::map<NodeIDPair, KSrcSnkDPItem> ItemMap;

	SrcSnkAnalysisContext(const SVFGNode* src, PathCondAllocator *pa, const SVFG* graph)
		: AnalysisContext(src, pa, graph), maxPathID(0){
	}

	virtual ~SrcSnkAnalysisContext() { 
		//TODO
	}

	void addVisitedSink(const SVFGNode *node) {
		visitedSinks.insert(node);
	}

	bool isVisitedSink(const SVFGNode *node) {
		return visitedSinks.find(node) != visitedSinks.end();
	}

	const SVFGNodeSet& getVisitedSinks() const {
		return visitedSinks;
	}

	/**
	 * Maps the backward item to the dealloc-functions.
	 */
	bool addSinkPath(const KSrcSnkDPItem &item) {
		unsigned int pathID = numPathMap[item.getLoc()];
		numPathMap[item.getLoc()] = pathID+1;
		NodeIDPair key(item.getLoc()->getId(), pathID);
//		llvm::outs() << "add sink\n";
//		item.dump();

		if(pathID > maxPathID || pathID == 0)
			maxPathID = pathID;
		
		sinkPaths.emplace(key, item);
		return true;
	}

	const ItemMap& getSinkItems() const {
		return sinkPaths;
	}

	const KSrcSnkDPItem& getSinkItemAt(NodeIDPair key) const {
		return sinkPaths.at(key);
	}

	unsigned int getMaxPathID() const {
		return maxPathID;
	}

	/**
	 * Sorts the doubleFree functions nodes depeding on their call.
	 */
	void sortPaths();

	/**
	 * Removes the paths with sinks that have VFCond as false.
	 */
	void filterUnreachablePaths();

	/**
	 * Returns the sorted paths.
	 */
	const NodeIDPairVec& getFlowSensSinkPaths() const {
		return fsSinkPaths;
	}

	void addToFlowSensSinkPaths(NodeIDPair key, const KSrcSnkDPItem &item);

private:
	SVFGNodeSet visitedSinks;

	// Contains the deallocation nodes.
	SVFGNodeSet deallocNodes;

	// Contains the nodes representing a null assignment.
	SVFGNodeSet nullNodes;

	// Contains the nodes representing a load or a store.
	SVFGNodeSet useNodes;

	// Stores an item containing the path from a deallocation or null assignment to the allocation.
	ItemMap sinkPaths;

	// Contains the sources (dealloc nodes) in the order they are called.
	NodeIDPairVec fsSinkPaths;

	// Contains the information how many paths a deallocation or null assignment has.
	SVFGNodeCnt numPathMap;

	unsigned int maxPathID;
};

#endif // ANALYSIS_CONTEXT_H
