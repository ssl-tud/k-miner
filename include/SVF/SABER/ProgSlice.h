//===- ProgSlice.h -- Program slicing based on SVF----------------------------//
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

/*
 * ProgSlice.h
 *
 *  Created on: Apr 1, 2014
 *      Author: Yulei Sui
 */

#ifndef PROGSLICE_H_
#define PROGSLICE_H_

#include "MSSA/SVFG.h"
#include "Util/PathCondAllocator.h"
#include "Util/DebugUtil.h"
#include "Util/WorkList.h"
#include "Util/DPItem.h"
#include "Util/SVFGAnalysisUtil.h"
#include <algorithm>    // std::sort
#include <omp.h>

class ProgSlice {

public:
	typedef std::set<const SVFGNode*> SVFGNodeSet;
	typedef SVFGNodeSet::iterator SVFGNodeSetIter;
	typedef PathCondAllocator::Condition Condition;
	typedef std::map<const SVFGNode*, Condition*> SVFGNodeToCondMap; 	///< map a SVFGNode to its condition during value-flow guard computation

	typedef FIFOWorkList<const SVFGNode*> VFWorkList;		    ///< worklist for value-flow guard computation
	typedef FIFOWorkList<const llvm::BasicBlock*> CFWorkList;	///< worklist for control-flow guard computation

	/// Constructor
	ProgSlice(const SVFGNode* src, PathCondAllocator* pa, const SVFG* graph):
		root(src), partialReachable(false), fullReachable(false), reachGlob(false),
		pathAllocator(pa), _curSVFGNode(NULL), finalCond(pa->getFalseCond()), svfg(graph) {
		}

	/// Destructor
	virtual ~ProgSlice() {
		destroy();
	}

	inline u32_t getForwardSliceSize() const {
		return forwardslice.size();
	}
	inline u32_t getBackwardSliceSize() const {
		return backwardslice.size();
	}
	/// Forward and backward slice operations
	//@{
	inline void addToForwardSlice(const SVFGNode* node) {
		forwardslice.insert(node);
	}
	inline void addToBackwardSlice(const SVFGNode* node) {
		backwardslice.insert(node);
	}
	inline bool inForwardSlice(const SVFGNode* node) {
		return forwardslice.find(node)!=forwardslice.end();
	}
	inline bool inBackwardSlice(const SVFGNode* node) {
		return backwardslice.find(node)!=backwardslice.end();
	}
	inline SVFGNodeSetIter forwardSliceBegin() const {
		return forwardslice.begin();
	}
	inline SVFGNodeSetIter forwardSliceEnd() const {
		return forwardslice.end();
	}
	inline SVFGNodeSetIter backwardSliceBegin() const {
		return backwardslice.begin();
	}
	inline SVFGNodeSetIter backwardSliceEnd() const {
		return backwardslice.end();
	}
	inline void clearForwardSlice() {
		forwardslice.clear();
	}
	inline void clearBackwardSlice() {
		backwardslice.clear();
	}
	inline SVFGNodeSet getForwardSlice() const {
		return forwardslice;
	}
	inline SVFGNodeSet getBackwardSlice() const {
		return backwardslice;
	}
	//@}

	/// root and sink operations
	//@{
	inline const SVFGNode* getSource() const {
		return root;
	}
	inline void addToSinks(const SVFGNode* node) {
		sinks.insert(node);
	}
	inline const SVFGNodeSet& getSinks() const {
		return sinks;
	}
	inline SVFGNodeSetIter sinksBegin() const {
		return sinks.begin();
	}
	inline SVFGNodeSetIter sinksEnd() const {
		return sinks.end();
	}
	inline void setPartialReachable() {
		partialReachable = true;
	}
	inline void setAllReachable() {
		fullReachable = true;
	}
	inline bool setReachGlobal() {
		return reachGlob = true;
	}
	inline bool isPartialReachable() const {
		return partialReachable || reachGlob;
	}
	inline bool isAllReachable() const {
		return fullReachable || reachGlob;
	}
	inline bool isReachGlobal() const {
		return reachGlob;
	}
	//@}

