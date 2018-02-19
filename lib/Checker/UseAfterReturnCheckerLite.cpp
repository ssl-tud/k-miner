#include "Checker/UseAfterReturnCheckerLite.h"
#include "SVF/MSSA/SVFGStat.h"
#include "SVF/Util/GraphUtil.h"
#include "Util/KernelAnalysisUtil.h"
#include "Util/DebugUtil.h"
#include <ctime>
#include <iomanip>
#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>

using namespace llvm;
using namespace analysisUtil;
using namespace svfgAnalysisUtil;
using namespace debugUtil;

char UseAfterReturnCheckerLite::ID = 0;

static cl::opt<std::string> ImportSinks("uarl-sinks", cl::init(""),
		cl::desc("Imports function names that will be interpreted as a sink."));

static cl::opt<unsigned int> UARTimeout("uarl-timeout", cl::init(10),
		cl::desc("Timeout used for for-/backwardTraversion"));

static RegisterPass<UseAfterReturnCheckerLite> USEAFTERRETURNCHECKERLITE("use-after-return-checker-lite", "UseAfterReturnCheckerLite");

// If a thread is finish it sets the boolen at index=thread_id true.
// If all booleans are true the analysis is done.
BoolVec *sched_threads;

// The worklist of items the threads share.
UARLiteWorkList *global_worklist;

// The worklist of items that is private to its thread.
UARLiteWorkList *tlocal_worklist;

// The dispatcher is set by a thread if its local worklist is empty,
// in that case other threads will insert parts of their items into the
// global worklist.
unsigned int item_dispatcher_index;
#pragma omp threadprivate(tlocal_worklist)

void UseAfterReturnCheckerLite::analyze(llvm::Module& module) {
	omp_set_num_threads(num_threads);
	double analysisStart_t = omp_get_wtime();
	int i=0;

	initialize(module);

	// Only matters during the backward phase.
	ContextCond::setMaxCxtLen(10);

	for(const auto &iter : stackSVFGNodes) {
		double start;
		i++;

		global_worklist = new UARLiteWorkList();
		sched_threads = new BoolVec(num_threads);
		NodeID stackSvfgNodeId = iter->getId();
		setCurAnalysisCxt(iter);
		clearVisitedMap();

		ContextCond cxt;
		CxtVar var(cxt, stackSvfgNodeId);
		ContextCond funcPath;
		const LocCond *loc = &*iter;
		UARLiteDPItem item(var, loc, funcPath);

		start = omp_get_wtime();

		forwardTraverse(item);

		start = omp_get_wtime();

		// Backwards, starting by the out of scope nodes.
		findDanglingPointers();

		delete global_worklist;
		delete sched_threads;
	}
}

void UseAfterReturnCheckerLite::initialize(llvm::Module& module) { 
	initKernelContext();

	double init_t = omp_get_wtime();

	// Creates PAG, MemSSA, PTACallGraph, SVFG
	ander = AndersenWaveDiff::createAndersenWaveDiff(module);
	init_t = omp_get_wtime(); 
	
	SVFG *svfg = new SVFGOPT(ander->getPTACallGraph());
	init_t = omp_get_wtime(); 

	svfgBuilder.build(svfg,ander);

	setGraph(svfg);
	
	pag = PAG::getPAG();
	max_t = UARTimeout;

	findSinks();
	initSrcs();
}

void UseAfterReturnCheckerLite::initKernelContext() {
	kCxt = KernelContext::getKernelContext();
	assert(kCxt && "KernelContext not available");
}

void UseAfterReturnCheckerLite::initSrcs() {
	stackSVFGNodes = svfgBuilder.getLocalNodes();
	globalSVFGNodes = svfgBuilder.getGlobalNodes();
	filterStackSVFGNodes(stackSVFGNodes);
}

void UseAfterReturnCheckerLite::findDanglingPointers() {
	for(const auto &iter : curAnalysisCxt->getOosNodes()) {
		// There might be more dangline pointer. We have to backward the SVFG
		// starting from the outofScopeNode.
		NodeID oosNodeId = iter->getId();
		ContextCond cxt;
		CxtVar var(cxt, oosNodeId);
		ContextCond funcPath;
		const LocCond *loc = getNode(oosNodeId);
		UARLiteDPItem item(var, loc, funcPath); 

		backwardTraverse(item);
	}
}

