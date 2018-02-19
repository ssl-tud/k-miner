#ifndef CALLGRAPH_ANALYSIS_H_
#define CALLGRAPH_ANALYSIS_H_

#include "Util/KCFSolver.h"
#include "Util/KDPItem.h"
#include "SVF/Util/PTACallGraph.h"
#include "SVF/Util/AnalysisUtil.h"
#include "Util/KMinerStat.h"

using namespace llvm;

typedef KCFSolver<PTACallGraph*, CGDPItem> LCGASolver;
typedef std::set<std::string> StringSet;
typedef std::list<string> StringList;
typedef std::set<StringList> StrListSet;
typedef std::set<const PTACallGraphNode*> CGNodeSet;
typedef std::set<const llvm::Function*> FuncSet;
typedef std::set<const llvm::Instruction*> InstSet;
typedef std::map<std::string, InstSet> StrToInstSetMap;

/***
 * Iterates above the PTACallGraph, starting from a given function, and
 * collects all dependent functions by their names.
 */
class LocalCallGraphAnalysis : public LCGASolver {
private:
	/**
	 * Handle the recursive constant express case
	 */
	void handleGlobalCE(const CGDPItem &item, const GlobalVariable *G);
	void handleCE(const CGDPItem &item, const Value *val);
	void handleGlobalInitializerCE(const CGDPItem &item, const Constant *C, u32_t offset);

	/**
	 * It converts the NodeIdList (item path) info a list of function names.
	 */
	void saveFuncPath(const CGDPItem &item, bool forward);

	/**
	 * Scans the arguments of a callsite for global variables and function pointers.
	 */
	void handleCallSite(const CGDPItem &item, llvm::CallSite *CS);

	/** 
	 * Scans an instruction for global variables and function pointers. 
	 */
	void handleAssignments(const CGDPItem &item, llvm::Instruction *inst);

	/** 
	 * If the function we start with is an alias we need to find the corresponding
	 * function. This is necessary if we want to determine system calls for example.
	 */
	llvm::Function* findAliasFunc(std::string funcName);

public:
	LocalCallGraphAnalysis(PTACallGraph *callGraph):
       			broad(true), forwardFilter(nullptr), backwardFilter(nullptr), 
			actualDepth(0) { 
		setGraph(callGraph);
		kminerStat = KMinerStat::createKMinerStat();
		importBlackList();
	}

	virtual ~LocalCallGraphAnalysis() { }

	const PTACallGraph* getPTACallGraph() const {
		return getGraph();
	}

	/**
	 * Resets all variables.
	 */
	void reset() {
		actualDepth = 0;
		relevantGlobalVars.clear();
		callerToCallSiteInst.clear();
		forwardFuncSlice.clear();
		forwardFuncPaths.clear();
		backwardFuncSlice.clear();
		backwardFuncPaths.clear();
	}

	/**
	 * Starts by a given function and forwards or backwards the callgraph up to the
	 * given maxDepth. All the visited functions and used global variables are stored.
	 * If the broad flag is set the analysis will treat a found function pointer as a function call
	 * even if this not the case. This is not necessary if the callgraph was improved by a pointer
	 * analysis (like andersen). 
	 */
	void analyze(std::string startFunc, bool forward=true, bool broad=true, uint32_t maxDepth=0);

	const StringSet& getForwardFuncSlice() const;
	const StringSet& getBackwardFuncSlice() const;
	const StrListSet& getForwardFuncPaths() const;
	const StrListSet& getBackwardFuncPaths() const;
	const StringSet& getRelevantGlobalVars() const;
	const InstSet& getCallSiteInstSet(std::string callerName);

	virtual void forwardTraverse(CGDPItem& it); 

	virtual void forwardProcess(const CGDPItem& item);
	
	bool forwardChecks(const CGDPItem &item);

	virtual void forwardpropagate(const CGDPItem& item, GEDGE* edge);

	void forwardpropagate(const CGDPItem& item, FuncSet &functions);
	
	void forwardpropagate(const CGDPItem& item, const PTACallGraphNode *node);

	bool backwardCheck(const CGDPItem &item);

	virtual void backwardTraverse(CGDPItem& it); 

	virtual void backwardProcess(const CGDPItem& item);

	virtual void backwardpropagate(const CGDPItem& item, GEDGE* edge);
	
	void addVisited(const PTACallGraphNode* node) {
		visitedSet.insert(node);
	}

	bool hasVisited(const PTACallGraphNode* node) {
		return visitedSet.find(node)!=visitedSet.end();
	}

	void clearVisitedSet() {
		visitedSet.clear();
	}

	void setForwardFilter(const StringSet &filter) {
		forwardFilter = &filter;
	}

	void setBackwardFilter(const StringSet &filter) {
		backwardFilter = &filter;
	}

	uint32_t getActualDepth() const {
		return actualDepth;
	}

	bool isRelevantGlobalVar(const llvm::Value *v) const;

	bool isFunctionPtr(const llvm::Value *v) const;

	bool isPointer(const llvm::Value *v) const;

	bool hasPointer(const llvm::Value *v) const;

	bool structContainsPointer(llvm::StructType *structTy) const;

	bool inBlackList(std::string funcName) const;

	bool hasBlackList() const;

	void importBlackList();

private:
	// The number of nodes the graph was forwarded/backwarded.
	uint32_t actualDepth;

	// If set, functions that are assigned as a pointer will be considered 
	// as used and for/backwarded.
	bool broad;

	// A set of strings. Functions containing such a string will be ignored.
	static StringSet blacklist;

	StringSet relevantGlobalVars;
	StringSet forwardFuncSlice;
	StrListSet forwardFuncPaths;
	StringSet backwardFuncSlice;
	StrListSet backwardFuncPaths;

	const StringSet *forwardFilter;
	const StringSet *backwardFilter;
	CGNodeSet visitedSet;

	// Maps the callsite instructions of a funtion.
	StrToInstSetMap callerToCallSiteInst;

	KMinerStat *kminerStat;
};

#endif // CALLGRAPH_ANALYSIS_H_