	/// Guarded reachability solve
	virtual void AllPathReachableSolve();
	virtual bool isSatisfiableForAll();
	virtual bool isSatisfiableForPairs();

	/// Get llvm value from a SVFGNode
	const llvm::Value* getLLVMValue(const SVFGNode* node) const;

	/// Get callsite ID and get returnsiteID from SVFGEdge
	//@{
	llvm::CallSite getCallSite(const SVFGEdge* edge) const;
	llvm::CallSite getRetSite(const SVFGEdge* edge) const;
	//@}

	/// Condition operations
	//@{
	inline Condition* condAnd(Condition* lhs, Condition* rhs) {
		return pathAllocator->condAnd(lhs,rhs);
	}
	inline Condition* condOr(Condition* lhs, Condition* rhs) {
		return pathAllocator->condOr(lhs,rhs);
	}
	inline Condition* condNeg(Condition* cond) {
		return pathAllocator->condNeg(cond);
	}
	inline Condition* getTrueCond() const {
		return pathAllocator->getTrueCond();
	}
	inline Condition* getFalseCond() const {
		return pathAllocator->getFalseCond();
	}
	inline std::string dumpCond(Condition* cond) const {
		return pathAllocator->dumpCond(cond);
	}
	/// Evaluate final condition
	std::string evalFinalCond() const;
	//@}

	/// Annotate program according to final condition
	void annotatePaths();

protected:
	inline const SVFG* getSVFG() const {
		return svfg;
	}

	/// Release memory
	void destroy();
	/// Clear Control flow conditions before each VF computation
	inline void clearCFCond() {
		/// TODO: how to clean bdd memory
		pathAllocator->clearCFCond();
	}

	/// Get/set VF (value-flow) and CF (control-flow) conditions
	//@{
	inline Condition* getVFCond(const SVFGNode* node) const {
		SVFGNodeToCondMap::const_iterator it = svfgNodeToCondMap.find(node);
		if(it==svfgNodeToCondMap.end()) {
			return getFalseCond();
		}
		return it->second;
	}
	inline bool setVFCond(const SVFGNode* node, Condition* cond) {
		SVFGNodeToCondMap::iterator it = svfgNodeToCondMap.find(node);
		if(it!=svfgNodeToCondMap.end() && it->second == cond)
			return false;

		svfgNodeToCondMap[node] = cond;
		return true;
	}
	//@}

	/// Compute guards for value-flows
	//@{
	inline Condition* ComputeIntraVFGGuard(const llvm::BasicBlock* src, const llvm::BasicBlock* dst) {
		return pathAllocator->ComputeIntraVFGGuard(src,dst);
	}
	inline Condition* ComputeInterCallVFGGuard(const llvm::BasicBlock* src, const llvm::BasicBlock* dst, const llvm::BasicBlock* callBB) {
		return pathAllocator->ComputeInterCallVFGGuard(src,dst,callBB);
	}
	inline Condition* ComputeInterRetVFGGuard(const llvm::BasicBlock* src, const llvm::BasicBlock* dst, const llvm::BasicBlock* retBB) {
		return pathAllocator->ComputeInterRetVFGGuard(src,dst,retBB);
	}
	//@}

	/// Return the basic block where a SVFGNode resides in
	/// a SVFGNode may not in a basic block if it is not a program statement
	/// (e.g. PAGEdge is an global assignment or NullPtrSVFGNode)
	inline const llvm::BasicBlock* getSVFGNodeBB(const SVFGNode* node) const {
		const llvm::BasicBlock* bb = node->getBB();
		if(llvm::isa<NullPtrSVFGNode>(node) == false) {
			if(!bb)
				return NULL;
//			assert(bb && "this SVFG node should be in a basic block");
			return bb;
		}
		return NULL;
	}