inline void UseAfterReturnCheckerLite::forwardProcess(const UARLiteDPItem& item) {
}

void UseAfterReturnCheckerLite::forwardTraverse(UARLiteDPItem& it) {
	global_worklist->clear();
	global_worklist->push_back(it);

	resetTimeout();

	#pragma omp parallel
	{
	assert(num_threads == omp_get_num_threads() && "Omp didn't set the correct number of threads");
	tlocal_worklist = new UARLiteWorkList();

	while(true) {
		ContextCond cxt;
		CxtVar var(cxt, 0);
		ContextCond funcPath;
		const LocCond *loc = nullptr;
		UARLiteDPItem item(var, loc, funcPath);

		bool finish = false;
		bool threadWait = false;

		if(!popFromWorkList(item, true)) {
			finish = true;
			#pragma omp critical (sched_threads)
			sched_threads->at(omp_get_thread_num()) = true;
			item_dispatcher_index = 1;

			#pragma omp critical (sched_threads)
			for(auto iter = sched_threads->begin(); iter != sched_threads->end(); ++iter)
				if(!*iter) {
					finish = false;
					threadWait = true;
				}
		}

		if(finish || timeout())
			break;
		else if(threadWait)
			continue;

		const SVFGNode* curNode = getNode(item.getCurNodeID());
		const SVFGNode* prevNode = getNode(item.getPrevNodeID());

		forwardProcess(item);

		GNODE* v = getNode(item.getCurNodeID());
		auto EI = GTraits::child_begin(v);
		auto EE = GTraits::child_end(v);

		for (; EI != EE; ++EI) {
			forwardpropagate(item,*(EI.getCurrent()));
		}
	}

	delete tlocal_worklist;
	}
}

bool UseAfterReturnCheckerLite::forwardChecks(UARLiteDPItem &item) {
	const SVFGNode* curNode = getNode(item.getCurNodeID());
	const SVFGNode* prevNode = getNode(item.getPrevNodeID());
	const StmtSVFGNode *curStmtNode = dyn_cast<StmtSVFGNode>(curNode);
	const StmtSVFGNode *prevStmtNode = dyn_cast<StmtSVFGNode>(prevNode);
	const llvm::Function *F = getSVFGFunction(curNode);

	if(curNode->getId() == prevNode->getId())
		return false;

	if(isa<StoreSVFGNode>(curNode)) {
		if(!item.hasFirstAsgnmVisited()) {
			if(!isParamStore(curNode)) 
				item.firstAsgnmVisited();
		} else if(!item.isDeref()) {
			return false;	
		}

		item.unsetDeref();	
	} 

	if(curStmtNode && prevStmtNode) {
		const PAGNode *curPagSrcNode = curStmtNode->getPAGSrcNode();
		const PAGNode *curPagDstNode = curStmtNode->getPAGDstNode();
		const PAGNode *prevPagSrcNode = prevStmtNode->getPAGSrcNode();
		const PAGNode *prevPagDstNode = prevStmtNode->getPAGDstNode();

		if(isa<StoreSVFGNode>(curNode)) {
			if(isa<LoadSVFGNode>(prevNode) || isa<GepSVFGNode>(prevNode) ||isa<AddrSVFGNode>(prevNode))
				// Something is put into the current stack variable.
				if(prevPagDstNode->getId() != curPagSrcNode->getId()) {
					return false;
				}
		} else if(isa<LoadSVFGNode>(curNode) && 
				(isa<AddrSVFGNode>(prevNode) || isa<GepSVFGNode>(prevNode))) {
			return false;
		}
	}

	if(isa<CopySVFGNode>(prevNode) && !getGraph()->getSVFGEdge(prevNode, curNode, SVFGEdge::SVFGEdgeK::DirRet)) {
		// This will only copy the value.
		return false;
	} 

	// Has to be at the end
	if(isa<LoadSVFGNode>(curNode))
		item.setDeref();

	return true;
}

