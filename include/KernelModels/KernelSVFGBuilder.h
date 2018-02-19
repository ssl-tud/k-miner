#ifndef Kernel_SVFG_Builder_H
#define Kernel_SVFG_Builder_H

// KernelSVFGBuilder.h adapted from:
//
//===- SaberSVFGBuilder.h -- Building SVFG for Saber--------------------------//
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

#include "SVF/MSSA/SVFGBuilder.h"
#include "SVF/Util/WorkList.h"
#include "KernelModels/KernelContext.h"

class SVFGNode;
class PAGNode;

class KernelSVFGBuilder: public SVFGBuilder {
public:
	typedef FIFOWorkList<NodeID> WorkList;
	typedef std::set<const SVFGNode*> SVFGNodeSet;
	typedef std::map<NodeID, NodeBS> NodeToPTSSMap;

	KernelSVFGBuilder() { 
		kernelCxt = KernelContext::getKernelContext();
	}

	virtual ~KernelSVFGBuilder() {}

	bool isAllocNode(const SVFGNode* node) const {
		return allocNodes.find(node) != allocNodes.end();
	}

	bool isDeallocNode(const SVFGNode* node) const {
		return deallocNodes.find(node) != deallocNodes.end();
	}

	bool isGlobalNode(const SVFGNode* node) const {
		return globalNodes.find(node) != globalNodes.end();
	}

	bool isLocalNode(const SVFGNode* node) const {
		return localNodes.find(node) != localNodes.end();
	}

	bool isUseNode(const SVFGNode* node) const {
		return useNodes.find(node) != useNodes.end();
	}

	bool isLockObjNode(const SVFGNode* node) const {
		return lockObjNodes.find(node) != lockObjNodes.end();
	}

	bool isLockNode(const SVFGNode* node) const {
		return lockNodes.find(node) != lockNodes.end();
	}

	bool isUnlockNode(const SVFGNode* node) const {
		return unlockNodes.find(node) != unlockNodes.end();
	}

	bool isGlobalStoreNode(const SVFGNode* node) const {
		return globalStoreNodes.find(node) != globalStoreNodes.end();
	}

	bool isNullStoreNode(const SVFGNode* node) const {
		return nullStoreNodes.find(node) != nullStoreNodes.end();
	}

	const SVFGNodeSet& getAllocNodes() const {
		return allocNodes;
	}

	const SVFGNodeSet& getDeallocNodes() const {
		return deallocNodes;
	}

	const SVFGNodeSet& getGlobalNodes() const {
		return globalNodes;
	}

	const SVFGNodeSet& getLocalNodes() const {
		return localNodes;
	}

	const SVFGNodeSet& getUseNodes() const {
		return useNodes;
	}

	const SVFGNodeSet& getLockObjNodes() const {
		return lockObjNodes;
	}

	const SVFGNodeSet& getLockNodes() const {
		return lockNodes;
	}

	const SVFGNodeSet& getUnlockNodes() const {
		return unlockNodes;
	}

	const SVFGNodeSet& getNullStoreNodes() const {
		return nullStoreNodes;
	}
protected:
	void createSVFG(MemSSA* mssa, SVFG* graph);
private:
	/**
	 * Function is part of the api.
	 */
	bool isAPIFunction(const llvm::Function* fun) {
		if(fun && fun->hasName())
			return kernelCxt->isAPIFunction(fun->getName());
		return false;
	}

	/**
	 * Function is part of the api context.
	 */
	bool inAPIContext(const llvm::Function* fun) {
		if(fun && fun->hasName())
			return kernelCxt->inAPIContext(fun->getName());
		return false;
	}

	/**
	 * Removes the edges between dereferences.
	 */
	void rmDerefDirSVFGEdges(BVDataPTAImpl* pta);

	/**
	 * Add actual parameter SVFGNode for 1st argument of a deallocation like external function.
	 */
	void AddExtActualParmSVFGNodes();

	/**
	 * Collects SVFG nodes related to alloc and free functions.
	 */
	void collectAllocationNodes();

	/**
	 * Collects SVFG nodes related to global variables.
	 */
	void collectGlobalNodes();

	/**
	 * Collects SVFG nodes related to local variables.
	 */
	void collectLocalNodes();

	/**
	 * Collects SVFG nodes related to lock objects, locks and unlock functions.
	 */
	void collectLockNodes();

	/**
	 * Collects SVFG nodes related to a null assignment.
	 */
	void collectNullStores();

	/**
	 * Collects SVFG nodes related to an usage.
	 */
	void collectUseNodes();

	/**
	 * Collects SVFG nodes related to alloc and free functions.
	 */
	void collectGlobalStores(BVDataPTAImpl* pta);

	/**
	 * Recursively collect global memory objects.
	 */
	void collectGlobals(BVDataPTAImpl* pta);

	/**
	 * Decide whether the node and its points-to contains a global objects.
	 */
	bool accessGlobal(BVDataPTAImpl* pta, const PAGNode* pagNode);

	NodeBS& CollectPtsChain(BVDataPTAImpl* pta, NodeID id, NodeToPTSSMap& cachedPtsMap);
	NodeBS globs;
	SVFGNodeSet allocNodes;
	SVFGNodeSet deallocNodes;
	SVFGNodeSet globalNodes;
	SVFGNodeSet localNodes;
	SVFGNodeSet lockObjNodes;
	SVFGNodeSet lockNodes;
	SVFGNodeSet unlockNodes;
	SVFGNodeSet useNodes;
	SVFGNodeSet globalStoreNodes;
	SVFGNodeSet nullStoreNodes;

	KernelContext *kernelCxt;
};

#endif // Kernel_SVFG_Builder