	/// Get/set current SVFG node
	//@{
	inline const SVFGNode* getCurSVFGNode() const {
		return _curSVFGNode;
	}
	inline void setCurSVFGNode(const SVFGNode* node) {
		_curSVFGNode = node;
		pathAllocator->setCurEvalVal(getLLVMValue(_curSVFGNode));
	}
	//@}
	/// Set final condition after all path reachability analysis
	inline void setFinalCond(Condition* cond) {
		finalCond = cond;
	}

private:
	SVFGNodeSet forwardslice;				///<  the forward slice
	SVFGNodeSet backwardslice;				///<  the backward slice
	SVFGNodeSet sinks;						///<  a set of sink nodes
	const SVFGNode* root;					///<  root node on the slice
	SVFGNodeToCondMap svfgNodeToCondMap;	///<  map a SVFGNode to its path condition starting from root
	bool partialReachable;					///<  reachable from some paths
	bool fullReachable;						///<  reachable from all paths
	bool reachGlob;							///<  Whether slice reach a global
	PathCondAllocator* pathAllocator;		///<  path condition allocator
	const SVFGNode* _curSVFGNode;			///<  current svfg node during guard computation
	Condition* finalCond;					///<  final condition
	const SVFG* svfg;						///<  SVFG
};

/**
 * An extended ProgSlice to capture dangling pointer.
 */
class UARLiteProgSlice : public ProgSlice {
public:
	typedef std::set<const SVFGNode*> SVFGNodeSet;
	typedef std::list<std::string> StringList;
	typedef std::list<CxtNodeIdVar> CxtNodeIdList;
	typedef std::map<NodeID, ContextCond> ContextCondMap;

	UARLiteProgSlice(const SVFGNode* src, PathCondAllocator* pa, const SVFG* graph): ProgSlice(src, pa, graph), start_t(omp_get_wtime()) {
	}

	virtual ~UARLiteProgSlice() { 
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

	double getStartTime() const {
		return start_t;
	}

	const StringList& getSyscallPath() const {
		return syscallPath;
	}

	void setSyscallPath(const StringList &path) {
		syscallPath = path;
	}

	static inline bool classof(const UARLiteProgSlice*) {
		return true;
	}

	static inline bool classof(const ProgSlice*) {
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

	// Path from the systemcall to the local variable.
	StringList syscallPath;

	double start_t;
};

/**
 * An extended ProgSlice to capture dangling pointer.
 */
class UARProgSlice : public ProgSlice {
public:
	typedef std::set<const SVFGNode*> SVFGNodeSet;
	typedef std::set<NodeID> NodeIdSet;
	typedef std::list<UARDPItem> ItemList;
	typedef std::list<std::string> StringList;
	typedef std::set<UARDPItem> ItemSet;
	typedef std::pair<NodeID, NodeID> CxtNodeIdVar;
	typedef std::map<CxtNodeIdVar, ItemSet> CxtNodeIdVarToItemSet;
	typedef std::pair<const ContextCond&, NodeID> CxtVar;
	typedef std::map<NodeID, CxtNodeIdSet> CxtNodeIdSetMap;
	typedef std::list<CxtNodeIdVar> CxtNodeIdList;
	typedef std::vector<CxtNodeIdList> PathVector;
	typedef std::map<NodeID, PathVector> PathSetMap;

	UARProgSlice(const SVFGNode* src, PathCondAllocator* pa, const SVFG* graph): ProgSlice(src, pa, graph), start_t(omp_get_wtime()) {
	}

	virtual ~UARProgSlice() { 
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

	double getStartTime() const {
		return start_t;
	}

	const StringList& getSyscallPath() const {
		return syscallPath;
	}

	void setSyscallPath(const StringList &path) {
		syscallPath = path;
	}

	static inline bool classof(const UARProgSlice*) {
		return true;
	}

	static inline bool classof(const ProgSlice*) {
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

	// Path from the systemcall to the local variable.
	StringList syscallPath;

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
class DRAProgSlice : public ProgSlice {
public:
	typedef std::pair<const ContextCond&, NodeID> CxtVar;
	typedef std::list<CxtNodeIdVar> CxtNodeIdList;
	typedef std::set<CxtNodeIdVar> CxtNodeIdSet;

	DRAProgSlice(const SVFGNode* src, PathCondAllocator* pa, const SVFG* graph): ProgSlice(src, pa, graph) {
	}

	virtual ~DRAProgSlice() { 
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

#endif /* PROGSLICE_H_ */