void UseAfterReturnCheckerLite::forwardpropagate(const UARLiteDPItem& item, GEDGE* edge) {
	const SVFGNode* curNode = edge->getSrcNode();
	const SVFGNode* nextNode = edge->getDstNode();
	UARLiteDPItem newItem(item);
	CallSiteID csId = 0;
	newItem.setCurNodeID(edge->getDstID());
	newItem.setPrevNodeID(item.getCurNodeID());

	if (edge->isCallVFGEdge()) {
		if(isSink(nextNode))
			return;

		if(const CallDirSVFGEdge* callEdge = dyn_cast<CallDirSVFGEdge>(edge))
			csId = callEdge->getCallSiteId();
		else
			csId = cast<CallIndSVFGEdge>(edge)->getCallSiteId();

		newItem.pushContext(csId);
		newItem.addFuncEntry(nextNode->getId());

	} else if (edge->isRetVFGEdge()) {
		if(const RetDirSVFGEdge* callEdge = dyn_cast<RetDirSVFGEdge>(edge))
			csId = callEdge->getCallSiteId();
		else
			csId = cast<RetIndSVFGEdge>(edge)->getCallSiteId();

		newItem.addFuncReturn(nextNode->getId());

		// We return from a function we never entered. This is a potential bug.
		// Will be found/handled if forwarded.
		if(newItem.getCond().cxtSize() == 0) {
			handleOosNode(nextNode, newItem);
			return;
		} else if (newItem.matchContext(csId) == false)
			return;
	}

	if(!forwardChecks(newItem))
		return;

	if(forwardVisited(nextNode, newItem))
		return;
	
	addForwardVisited(nextNode, newItem);

	pushIntoWorkList(newItem);
}

void UseAfterReturnCheckerLite::pushIntoWorkList(const UARLiteDPItem &item) {
	if(item_dispatcher_index == 0) {
		tlocal_worklist->push_back(item);
	} else {
		#pragma omp critical (global_worklist)
		global_worklist->push_back(item);
	}
}

bool UseAfterReturnCheckerLite::popFromWorkList(UARLiteDPItem &item, bool fifo) {
	bool res = false;

	if(!tlocal_worklist->empty()) {
		if(fifo) {
			item = tlocal_worklist->front();
			tlocal_worklist->pop_front();
		} else {
			item = tlocal_worklist->back();
			tlocal_worklist->pop_back();
		}
		res = true;
	} else {
		#pragma omp critical (global_worklist)
		{
		if(!global_worklist->empty()) {
			if(fifo) {
				item = global_worklist->front();
				global_worklist->pop_front();
			} else {
				item = global_worklist->back();
				global_worklist->pop_back();
			}
			res = true;
			#pragma omp critical (sched_threads)
			sched_threads->at(omp_get_thread_num()) = false;
			item_dispatcher_index = 0;
		} 
		}
	}

	return res;
}

inline void UseAfterReturnCheckerLite::backwardProcess(const UARLiteDPItem& item) {
	const SVFGNode *srcNode = curAnalysisCxt->getSource();
	const SVFGNode *curNode = getNode(item.getCurNodeID());

	// The current node might be a dangling pointer.
	if(isa<AddrSVFGNode>(curNode)) {
		if(globalSVFGNodes.find(curNode) != globalSVFGNodes.end()) {
			handleDanglingPtr(curNode, item);
		} else if(stackSVFGNodes.find(curNode) != stackSVFGNodes.end() && item.getCond().cxtSize() == 0) {
			// The dangling pointer cant be be our local node we started with.
			if(curNode->getId() != srcNode->getId()) 
				handleDanglingPtr(curNode, item);
		}
	}
}

