#ifndef LOCAL_POINTER_ASSIGNMENT_ANALYSIS_H_
#define LOCAL_POINTER_ASSIGNMENT_ANALYSIS_H_

#include "llvm/Analysis/CFG.h"
#include "SABER/PAGAnalysis.h"
#include "Util/SVFGAnalysisUtil.h"
#include "SABER/ProgSlice.h"

typedef std::list<const SVFGEdge*> SVFGEdgeList;
typedef std::set<const PAGNode*> PAGNodeSet;
typedef std::set<const SVFGNode*> SVFGNodeSet;
typedef std::pair<const InterPHISVFGNode*, u32_t> FormalParamToPos;

class LPAAnalysis : public CFLSolver<SVFG*,ScopeDPItem> {
private:
	// Because we want start with the latest assignment before the recentNode
	// we have to insert all items in the order they were accessed.
	void initWorklist(ScopeDPItem &item, const SVFGNode* recentNode);

	// Adds the correct parameter informations to the item.
	void handleFuncEntry(ScopeDPItem &funcEntryItem, const SVFGEdge *edge);

	// Sorts the edges in the list by the order of instructions of the destination nodes.
	// The node which doesn't reach another node is the last one. All nodes have to reach
	// the recentNode or the corresponding edge is removed from the list. 
	void sortSVFGEdgeList(SVFGEdgeList &edgeList, const SVFGNode *recentNode);

	void convertPAGNodesToSVFGNodes(PAGNodeSet &pagNodes, SVFGNodeSet &svfgNodes);

	bool compare_inst_first(const SVFGEdge *edge1, const SVFGEdge *edge2);

	const llvm::Instruction* getInstruction(const SVFGNode* node);
public:
	LPAAnalysis(llvm::Module &module, SVFG *svfg): module(module) { 
		setGraph(svfg);	
		pathCondAllocator = new PathCondAllocator();
		pagAnalysis = PAGAnalysis::createPAGAnalysis(svfg);
		PAGNodeSet &globalObjPNodes = pagAnalysis->getGlobalObjPNodes();
		convertPAGNodesToSVFGNodes(globalObjPNodes, globalAddrSVFGNodes);
		PAGNodeSet &localObjPNodes = pagAnalysis->getLocalObjPNodes();
		convertPAGNodesToSVFGNodes(localObjPNodes, localAddrSVFGNodes);
	}

	virtual ~LPAAnalysis() { 
		if(pathCondAllocator)
			delete pathCondAllocator;
		pathCondAllocator = NULL;
	}

	// Finds the latest global variable which the local pointer points to. The assignment has 
	// to be done before the recentNode.
	const SVFGNode* findRecentAssignment(const SVFGNode *localPtr, const SVFGNode *recentNode);

	// Finds a global variable which is the source of the given storeNode (curNode in Item). 
	// The item contains the path information from the dst node (local ptr) to the store node.
	const SVFGNode* findSrcOfStoreNode();
	const SVFGNode* findSrcOfStoreNode(const SVFGNode *storeNode, const ScopeDPItem &item);

	void setCurSlice(const SVFGNode* src);

protected:
	// Forward traverse
	virtual inline void forwardProcess(const ScopeDPItem& item); 

	// Backward traverse
	virtual inline void backwardProcess(const ScopeDPItem& item);

	// CFL forwardward traverse solve
    	virtual void forwardTraverse(ScopeDPItem& it); 
	// CFL backward traverse solve
	virtual void backwardTraverse(ScopeDPItem& it); 
	// Propagate information forward by matching context
	virtual void forwardpropagate(const ScopeDPItem& item, SVFGEdge* edge);
	// Propagate information backward without matching context, as forward analysis already did it
	virtual void backwardpropagate(const ScopeDPItem& item, SVFGEdge* edge);

	// Checks if a node should be forwarded.
	bool forwardChecks(ScopeDPItem &item); 

	// Checks if a node should be backwarded.
	bool backwardChecks(ScopeDPItem &item);

private:
	SVFGNodeSet globalAddrSVFGNodes, localAddrSVFGNodes;
	LPAAProgSlice* _curSlice;		
	PAGAnalysis *pagAnalysis;
	PTACFInfoBuilder CFBuilder;
	PathCondAllocator* pathCondAllocator;
	llvm::Module &module;
};

#endif // LOCAL_POINTER_ASSIGNMENT_ANALYSIS_H_


