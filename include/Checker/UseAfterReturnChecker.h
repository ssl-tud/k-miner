#ifndef USE_AFTER_RETURN_CHECKER
#define USE_AFTER_RETURN_CHECKER

#include "Checker/KernelCheckerAPI.h"
#include "Checker/KernelChecker.h"
#include "Util/KCFSolver.h"
#include "Util/AnalysisContext.h"
#include "Util/KMinerStat.h"
#include "Util/DirRetAnalysis.h"
#include "Util/CallGraphAnalysis.h"
#include "Util/Bug.h"
#include "KernelModels/KernelSVFGBuilder.h"
#include "KernelModels/KernelContext.h"
#include "SVF/MemoryModel/PAG.h"
#include "SVF/MSSA/SVFGOPT.h"
#include "SVF/WPA/Andersen.h"
#include "SVF/WPA/FlowSensitive.h"
#include <string>
#include <sstream>
#include <fstream>
#include <thread>
#include <ctime>

typedef std::set<NodeID> NodeIdSet;
typedef KCFSolver<SVFG*,UARDPItem> Solver;
typedef std::list<UARDPItem> UARWorkList;
typedef std::vector<bool> BoolVec;

/***
 * A SVF-Analysis to find use-after-return bugs in C-Programs.
 */
class UseAfterReturnChecker : public Solver, public KernelChecker {
public:
	typedef std::set<const SVFGNode*> SVFGNodeSet;
	typedef std::set<const PAGNode*> PAGNodeSet;
	typedef std::set<std::string> StringSet;
	typedef std::list<std::string> StringList;
	typedef std::pair<NodeID, NodeID> CxtNodeIdVar;
	typedef std::set<UARDPItem> ItemSet;
	typedef std::map<CxtNodeIdVar, ItemSet> CxtToItemSet;
	typedef std::map<std::string, int> StringToBool;

	static char ID;

	UseAfterReturnChecker(unsigned int num_threads=1, char id = ID): 
		KernelChecker("Use-After-Return", ID), num_threads(num_threads), 
		curAnalysisCxt(nullptr), pag(nullptr), max_t(300), start_t(0) { 
	}