void UseAfterReturnCheckerLite::backwardTraverse(UARLiteDPItem& it) {
	resetTimeout();
	global_worklist->clear();
	global_worklist->push_back(it);

	#pragma omp parallel
	{
	assert(num_threads == omp_get_num_threads() && "Omp didn't set the correct number of threads");
	tlocal_worklist = new UARLiteWorkList();

	while (true) {
		ContextCond cxt;
		CxtVar var(cxt, 0);
		ContextCond funcPath;
		const LocCond *loc = nullptr;
		UARLiteDPItem item(var, loc, funcPath);

		bool finish = false;
		bool threadWait = false;

		if(!popFromWorkList(item, false)) {
			finish = true;
			#pragma omp critical (sched_threads)
			sched_threads->at(omp_get_thread_num()) = true;
			item_dispatcher_index = 1;

			#pragma omp critical (sched_threads)
			for(auto iter = sched_threads->begin(); iter != sched_threads->end(); ++iter)
				if(!*iter) {
					finish = false;
					threadWait = true;
				}
		}

		if(finish || timeout())
			break;
		else if(threadWait)
			continue;

		const SVFGNode *curNode = getNode(item.getCurNodeID());
		const SVFGNode *prevNode = getNode(item.getPrevNodeID());

		backwardProcess(item);

		GNODE* v = getNode(item.getCurNodeID());
		// GNODE* v = getNode(item->getCurNodeID());
            	auto EI = InvGTraits::child_begin(v);
            	auto EE = InvGTraits::child_end(v);

            	for (; EI != EE; ++EI) {
                	backwardpropagate(item,*(EI.getCurrent()) );
            	}
		
	}

	delete tlocal_worklist;
	}
}

bool UseAfterReturnCheckerLite::backwardChecks(UARLiteDPItem &item) {
	const SVFGNode *curNode = getNode(item.getCurNodeID());
	const SVFGNode *prevNode = getNode(item.getPrevNodeID());
	const StmtSVFGNode *curStmtNode = dyn_cast<StmtSVFGNode>(curNode);
	const StmtSVFGNode *prevStmtNode = dyn_cast<StmtSVFGNode>(prevNode);

	if(isa<NullPtrSVFGNode>(curNode)) 
		return false;

	if(curNode->getId() == prevNode->getId())
		return false;

	if(curStmtNode && prevStmtNode) {
		const PAGNode *curPagSrcNode = curStmtNode->getPAGSrcNode();
		const PAGNode *curPagDstNode = curStmtNode->getPAGDstNode();
		const PAGNode *prevPagSrcNode = prevStmtNode->getPAGSrcNode();
		const PAGNode *prevPagDstNode = prevStmtNode->getPAGDstNode();

		if(!isa<StoreSVFGNode>(curNode) && isa<StoreSVFGNode>(prevNode)) {
			if(!item.isDeref() && curPagDstNode->getId() != prevPagDstNode->getId())
				return false;
			if(item.isDeref() && curPagDstNode->getId() != prevPagSrcNode->getId())
				return false;
		}
				
		if(isa<StoreSVFGNode>(curNode) && isa<StoreSVFGNode>(prevNode)) {
			if(!isParamStore(curNode))
				return false;
		} else if(isa<GepSVFGNode>(curNode) && isa<LoadSVFGNode>(prevNode)) {
			if(curPagDstNode->getId() != prevPagSrcNode->getId())
				return false;
		} else if(isa<AddrSVFGNode>(curNode) && isa<LoadSVFGNode>(prevNode)) {
			return false;
		} else if(isa<LoadSVFGNode>(curNode) && isa<LoadSVFGNode>(prevNode)) {
			return false;
		}
	} 

	// The Ptr is set to NULL.
	if(isa<StoreSVFGNode>(curNode) && !isParamStore(curNode)) {
		if(!item.hasFirstAsgnmVisited()) {
			if(forwardVisited(curNode))
				item.firstAsgnmVisited();
			else
				return false;
		} else if(!item.isDeref()) {
			return false;	
		}
	} else if(!item.hasFirstAsgnmVisited()) {
		if(!forwardVisited(curNode))
			return false;
	}

	if(isa<StoreSVFGNode>(prevNode))
		item.unsetDeref();

	// Has to be at the end
	if(isa<LoadSVFGNode>(curNode))
		item.setDeref();

	return true;
}

