#ifndef DIR_RET_ANALYSIS_H_
#define DIR_RET_ANALYSIS_H_

#include "Util/AnalysisContext.h"
#include "Util/KCFSolver.h"

/***
 * Analysis the return of a function and determines the variable
 * the return value is assigned to.
 */
class DirRetAnalysis : public KCFSolver<SVFG*, CxtTraceDPItem> {
public:
	DirRetAnalysis(SVFG *svfg): dra_curAnalysisCxt(nullptr) { 
		setGraph(svfg);	
	}

	virtual ~DirRetAnalysis() { 
	}

	/**
	 * Finds for the given node the variable (dest) the value
	 * of the node is assigned to.
	 */
	const SVFGNode* findDstOfReturn(const SVFGNode *retNode); 

	/**
	 * Returns a list of the node ids that were forwarded as last.
	 */
	const CxtNodeIdList& getForwardPath() const {
		return dra_curAnalysisCxt->getForwardPath();
	}

	/**
	 * Returns a list of the node ids that were backwarded as last.
	 */
	const CxtNodeIdList& getBackwardPath() const {
		return dra_curAnalysisCxt->getBackwardPath();
	}

	/**
	 * Returns a set of the node ids that were forwarded as last.
	 */
	const CxtNodeIdSet& getForwardSlice() const {
		return dra_curAnalysisCxt->getForwardSlice();
	}

	/**
	 * Returns a set of the nodes that were backwarded as last.
	 */
	const CxtNodeIdSet& getBackwardSlice() const {
		return dra_curAnalysisCxt->getBackwardSlice();
	}

protected:

	/**
	 * Seeks for the first StoreSVFGNode.
	 */
	virtual inline void forwardProcess(const CxtTraceDPItem &item); 

	/**
	 * Seeks for an AddrSVFGNode.
	 */
	virtual inline void backwardProcess(const CxtTraceDPItem &item);
	
	/**
	 * Forward propagets the item in a contextsensitive way. If a StoreSVFGNode
	 * if visited, the worklist will be cleared.
	 */
	virtual void forwardpropagate(const CxtTraceDPItem &item, SVFGEdge *edge);

	/**
	 * Backward propagets the item in a contextsensitive way. 
	 */
	virtual void backwardpropagate(const CxtTraceDPItem &item, SVFGEdge *edge);

	/**
	 * Sets the program slice.
	 */
	void setCurSlice(const SVFGNode* src);

	/**
	 * Starts the forward traversion.
	 */
	void findAssignment(const SVFGNode *srcNode);

	/**
	 * Starts the backward traversion.
	 */
	void findDst();

	/**
	 * Adds the forwardSlice/Path and assignment node to the program slice.
	 */
	void handleAssignment(const SVFGNode *node, const CxtTraceDPItem &item);

	/**
	 * Adds the backwardSlice/Path and dest node to the program slice.
	 */
	void handleDst(const SVFGNode *node, const CxtTraceDPItem &item);

	/**
	 * Initializes the current programSlice.
	 */
	void setCurAnalysisCxt(const SVFGNode* src);

private:
	// Stores information during the traversion.
	DRAAnalysisContext *dra_curAnalysisCxt;
};

#endif // DIR_RET_ANALYSIS_H_