	virtual ~UseAfterReturnChecker() {
		if(pag)
			PAG::releasePAG();

		if(dra)
			delete dra;
		dra = nullptr;
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
	virtual inline void forwardProcess(const UARDPItem& item); 

	/**
	 * If a AddrSVFGNode was reached during backwarding this node
	 * will be handled as a dangling pointer.
	 */
	virtual inline void backwardProcess(const UARDPItem& item);

	/**
	 * Starting by a local variable the function forwards through the SVFG.
	 * Some optimations were added in order to improve the performance with omp.
	 */
    	virtual void forwardTraverse(UARDPItem& it); 
	
	/**
	 * Starting by a node that left the valid scope of the forwarded variable,
	 * this function start backwarding the SVFG.
	 * Some optimations were added in order to improve the performance with omp.
	 */
	virtual void backwardTraverse(UARDPItem& it); 

	/**
	 * Forward propagates the item in a context sensitive way.
	 * Function paths are only forwarded once.
	 */
	virtual void forwardpropagate(const UARDPItem& item, SVFGEdge* edge);

	/**
	 * Backward propagates the item in a context sensitive way.
	 */
	virtual void backwardpropagate(const UARDPItem& item, SVFGEdge* edge);

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
	 * A function entry that is visited the first time by a path will be forwarded by a
	 * new item seperatlly. All items that visit the same function entry later on will either
	 * merge their paths with the already finished function paths or wait till a function item
	 * finished a path, in this cased the waiting item will be also merged with the return node.
	 * Sinks (delete functions) wont be forwarded.
	 */
	void handleFunctionEntry(UARDPItem &funcItem, GEDGE *edge);

	/** 
	 * If a function item reaches an return, it merges the function path with the paths of
	 * the waiting items paths. The function also checks if the return node has a valid scope.
	 */
	void handleFunctionReturn(UARDPItem &retItem, GEDGE *edge); 

	/** 
	 * If we are about to forward into a function we already visited with the same context
	 * use the stored information to skip the whole function.
	 */
	void handleVisitedFunction(const UARDPItem &item, CallSiteID csId);

	/**
	 * If we are about to forward into a function we already visited with the same context
	 * use the stored information to skip the whole function. The optimized version merges
	 * paths with the same function entry node, context and return node.
	 */
	void handleVisitedFunctionOpt(const UARDPItem &item, CallSiteID csId);

	/**
	 * During the handling of the function that returns now it might be possible that
	 * another item reached the function entry node and was added to a waiting list. 
	 */
	void handleWaitingFuncEntryNodes(const UARDPItem &funcRetItem, CallSiteID csId);

	/**
	 * Either insert the item into the thread lokalworklist or if the 
	 * itemDispatcherIndex is set, push it to the globalworklist.
	 */
	void pushIntoWorkList(const UARDPItem &item);

	/** 
	 * Either pop the item from the thread lokalworklist or from the globalworklist.
	 */
	bool popFromWorkList(UARDPItem &item, bool fifo);

	/**
	 * Checks if a node should be forwarded.
	 */
	bool forwardChecks(UARDPItem &item); 

	/**
	 * Checks if a node should be backwarded.
	 */
	bool backwardChecks(UARDPItem &item);

	/**
	 * Initializes the current programSlice.
	 */
	void setCurAnalysisCxt(const SVFGNode* src); 

	/**
	 * Stores the forwarded path up to the out of scpe node.
	 */
	void handleOosNode(const SVFGNode *node, const UARDPItem &item) {
		#pragma omp critical (OoS)
		{
		bool isNewOos = false;

		if(item.isDirRet())
			isNewOos = curAnalysisCxt->addDirOosNode(node);
		else
			isNewOos = curAnalysisCxt->addOosNode(node);
		}

		// saves the forwardslice for each OoSNode. If a OoSNode already exist
		// we merge the forward slices. 
		#pragma omp critical (ForwardSlice)
		curAnalysisCxt->addOosForwardSlice(node->getId(), item.getCxtVisitedNodes());

		#pragma omp critical (ForwardPath)
		curAnalysisCxt->addOosForwardPath(node->getId(), item.getVisitedPath());
	}

	/**
	 * Stores the backward path and saves the dangling poniter as bug.
	 */
	void handleBug(const SVFGNode *danglingPtrNode, 
			       const SVFGNode *oosNode, 
			       CxtNodeIdList backwardPath);

	/**
	 * Returns the return items of a function visited in a given context.
	 */
	const ItemSet& getFuncRetItems(NodeID funcEntryNodeId, CallSiteID csId) const {
		CxtNodeIdVar cxt(csId, funcEntryNodeId);
		return cxtToFuncRetItems.at(cxt);
	}

	/**
	 * Checks if the function entry node was already visited by another item.
	 */
	bool isVisitedFuncEntryNode(NodeID funcEntryNodeId, CallSiteID csId) const {
		CxtNodeIdVar cxt(csId, funcEntryNodeId);
		return cxtToFuncRetItems.find(cxt) != cxtToFuncRetItems.end();	
	}

	/**
	 * Checks if the function return node was visited by another item.
	 */
	bool hasFuncCxtRetItem(NodeID funcEntryNodeId, CallSiteID csId, 
			const UARDPItem &funcRetItem) const {
		CxtNodeIdVar cxt(csId, funcEntryNodeId);
		auto iter = cxtToFuncRetItems.find(cxt);

		if(iter == cxtToFuncRetItems.end())
			return false;

 		const ItemSet &retItems = (*iter).second;
		
		return retItems.find(funcRetItem) != retItems.end();		
	}

	/**
	 * Adds a return item with a specific context to a global map.
	 */
	void addRetToFuncCxt(NodeID funcEntryNodeId, CallSiteID csId, const UARDPItem &funcRetItem){
		CxtNodeIdVar cxt(csId, funcEntryNodeId);
		cxtToFuncRetItems[cxt].insert(funcRetItem);
	}

	/**
	 * Returns the number of return itemes for a given function entry an context.
	 */
	size_t getRetToFuncCxtSize(NodeID funcEntryNodeId, CallSiteID csId) {
		CxtNodeIdVar cxt(csId, funcEntryNodeId);
		return cxtToFuncRetItems[cxt].size();
	}

	/**
	 * If a node was forwarded completely, then all functions were finished (if no timeout).
	 */
	void setCompletedFunctions() {
		for(auto iter = cxtToFuncRetItems.begin(); iter != cxtToFuncRetItems.end(); ++iter)
			completedFunctions.insert(iter->first.second);
	}

	/**
	 * Checks if a function entry was already visited during a previous forwarding process.
	 */
	bool isFunctionCompleted(NodeID funcEntryNodeId) const {
		return completedFunctions.find(funcEntryNodeId) != completedFunctions.end();
	}

	/**
	 * Creates for a given set (forward/backwardSlice) a map that contains the function
	 * names and the correspoding location of the function.
	 */
	//void createFuncToLocationMap(const CxtNodeIdSet &slice, StringToLoc &map);
	void createFuncToLocationMap(const CxtNodeIdList &path, StringToLoc &map);

	/**
	 * Checks if for a given node a bug alreaf was found.
	 */
	bool inImportedBugs(const SVFGNode *node);

	/**
	 * Imports previously found bugs.
	 */
	void importBugs();

	/**
	 * Exports the imported and the new found bugs.
	 */
	void exportBugs();

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

	// Finds the variable a return value is assigned to.
	DirRetAnalysis *dra;

	// Contains information that are relevant during the complete analysis.
	UARAnalysisContext *curAnalysisCxt;		

	// Contains the local and global variables (SVFGNodes).
	SVFGNodeSet stackSVFGNodes, globalSVFGNodes;

	// Maps a function entry node and its context to a set of corresponding return nodes.
	CxtToItemSet cxtToFuncRetItems;
	NodeIdSet completedFunctions;

	// The delete function names as llvm has set them.
	StringSet sinks;

	// Set of the bugs that were imported.
	UARBug::BugSet importedBugs;

	// Contains kernel related informations.
	KernelContext *kCxt;

	// Caches the information if a node is reachable from the current systemcall.
	StringToBool inSyscallPath;

	// Number of threads the analysis should run with.
	const unsigned int num_threads;

	// Time limit.
	double max_t; 

	// Start of timer.
	double start_t;
};

#endif // USE_AFTER_RETURN_CHECKER