void UseAfterReturnCheckerLite::backwardpropagate(const UARLiteDPItem& item, GEDGE* edge) {
	const SVFGNode* curNode = edge->getDstNode();
	const SVFGNode* nextNode = edge->getSrcNode();
	CallSiteID csId;

	UARLiteDPItem newItem(item);
	newItem.setCurNodeID(edge->getSrcID());
	newItem.setPrevNodeID(item.getCurNodeID());


	if (edge->isCallVFGEdge()) {
		if(const CallDirSVFGEdge* callEdge = dyn_cast<CallDirSVFGEdge>(edge))
			csId = callEdge->getCallSiteId();
		else
			csId = cast<CallIndSVFGEdge>(edge)->getCallSiteId();

		if(!newItem.matchContext(csId))
			return;

		newItem.addFuncEntry(curNode->getId());

		// Initcalls will be the last functions.
		if(kCxt->isInitcall(getSVFGFunction(curNode)))
			return;
	} else if (edge->isRetVFGEdge()) {
		if(const RetDirSVFGEdge* callEdge = dyn_cast<RetDirSVFGEdge>(edge))
			csId = callEdge->getCallSiteId();
		else
			csId = cast<RetIndSVFGEdge>(edge)->getCallSiteId();

		if(isSink(nextNode))
			return;

		newItem.pushContext(csId);
		newItem.addFuncReturn(curNode->getId());
	}

	if(!backwardChecks(newItem))
		return;

	// If the current path contains the next node, we wont backward it. 
	if(backwardVisited(nextNode, newItem))
		return;
	else
		addBackwardVisited(nextNode, newItem);

	pushIntoWorkList(newItem);
}

void UseAfterReturnCheckerLite::createFuncToLocationMap(const ContextCond &funcPath, StringToLoc &map) {
	for(const auto &iter : funcPath) {
		const SVFGNode *node = getNode(iter);
		const llvm::BasicBlock *BB = node->getBB();

		if(BB == nullptr) 
			continue;

		const llvm::Function *F = BB->getParent();

		if(F == nullptr) 
			continue;

		std::string funcName = F->getName();

		if(map.find(funcName) != map.end() && funcName != "")
			continue;

		Location loc(getSourceFileNameOfFunction(F), funcName, getSourceLineOfFunction(F));
		map[funcName] = loc;
	}
}

StringList UseAfterReturnCheckerLite::findAPIPath() {
	StringList syscallPath = curAnalysisCxt->getAPIPath();
	std::string funcName = getSVFGFuncName(curAnalysisCxt->getSource());
	StringSet apiNames = kCxt->getAPIFunctions();

	if(!syscallPath.empty())
		return syscallPath;

	LocalCallGraphAnalysis LCGA(ander->getPTACallGraph());
	// Dont backward further than the systemcall.
	LCGA.setBackwardFilter(apiNames);
	LCGA.analyze(funcName, false, false, std::numeric_limits<uint32_t>::max());
	const StrListSet &paths = LCGA.getBackwardFuncPaths();

	for(const auto &iter : paths) {
		const StringList &path = iter;
		std::string srcFunc = path.back();

		if(kCxt->isAPIFunction(srcFunc)) {
			syscallPath = path;		
			break;
		}
	}

	assert(kCxt->inAPIContext(funcName) && "no API function");

	assert(!syscallPath.empty() && "No path to the systemcall found");

	curAnalysisCxt->setAPIPath(syscallPath);

	return syscallPath;
}

