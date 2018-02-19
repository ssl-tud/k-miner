#ifndef USE_AFTER_RETURN_CHECKER_LITE
#define USE_AFTER_RETURN_CHECKER_LITE

#include "Checker/KernelCheckerAPI.h"
#include "Checker/KernelChecker.h"
#include "KernelModels/KernelSVFGBuilder.h"
#include "KernelModels/KernelContext.h"
#include "SVF/MemoryModel/PAG.h"
#include "SVF/MSSA/SVFGOPT.h"
#include "SVF/WPA/Andersen.h"
#include "SVF/WPA/FlowSensitive.h"
#include "Util/KCFSolver.h"
#include "Util/AnalysisContext.h"
#include "Util/CallGraphAnalysis.h"
#include "Util/Bug.h"
#include <string>
#include <sstream>
#include <fstream>
#include <thread>
#include <ctime>

typedef std::set<NodeID> NodeIdSet;
typedef KCFSolver<SVFG*, UARLiteDPItem> LiteSolver;
typedef std::list<UARLiteDPItem> UARLiteWorkList;
typedef std::vector<bool> BoolVec;

/***
 * A SVF-Analysis to find use-after-return bugs in C-Programs.
 */
class UseAfterReturnCheckerLite : public LiteSolver, public KernelChecker {
public:
	typedef std::set<const SVFGNode*> SVFGNodeSet;
	typedef std::set<const PAGNode*> PAGNodeSet;
	typedef std::set<std::string> StringSet;
	typedef std::list<std::string> StringList;
	typedef std::set<UARLiteDPItem> ItemSet;
	typedef std::map<std::string, int> StringToBool;
	typedef std::set<UARBug> UARBugSet;
	typedef std::map<const SVFGNode*, ItemSet> SVFGNodeToDPItemsMap; 

	static char ID;

	UseAfterReturnCheckerLite(unsigned int num_threads=1, char id = ID): 
		KernelChecker("Use-After-Return", ID), num_threads(num_threads), 
		curAnalysisCxt(nullptr), pag(nullptr), max_t(300), start_t(0) { 
	}

	virtual ~UseAfterReturnCheckerLite() {
		if(pag)
			PAG::releasePAG();
	}

protected:
	/**
	 * Has to be implemented by the individual checkers.
	 */
	virtual void analyze(llvm::Module &module);

	/**
	 * Has to be implemented by the checkers.
	 */
	virtual void evaluate() { }

	/**
	 * Return the number of variables analyzed.
	 */
	virtual unsigned int getNumAnalyzedVars() const { 
		return stackSVFGNodes.size();
	}

	/**
	 * Nothing
	 */
	virtual inline void forwardProcess(const UARLiteDPItem& item); 

	/**
	 * If a AddrSVFGNode was reached during backwarding this node
	 * will be handled as a dangling pointer.
	 */
	virtual inline void backwardProcess(const UARLiteDPItem& item);

	/**
	 * Starting by a local variable the function forwards through the SVFG.
	 * Some optimations were added in order to improve the performance with omp.
	 */
    	virtual void forwardTraverse(UARLiteDPItem& it); 
	
	/**
	 * Starting by a node that left the valid scope of the forwarded variable,
	 * this function start backwarding the SVFG.
	 * Some optimations were added in order to improve the performance with omp.
	 */
	virtual void backwardTraverse(UARLiteDPItem& it); 

	/**
	 * Forward propagates the item in a context sensitive way.
	 * Function paths are only forwarded once.
	 */
	virtual void forwardpropagate(const UARLiteDPItem& item, SVFGEdge* edge);

	/**
	 * Backward propagates the item in a context sensitive way.
	 */
	virtual void backwardpropagate(const UARLiteDPItem& item, SVFGEdge* edge);

	bool forwardVisited(const SVFGNode* node) const {
		return forwardVisitedMap.find(node) != forwardVisitedMap.end();
	}

	bool forwardVisited(const SVFGNode* node, const UARLiteDPItem& item) const {
		auto iter = forwardVisitedMap.find(node);
		if(iter!=forwardVisitedMap.end())
			return iter->second.find(item)!=iter->second.end();
		else
			return false;
	}

	void addForwardVisited(const SVFGNode* node, const UARLiteDPItem& item) {
		#pragma omp critical (forwardVisitedMap)
		forwardVisitedMap[node].insert(item);
	}

	bool backwardVisited(const SVFGNode* node) const {
		return backwardVisitedMap.find(node)!=backwardVisitedMap.end();
	}
	
	bool backwardVisited(const SVFGNode* node, const UARLiteDPItem& item) const {
		auto iter = backwardVisitedMap.find(node);
		if(iter!=backwardVisitedMap.end())
			return iter->second.find(item)!=iter->second.end();
		else
			return false;
	}

	void addBackwardVisited(const SVFGNode* node, const UARLiteDPItem& item) {
		#pragma omp critical (backwardVisitedMap)
		backwardVisitedMap[node].insert(item);
	}

