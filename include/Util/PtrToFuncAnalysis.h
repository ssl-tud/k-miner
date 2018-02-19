#ifndef PTR_TO_FUNC_ANALYSIS_H
#define PTR_TO_FUNC_ANALYSIS_H

#include "Util/KCFSolver.h"
#include "Util/PAGAnalysis.h"
#include "SVF/MemoryModel/ConsG.h"
#include "SVF/Util/BasicTypes.h"

/***
 * Determines the function that are used by a pointer.
 * This is done on top of the ConstraintGraph and the PAG.
 */
class PtrToFuncAnalysis : public KCFSolver<ConstraintGraph*, DPItem> {
public:
	typedef std::set<const ConstraintNode*> ConsNodeSet;
	typedef std::set<const PAGNode*> PAGNodeSet;
	typedef std::set<NodeID> NodeIDSet;

	PtrToFuncAnalysis(ConstraintGraph *consCG, PAG *pag): pag(pag) {
		setGraph(consCG);	
		pagAnalysis = PAGAnalysis::createPAGAnalysis(pag);
	}

	virtual ~PtrToFuncAnalysis() { 
		PAGAnalysis::releasePAGAnalysis();
	}

	/**
	 * For-/Backwards the Constraint Graph and checks the corresponding
	 * PAG edges in order to find the used functions.
	 */
	bool analyze(std::string globalVariableName);

	/**
	 * Get the found (used) functions.
	 */
	StringSet& getRelevantFunctions() {
		return relevantFunctions;
	}

	/**
	 * Get the found (used) globalvars.
	 */
	StringSet& getRelevantGlobalVars() {
		return relevantGlobalVars;
	}

	/**
	 * Resets the variables.
	 */
	void reset() {
		relevantFunctions.clear();
		relevantGlobalVars.clear();
		clearForwardVisited();
		clearBackwardVisited();
		backwardNodes.clear();
	}

protected:
	/**
	 * Checks if an edge contains a function.
	 */
	virtual inline void forwardProcess(const DPItem &item); 

	/**
	 * Checks if an edge contains a function and collects globalvars.
	 */
	virtual inline void backwardProcess(const DPItem &item);

	/**
	 * Forwards non visited nodes.
	 */
	virtual void forwardpropagate(const DPItem &item, ConstraintEdge *edge);

	/**
	 * Backwards non visited nodes.
	 */
	virtual void backwardpropagate(const DPItem &item, ConstraintEdge *edge);

	/**
	 * Collects new functions.
	 */
	void collectFunction(llvm::CallSite CS);

	/**
	 * Collects new functions.
	 */
	void collectFunction(const llvm::Function *F);

	/**
	 * Collects global and local nodes visited during backwarding.
	 */
	void collectVariables(const DPItem &item, ConstraintEdge *edge);

	/**
	 * Collects nodes during forwarding, that will be backwarded afterwards.
	 */
	void collectBackwardNodes(ConstraintNode *curNode, ConstraintNode *prevNode);

	/**
	 * Checks if a node was already visited by an item.
	 */
	bool hasForwardVisited(NodeID id) {
		return forwardVisitedNodes.find(id) != forwardVisitedNodes.end();
	}

	/**
	 * Adds a node to a set of visited.
	 */
	void addToForwardVisited(NodeID id) {
		forwardVisitedNodes.insert(id);
	}

	/**
	 * Clears the visited set.
	 */
	void clearForwardVisited() {
		forwardVisitedNodes.clear();
	}

	/**
	 * Clears the visited set.
	 */
	void clearBackwardVisited() {
		backwardVisitedNodes.clear();
	}

	/**
	 * Checks if a node was already visited by an item.
	 */
	bool hasBackwardVisited(NodeID id) {
		return backwardVisitedNodes.find(id) != backwardVisitedNodes.end();
	}

	/**
	 * Adds a node to a set of visited.
	 */
	void addToBackwardVisited(NodeID id) {
		backwardVisitedNodes.insert(id);
	}
	
private:
	PAG *pag;
	
	// Functions visited during for-/backwarding.
	StringSet relevantFunctions;

	// Globalvars visited during backwarding.
	StringSet relevantGlobalVars;
	
	// Nodes visited during forwarding.
	NodeIDSet forwardVisitedNodes;

	// Nodes visited during backwarding.
	NodeIDSet backwardVisitedNodes;

	// Nodes from which the backward-traversion starts from.
	ConsNodeSet backwardNodes; 

	PAGAnalysis *pagAnalysis;
};


#endif // PTR_TO_FUNC_ANALYSIS_H