void UseAfterReturnCheckerLite::handleBug(const SVFGNode *danglingPtrNode, const SVFGNode *localVarNode, 
				const SVFGNode *oosNode) {
	double t = omp_get_wtime() - curAnalysisCxt->getStartTime();

	StringList syscallPath = findAPIPath();

	if(syscallPath.empty())
		return;

	StringToLoc visitedFuncToLocMap;
	ContextCond forwardPath = curAnalysisCxt->getOosForwardPath(oosNode->getId());
	ContextCond backwardPath = curAnalysisCxt->getDPtrBackwardPath(danglingPtrNode->getId());
	std::reverse(backwardPath.getContexts().begin(), backwardPath.getContexts().end());

	Location danglingPtrLoc(getSVFGSourceFileName(danglingPtrNode), 
				getSVFGFuncName(danglingPtrNode), 
				getSVFGSourceLine(danglingPtrNode),
				getSVFGValueName(danglingPtrNode)); 

	Location localVarLoc(getSVFGSourceFileName(localVarNode), 
			     getSVFGFuncName(localVarNode), 
			     getSVFGSourceLine(localVarNode), 
			     getSVFGValueName(localVarNode));

	// Fills the maps with the name of the functions (visited during the analysis) 
	// and the correspoding location of the functions.
	createFuncToLocationMap(forwardPath, visitedFuncToLocMap);
	createFuncToLocationMap(backwardPath, visitedFuncToLocMap);

	UARBug *bug = new UARBug();
	bug->setAPIPath(syscallPath);
	bug->setDanglingPtrLocation(danglingPtrLoc);
	bug->setLocalVarLocation(localVarLoc); 
	bug->setVisitedFuncToLocMap(visitedFuncToLocMap);
	bug->setLocalVarFuncPathStr(getFuncPathStr(getGraph(), forwardPath, true));
	bug->setDanglingPtrFuncPathStr(getFuncPathStr(getGraph(), backwardPath, true));
	bug->setDuration(t);

	#pragma omp critical(addBug)
	addBug(bug);
}

void UseAfterReturnCheckerLite::convertPAGNodesToSVFGNodes(const PAGNodeSet &pagNodes, SVFGNodeSet &svfgNodes) {
	const SVFGNode *svfgNode;

	for(auto pagNodesIter = pagNodes.begin(); pagNodesIter != pagNodes.end(); ++pagNodesIter) {
		svfgNode = getGraph()->getDefSVFGNode(*pagNodesIter);
		svfgNodes.insert(svfgNode);
	}
}

void UseAfterReturnCheckerLite::filterStackSVFGNodes(SVFGNodeSet &svfgNodes) {
	const SVFGNode *svfgNode;
	const llvm::Function *F;

	auto svfgNodeIterB = svfgNodes.begin();
	auto svfgNodeIterE = svfgNodes.end();

	while(svfgNodeIterB != svfgNodeIterE) {
		svfgNode = *svfgNodeIterB;
		F = getSVFGFunction(svfgNode);

		if(!F || !F->hasName() || !kCxt->inAPIContext(F->getName())) {
			svfgNodeIterB = svfgNodes.erase(svfgNodeIterB);
			continue;
		}
		
		if(isParamAddr(svfgNode) || !svfgNode->hasOutgoingEdge() || isArg(svfgNode)
			/*|| !GraphMinimizer::isSyscallRelevantFunction(F))*/)
			svfgNodeIterB = svfgNodes.erase(svfgNodeIterB);
		else
			++svfgNodeIterB;
	}
}

void UseAfterReturnCheckerLite::findSinks() {
	StringSet deleteFunctions;	

	importSinks(deleteFunctions);

	for(auto iter = deleteFunctions.begin(); iter != deleteFunctions.end(); ++iter) {
		std::string funcSubName = *iter; 

		for(auto iter2 = getModule().begin(); iter2 != getModule().end(); ++iter2) {
			const llvm::Function *F = &*iter2;

			if(F == nullptr)
				continue;

			std::string funcName = F->getName();

			if(funcName.find(funcSubName) != std::string::npos)
				sinks.insert(funcName);
		}
	}
}

void UseAfterReturnCheckerLite::importSinks(StringSet &sinks) {
	std::string fileName = ImportSinks;

	if(fileName != "") {
		std::ifstream ifs(fileName);

		assert(ifs.good() && "No sink file found");

		std::stringstream buffer;
		buffer << ifs.rdbuf();
		std::string funcName;

		while(std::getline(buffer, funcName, '\n')) {
			sinks.insert(funcName);
		}
	}
}

bool UseAfterReturnCheckerLite::isSink(const SVFGNode *node) const {
	const llvm::Function *F = getSVFGFunction(node);

	if(!F)
		return false;

	return sinks.find(F->getName()) != sinks.end();
}

void UseAfterReturnCheckerLite::setCurAnalysisCxt(const SVFGNode* src) {
	if(curAnalysisCxt!=nullptr) {
		delete curAnalysisCxt;
		curAnalysisCxt = nullptr;
	}

	curAnalysisCxt = new UARLiteAnalysisContext(src, getGraph());
}