	void clearVisitedMap() {
		forwardVisitedMap.clear();
		backwardVisitedMap.clear();
	}

private:
	/**
	 * Creats CallGraphs, PAG, FSPTA, SVFG and further initializations.
	 */
	void initialize(llvm::Module& module); 

	/**
	 * If the kernel_analysis flag is set, this analysis will use
	 * special variables. These are contained in the KernelContext container.
	 */
	void initKernelContext();

	/**
	 * Find all stack and global PAGNodes.
	 */
	void initSrcs();

	/** 
	 * Backwards through all OutofScopeNodes to find the "global" node. 
	 */
	void findDanglingPointers();

	/**
	 * Seeks the function path from the function of the local variable to
	 * the systemcall by backwarding the callgraph.
	 */
	StringList findAPIPath();

	/** 
	 * Converts a Set of PAGNodes into a Set of SVFGNodes.
	 */
	void convertPAGNodesToSVFGNodes(const PAGNodeSet &pagNodes, SVFGNodeSet &svfgNodes); 

	/** 
	 * Removes the irrelevant nodes like parameter and local pointer.
	 */
	void filterStackSVFGNodes(SVFGNodeSet &svfgNodes);

	/**
	 * Either insert the item into the thread lokalworklist or if the 
	 * itemDispatcherIndex is set, push it to the globalworklist.
	 */
	void pushIntoWorkList(const UARLiteDPItem &item);

	/** 
	 * Either pop the item from the thread lokalworklist or from the globalworklist.
	 */
	bool popFromWorkList(UARLiteDPItem &item, bool fifo);

	/**
	 * Prints a msg to each of the found bugs.
	 */
	void reportBugs();

	/**
	 * Checks if a node should be forwarded.
	 */
	bool forwardChecks(UARLiteDPItem &item); 

	/**
	 * Checks if a node should be backwarded.
	 */
	bool backwardChecks(UARLiteDPItem &item);

	/**
	 * Initializes the current programSlice.
	 */
	void setCurAnalysisCxt(const SVFGNode* src); 

	/**
	 * Stores the forwarded path up to the out of scpe node.
	 */
	void handleOosNode(const SVFGNode *node, const UARLiteDPItem &item) {
		#pragma omp critical (OoS)
		curAnalysisCxt->addOosNode(node);

		// saves the forwardslice for each OoSNode. If a OoSNode already exist
		// we merge the forward slices. 
		#pragma omp critical (ForwardPath)
		curAnalysisCxt->setOosForwardPath(node->getId(), item.getFuncPath());
	}

	/**
	 * Stores the backward path and saves the dangling poniter as bug.
	 */
	void handleDanglingPtr(const SVFGNode *node, const UARLiteDPItem &item) {
		const SVFGNode *srcNode = curAnalysisCxt->getSource();

		#pragma omp critical(DanglingPtr)
		if(curAnalysisCxt->addDanglingPtrNode(node)) {
			#pragma omp critical(BackwardPath)
			curAnalysisCxt->setDPtrBackwardPath(node->getId(), item.getFuncPath());

			handleBug(node, srcNode, item.getLoc());
		}
	}

	/**
	 * Creates for a given set (forward/backwardSlice) a map that contains the function
	 * names and the correspoding location of the function.
	 */
	void createFuncToLocationMap(const ContextCond &funcPath, StringToLoc &map);

	/**
	 * Creates a Bug object and adds it to a set of bugs and to the KMinerStatistics. 
	 */
	void handleBug(const SVFGNode *danglingPtrNode, const SVFGNode *localVarNode, const SVFGNode *oosNode);

	/**
	 * Checks if a call might be a sink.
	 */
	bool isSink(const SVFGNode *node) const;

	/**
	 * Finds all functions inside the module where the 
	 * name is contains the subname of a potential delete function.
	 */
	void findSinks();

	/**
	 * Imports a list of functions that should be interpreted as a sink.
	 */
	void importSinks(StringSet &sinks);

	/**
	 * Checks if the time limit has been reached.
	 */
	bool timeout() const {
		return omp_get_wtime()-start_t > max_t;
	}

	/**
	 * Resets the timer.
	 */
	void resetTimeout() {
		start_t = omp_get_wtime();
	}

	// Pointer Assignment Graph.
	const PAG *pag;

	// Pointer Analysis.
	AndersenWaveDiff* ander; 

	// Helper class for building the SVFG. 
    	KernelSVFGBuilder svfgBuilder;

	// Contains information that are relevant during the complete analysis.
	UARLiteAnalysisContext *curAnalysisCxt;		

	// Contains the local and global variables (SVFGNodes).
	SVFGNodeSet stackSVFGNodes, globalSVFGNodes;

	// The delete function names as llvm has set them.
	StringSet sinks;

	// Set of the bugs that were imported.
	UARBug::BugSet importedBugs;

	KernelContext *kCxt;

	const unsigned int num_threads;

	// Time limit.
	double max_t; 

	// Start of timer.
	double start_t;

	// Record forward visited items.
	SVFGNodeToDPItemsMap forwardVisitedMap;	
	SVFGNodeToDPItemsMap backwardVisitedMap;	
};

#endif // USE_AFTER_RETURN_CHECKER_LITE
