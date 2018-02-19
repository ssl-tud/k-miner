#include "Checker/UseAfterReturnChecker.h"
#include "SVF/MSSA/SVFGStat.h"
#include "SVF/WPA/Andersen.h"
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

char UseAfterReturnChecker::ID = 0;

static cl::opt<std::string> ImportBugs("import-bugs", cl::init(""),
		cl::desc("Imports previously found bugs."));

static cl::opt<std::string> ExportBugs("export-bugs", cl::init(""),
		cl::desc("Exports the bugs the analysis found."));

static cl::opt<std::string> ImportSinks("uar-sinks", cl::init("kernel_delete_funcs.txt"),
		cl::desc("Imports function names that will be interpreted as a sink."));

static cl::opt<unsigned int> UARTimeout("uar-timeout", cl::init(11),
		cl::desc("Timeout used for for-/backwardTraversion"));

static RegisterPass<UseAfterReturnChecker> USEAFTERRETURNCHECKER("use-after-return-checker",
								   "UseAfterReturnChecker");

#define IMPORTBUGS ImportBugs != ""
#define EXPORTBUGS ExportBugs != ""

// If a thread is finish it sets the boolen at index=thread_id true.
// If all booleans are true the analysis is done.
BoolVec *uar_sched_threads;

// The worklist of items the threads share.
UARWorkList *uar_global_worklist;

// The worklist of items that is private to its thread.
UARWorkList *uar_tlocal_worklist;

// The dispatcher is set by a thread if its local worklist is empty,
// in that case other threads will insert parts of their items into the
// global worklist.
unsigned int uar_item_dispatcher_index;
int i=0;
#pragma omp threadprivate(uar_tlocal_worklist)

std::map<std::map<int, CxtNodeIdVar>, NodeID > retPaths;

void UseAfterReturnChecker::analyze(llvm::Module& module) {
	omp_set_num_threads(num_threads);
	double analysisStart_t = omp_get_wtime();

	if(IMPORTBUGS)
		importBugs();

	initialize(module);

	// Only matters during the backward phase.
	ContextCond::setMaxCxtLen(10);

	for(i=0; i < stackSVFGNodes.size(); ++i) {
		auto stackSVFGNodesIter = stackSVFGNodes.begin();
		advance(stackSVFGNodesIter, i);
		double start;

		if(inImportedBugs(*stackSVFGNodesIter))
			continue;

		uar_global_worklist = new UARWorkList();
		uar_sched_threads = new BoolVec(num_threads);
		NodeID stackSvfgNodeId = (*stackSVFGNodesIter)->getId();
		setCurAnalysisCxt(*stackSVFGNodesIter);

		ContextCond cxt;
		UARDPItem item(stackSvfgNodeId, cxt, *stackSVFGNodesIter);
		item.addToVisited(0, stackSvfgNodeId);

		start = omp_get_wtime();

		forwardTraverse(item);

		cxtToFuncRetItems.clear();
		start = omp_get_wtime();

		// Backwards, starting by the out of scope nodes.
		findDanglingPointers();

		delete uar_global_worklist;
		delete uar_sched_threads;
	}

	if(EXPORTBUGS)
		exportBugs();
}

void UseAfterReturnChecker::initialize(llvm::Module& module) { 
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
	dra = new DirRetAnalysis(getGraph());
	max_t = UARTimeout;

	findSinks();
	initSrcs();
}

void UseAfterReturnChecker::initKernelContext() {
	kCxt = KernelContext::getKernelContext();
	assert(kCxt && "KernelContext not available");
}

void UseAfterReturnChecker::initSrcs() {
	stackSVFGNodes = svfgBuilder.getLocalNodes();
	globalSVFGNodes = svfgBuilder.getGlobalNodes();
	filterStackSVFGNodes(stackSVFGNodes);
}

void UseAfterReturnChecker::findDanglingPointers() {
	auto oosIterB = curAnalysisCxt->getOosNodes().begin();
	auto oosIterE = curAnalysisCxt->getOosNodes().end();
	auto dirOosIterB = curAnalysisCxt->getDirOosNodes().begin();
	auto dirOosIterE = curAnalysisCxt->getDirOosNodes().end();

	for(; oosIterB != oosIterE; ++oosIterB) {
		// There might be more dangline pointer. We have to backward the SVFG
		// starting from the outofScopeNode.
		NodeID oosNodeId = (*oosIterB)->getId();
		ContextCond cxt;
		UARDPItem item(oosNodeId, cxt, *oosIterB); 
		item.addToVisited(0, oosNodeId);
		backwardTraverse(item);
	}

	for(;dirOosIterB != dirOosIterE; ++dirOosIterB) {
		const SVFGNode *oosNode = *dirOosIterB;
		const SVFGNode *srcNode = curAnalysisCxt->getSource();
		const SVFGNode *danglingPtrNode = dra->findDstOfReturn(oosNode);	

		assert(danglingPtrNode != nullptr && "SimplePtrAssignmentAnalysis failed");

		curAnalysisCxt->appendOosForwardPath(oosNode->getId(), dra->getForwardPath()); 
		curAnalysisCxt->addOosForwardSlice(oosNode->getId(), dra->getForwardSlice()); 

		handleBug(danglingPtrNode, oosNode, dra->getBackwardPath());
	}
}

inline void UseAfterReturnChecker::forwardProcess(const UARDPItem& item) {
}

void UseAfterReturnChecker::forwardTraverse(UARDPItem& it) {
	uar_global_worklist->clear();
	uar_global_worklist->push_back(it);

	resetTimeout();

	#pragma omp parallel
	{
	uar_tlocal_worklist = new UARWorkList();

	while(true) {
		ContextCond cxt;
		UARDPItem item(0, cxt, nullptr);

		bool finish = false;
		bool threadWait = false;

		if(!popFromWorkList(item, true)) {
			finish = true;
			#pragma omp critical (uar_sched_threads)
			uar_sched_threads->at(omp_get_thread_num()) = true;
			uar_item_dispatcher_index = 1;

			#pragma omp critical (uar_sched_threads)
			for(auto iter = uar_sched_threads->begin(); iter != uar_sched_threads->end(); ++iter)
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

		if(!forwardChecks(item))
			continue;

		forwardProcess(item);

		// We found a SVFGNode that is used outside its scope.
		if(!item.hasValidScope()) {
			handleOosNode(curNode, item);
			continue;
		} 
		
		GNODE* v = getNode(item.getCurNodeID());
		auto EI = GTraits::child_begin(v);
		auto EE = GTraits::child_end(v);

		for (; EI != EE; ++EI) {
			forwardpropagate(item,*(EI.getCurrent()));
		}
	}

	delete uar_tlocal_worklist;
	}
}

bool UseAfterReturnChecker::forwardChecks(UARDPItem &item) {
	const SVFGNode* curNode = getNode(item.getCurNodeID());
	const SVFGNode* prevNode = getNode(item.getPrevNodeID());
	const StmtSVFGNode *curStmtNode = dyn_cast<StmtSVFGNode>(curNode);
	const StmtSVFGNode *prevStmtNode = dyn_cast<StmtSVFGNode>(prevNode);
	const llvm::Function *F = getSVFGFunction(curNode);

	if(curNode->getId() == prevNode->getId())
		return false;

	if(isa<StoreSVFGNode>(curNode)) {
		const SVFGNode *svfgSrcNode = getSVFGNodeDef(getGraph(), curNode);

		// The Ptr is set to NULL.
		if(svfgSrcNode->getId() == 0) {
			return false;
		}

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
			if(isa<LoadSVFGNode>(prevNode) || isa<GepSVFGNode>(prevNode) 
					|| isa<AddrSVFGNode>(prevNode))
				// Something is put into the current stack variable.
				if(prevPagDstNode->getId() != curPagSrcNode->getId()) {
					return false;
				}
		} else if(isa<LoadSVFGNode>(curNode) && 
				(isa<AddrSVFGNode>(prevNode) || isa<GepSVFGNode>(prevNode))) {
			return false;
		}
	}

	if(isa<CopySVFGNode>(prevNode) && 
			!getGraph()->getSVFGEdge(prevNode, curNode, SVFGEdge::SVFGEdgeK::DirRet)) {
		// This will only copy the value.
		return false;
	} 

	// Has to be at the end
	if(isa<LoadSVFGNode>(curNode))
		item.setDeref();

	return true;
}

void UseAfterReturnChecker::forwardpropagate(const UARDPItem& item, GEDGE* edge) {
	const SVFGNode* curNode = edge->getSrcNode();
	const SVFGNode* nextNode = edge->getDstNode();
	UARDPItem newItem(item);
	newItem.setCurNodeID(edge->getDstID());
	newItem.setPrevNodeID(item.getCurNodeID());

	if (edge->isCallVFGEdge()) {
		// We will handle the function with a new item.
		handleFunctionEntry(newItem, edge);
		return;
	} else if (edge->isRetVFGEdge()) {
		handleFunctionReturn(newItem, edge);
		return;
	}

	if(newItem.hasVisited(item.getCurCxt(), nextNode->getId()))
		return;
	
	newItem.addToVisited(item.getCurCxt(), nextNode->getId());

	pushIntoWorkList(newItem);
}

void UseAfterReturnChecker::handleFunctionEntry(UARDPItem &funcEntryItem, GEDGE *edge) {
	const SVFGNode* curNode = edge->getSrcNode();
	const SVFGNode* nextNode = edge->getDstNode();
	ContextCond cxt;
	NodeID funcEntryNodeId = funcEntryItem.getCurNodeID();
	// funcEntryNode is identical with nextNode in that case!
	UARDPItem funcItem(funcEntryNodeId, cxt, nextNode);
	CallSiteID csId = 0;

	if(const CallDirSVFGEdge* callEdge = dyn_cast<CallDirSVFGEdge>(edge)) {
		csId = callEdge->getCallSiteId();
	} else {
		csId = cast<CallIndSVFGEdge>(edge)->getCallSiteId();
		// Because this function entry is indirect there was a StoreSVFGNode
		// before, so a global node points to a local one and we have to
		// se the fistAsgnm flag.
		funcItem.firstAsgnmVisited();
	}

	if(isSink(nextNode))
		return;

	if(funcEntryItem.hasVisited(csId, nextNode->getId()))
		return;

	funcEntryItem.addToVisited(csId, nextNode->getId());

	// If we are about to forward into a function we already visited with the same context
	// use the stored information to skip the function by all paths that are already visited.
	if(isVisitedFuncEntryNode(funcEntryNodeId, csId)) {
		handleVisitedFunction(funcEntryItem, csId);

		// If the waiting list of function entries is empty, from this we can conclude
		// that there is no other item working at this function and the results are
		// completly.
		if(isFunctionCompleted(funcEntryNodeId))
			return;
	} 
	
	// But because there might be items which are handling nodes for the same function entry,
	// the item has also be added to the waitinglist of funcEntries.
	#pragma omp critical (WaitingFuncEntries)
	curAnalysisCxt->addWaitingFuncEntryItem(funcEntryNodeId, csId, funcEntryItem);

	// The called function is currently handled. The new item will be
	// handled if the items forwarding the function entry is finish.
	if(!curAnalysisCxt->inFuncEntryWorkSet(funcEntryNodeId)) {
		#pragma omp critical (FuncEntryWorkSet)
		{
		if(!curAnalysisCxt->inFuncEntryWorkSet(funcEntryNodeId)) {
			curAnalysisCxt->addFuncEntryToWorkSet(funcEntryNodeId);
			pushIntoWorkList(funcItem);
		}
		}	
	} 

	return;
}

void UseAfterReturnChecker::handleFunctionReturn(UARDPItem &retItem, GEDGE *edge) { 
	const SVFGNode* curNode = edge->getSrcNode();
	const SVFGNode* nextNode = edge->getDstNode();
	const llvm::BasicBlock *dstBB = nextNode->getBB();
	const llvm::Function *calleeFunc;		
	CallSiteID csId = 0;
	NodeID funcEntryNodeId = retItem.getRootID(); 

	assert(dstBB && "Dst node has no basic block");

	calleeFunc = dstBB->getParent();

	if(const RetDirSVFGEdge* callEdge = dyn_cast<RetDirSVFGEdge>(edge)) {
		csId = callEdge->getCallSiteId();
		retItem.setDirRet();
	} else {
		csId = cast<RetIndSVFGEdge>(edge)->getCallSiteId();
		retItem.setIndRet();
	}

	// Replace all unkown contexts(=0) with the given one.
	// Has to be before the current node is added to the visitedset,
	// because this node will be in another context.
	if(!retItem.resolveUnkownCxts(csId))
		return;

	// We return from a function we never entered. This is a potential bug.
	// Will be found/handled if forwarded.
	if(retItem.getRootID() == curAnalysisCxt->getSource()->getId()) {
		retItem.setInvalidScope();

		// If the out of scope node didn't use a direct return, this will be
		// the last node in the forward path. In case the last return was direct, 
		// the out of scope node will be forwarded again during the DirRetAnalysis 
		// and will be added to the visited nodes in a later step.
		if(!retItem.isDirRet())
			retItem.addToVisited(0, nextNode->getId());

		pushIntoWorkList(retItem);
		return;
	}

	// TODO is this necessary?
	if(retItem.hasVisited(0, nextNode->getId()))
		return;
	
	retItem.addToVisited(0, nextNode->getId());

	// We leave a function and save the next node as a result corresponding 
	// to the context. This should only be done if we actually entered a 
	// function.
	#pragma omp critical (cxtToFuncRetItems)
	addRetToFuncCxt(funcEntryNodeId, csId, retItem);

	// During the handling of the function that returns now it might be 
	// possible that another item reached the function entry node and was 
	// added to a waiting list. Now we have to handle these waiting items.
	handleWaitingFuncEntryNodes(retItem, csId);
}

void UseAfterReturnChecker::handleVisitedFunction(const UARDPItem &item, CallSiteID csId) {
	const ItemSet &funcRetSet = getFuncRetItems(item.getCurNodeID(), csId);
	llvm::CallSite CS = getGraph()->getCallSite(csId);
	llvm::Function *F = CS.getCalledFunction();

	for(auto iter = funcRetSet.begin(); iter != funcRetSet.end(); ++iter) {
		const UARDPItem &funcRetItem = *iter;

		UARDPItem newItem(item);
		bool res = newItem.merge(funcRetItem);

		// After the items were merged it might be that the path
		// of the item is not valid anymore (e.g. nodes visited again (test23))
		// so we have to verify that the new item can be forwarded.
		if(res) {
			pushIntoWorkList(newItem);
		}
	}
}

void UseAfterReturnChecker::handleWaitingFuncEntryNodes(const UARDPItem &funcRetItem, CallSiteID csId) {
	NodeID funcEntryNodeId = funcRetItem.getRootID(); 
	const ItemSet *waitingItems;

	if(!curAnalysisCxt->hasWaitingFuncEntry(funcEntryNodeId, csId))
		return;

	#pragma omp critical (WaitingFuncEntries)
	waitingItems = &curAnalysisCxt->getWaitingFuncEntryItems(funcEntryNodeId, csId);

	llvm::CallSite CS = getGraph()->getCallSite(csId);
	llvm::Function *F = CS.getCalledFunction();

	for(auto iter = waitingItems->begin(); iter != waitingItems->end(); ++iter) {
		UARDPItem newItem = *iter;

		bool res = newItem.merge(funcRetItem);

		// After the items were merged it might be that the path
		// of the item is not valid anymore (e.g. nodes visited again (test23))
		// so we have to verify that the new item can be forwarded.
		if(res){
			pushIntoWorkList(newItem);
		} 
	}
}

void UseAfterReturnChecker::pushIntoWorkList(const UARDPItem &item) {
	if(uar_item_dispatcher_index == 0) {
		uar_tlocal_worklist->push_back(item);
	} else {
		#pragma omp critical (global_worklist)
		uar_global_worklist->push_back(item);
	}
}

bool UseAfterReturnChecker::popFromWorkList(UARDPItem &item, bool fifo) {
	bool res = false;

	if(!uar_tlocal_worklist->empty()) {
		if(fifo) {
			item = uar_tlocal_worklist->front();
			uar_tlocal_worklist->pop_front();
		} else {
			item = uar_tlocal_worklist->back();
			uar_tlocal_worklist->pop_back();
		}
		res = true;
	} else {
		#pragma omp critical (global_worklist)
		{
		if(!uar_global_worklist->empty()) {
			if(fifo) {
				item = uar_global_worklist->front();
				uar_global_worklist->pop_front();
			} else {
				item = uar_global_worklist->back();
				uar_global_worklist->pop_back();
			}
			res = true;
			#pragma omp critical (uar_sched_threads)
			uar_sched_threads->at(omp_get_thread_num()) = false;
			uar_item_dispatcher_index = 0;
		} 
		}
	}

	return res;
}

inline void UseAfterReturnChecker::backwardProcess(const UARDPItem& item) {
	const SVFGNode *srcNode = curAnalysisCxt->getSource();
	const SVFGNode *curNode = getNode(item.getCurNodeID());

	// The current node might be a dangling pointer.
	if(isa<AddrSVFGNode>(curNode)) {
		if(globalSVFGNodes.find(curNode) != globalSVFGNodes.end()) {
			handleBug(curNode, item.getRoot(), item.getVisitedPath());
		} else if(stackSVFGNodes.find(curNode) != stackSVFGNodes.end() && !item.hasValidScope()) {
			// The dangling pointer cant be be our local node we started with.
			if(curNode->getId() != srcNode->getId()) 
				handleBug(curNode, item.getRoot(), item.getVisitedPath());
		}
	}
}

void UseAfterReturnChecker::backwardTraverse(UARDPItem& it) {
	resetTimeout();
	uar_global_worklist->clear();
	uar_global_worklist->push_back(it);

	#pragma omp parallel
	{
	uar_tlocal_worklist = new UARWorkList();

	while (true) {
		ContextCond cxt;
		UARDPItem item(0, cxt, nullptr);

		bool finish = false;
		bool threadWait = false;

		if(!popFromWorkList(item, false)) {
			finish = true;
			#pragma omp critical (uar_sched_threads)
			uar_sched_threads->at(omp_get_thread_num()) = true;
			uar_item_dispatcher_index = 1;

			#pragma omp critical (uar_sched_threads)
			for(auto iter = uar_sched_threads->begin(); iter != uar_sched_threads->end(); ++iter)
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

		if(!backwardChecks(item)) {
			continue;
		}

		backwardProcess(item);

		GNODE* v = getNode(item.getCurNodeID());
		// GNODE* v = getNode(item->getCurNodeID());
            	auto EI = InvGTraits::child_begin(v);
            	auto EE = InvGTraits::child_end(v);

            	for (; EI != EE; ++EI) {
                	backwardpropagate(item,*(EI.getCurrent()) );
            	}
		
	}

	delete uar_tlocal_worklist;
	}
}

bool UseAfterReturnChecker::backwardChecks(UARDPItem &item) {
	const SVFGNode *curNode = getNode(item.getCurNodeID());
	const SVFGNode *prevNode = getNode(item.getPrevNodeID());
	const StmtSVFGNode *curStmtNode = dyn_cast<StmtSVFGNode>(curNode);
	const StmtSVFGNode *prevStmtNode = dyn_cast<StmtSVFGNode>(prevNode);

	if(isa<NullPtrSVFGNode>(curNode)) 
		return false;

	if(curNode->getId() == prevNode->getId())
		return false;

	// All nodes that are backwarded before the first assignment has been mad, have
	// to be in the forwardslice of the out of scope node.
	if(!item.hasFirstAsgnmVisited()) {
		if(!curAnalysisCxt->inOosForwardSlice(item.getRootID(), 
					item.getCurCxt(), curNode->getId())) {
			return false;			
		} 
	}

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
	if(isa<StoreSVFGNode>(curNode)) {
		const SVFGNode *svfgSrcNode = getSVFGNodeDef(getGraph(), curNode);

		if(svfgSrcNode->getId() == 0)
			return false;

		if(!item.hasFirstAsgnmVisited() && !isParamStore(curNode)) {
			if(curAnalysisCxt->inOosForwardSlice(item.getRootID(), 
						item.getCurCxt(), curNode->getId()))
				item.firstAsgnmVisited();
			else		
				return false;
		} else if(!item.isDeref()) {
			return false;	
		}
	}

	// TODO move into process function
	if(isa<StoreSVFGNode>(prevNode))
		item.unsetDeref();

	// TODO move into process function
	// Has to be at the end
	if(isa<LoadSVFGNode>(curNode))
		item.setDeref();

	return true;
}

void UseAfterReturnChecker::backwardpropagate(const UARDPItem& item, GEDGE* edge) {
	const SVFGNode* curNode = edge->getDstNode();
	const SVFGNode* nextNode = edge->getSrcNode();
	CallSiteID csId;

	UARDPItem newItem(item);
	newItem.setCurNodeID(edge->getSrcID());
	newItem.setPrevNodeID(item.getCurNodeID());

	if (edge->isCallVFGEdge()) {
		if(const CallDirSVFGEdge* callEdge = dyn_cast<CallDirSVFGEdge>(edge))
			csId = callEdge->getCallSiteId();
		else
			csId = cast<CallIndSVFGEdge>(edge)->getCallSiteId();

		// If the current context is unknown, than all unknown contexts we have visited
		// belong to the one we are leaving with this function entry.
		if(newItem.getCurCxt() == 0)
			newItem.resolveUnkownCxts(csId);

		if(!newItem.matchContext(csId))
			return;

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
	}

	// This will be the next visited context.
	csId = newItem.getCurCxt();

	// If the current path contains the next node, we wont backward it. 
	if(newItem.hasVisited(csId, nextNode->getId()))
		return;

	// If we leave the valid scope, the next context will be 0.
	// As soon as we leave the valid scope dangling ptr are possible.
	if(csId == 0)
		newItem.setInvalidScope();

	newItem.addToVisited(csId, nextNode->getId());

	pushIntoWorkList(newItem);
}

//void UseAfterReturnChecker::createFuncToLocationMap(const CxtNodeIdSet &slice, StringToLoc &map) {
void UseAfterReturnChecker::createFuncToLocationMap(const CxtNodeIdList &path, StringToLoc &map) {
	//for(auto iter = slice.begin(); iter != slice.end(); ++iter) {
	for(auto iter = path.begin(); iter != path.end(); ++iter) {
		const SVFGNode *node = getNode(iter->second);
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

StringList UseAfterReturnChecker::findAPIPath() {
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


void UseAfterReturnChecker::handleBug(const SVFGNode *danglingPtrNode, 
			           const SVFGNode *oosNode,
				   CxtNodeIdList backwardPath) {

	double t = omp_get_wtime() - curAnalysisCxt->getStartTime();
	const SVFGNode *localVarNode = curAnalysisCxt->getSource();
	StringList syscallPath = findAPIPath();
	StringToLoc visitedFuncToLocMap;
	CxtNodeIdList forwardPath;

	Location danglingPtrLoc(getSVFGSourceFileName(danglingPtrNode), 
				getSVFGFuncName(danglingPtrNode), 
				getSVFGSourceLine(danglingPtrNode),
				getSVFGValueName(danglingPtrNode)); 

	Location localVarLoc(getSVFGSourceFileName(localVarNode), 
			     getSVFGFuncName(localVarNode), 
			     getSVFGSourceLine(localVarNode), 
			     getSVFGValueName(localVarNode));

	// Check if this bug was already found. (on another path)
	for(const auto &iter : getBugs()) {
		const UARBug *bug = dyn_cast<UARBug>(iter);
		if(bug->getDanglingPtrLocation() == danglingPtrLoc && 
	           bug->getLocalVarLocation() == localVarLoc)
			return;
	}

	backwardPath.reverse();

	// Since there are multiple forward-paths we have to find the one that has at least
	// the last store node identical to the backward path.
	if(!curAnalysisCxt->getOosForwardPath(oosNode->getId(), backwardPath, forwardPath))
		return;

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

void UseAfterReturnChecker::convertPAGNodesToSVFGNodes(const PAGNodeSet &pagNodes, SVFGNodeSet &svfgNodes) {
	const SVFGNode *svfgNode;

	for(auto pagNodesIter = pagNodes.begin(); pagNodesIter != pagNodes.end(); ++pagNodesIter) {
		svfgNode = getGraph()->getDefSVFGNode(*pagNodesIter);
		svfgNodes.insert(svfgNode);
	}
}

void UseAfterReturnChecker::filterStackSVFGNodes(SVFGNodeSet &svfgNodes) {
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
		
		if(isParamAddr(svfgNode) || !svfgNode->hasOutgoingEdge())
			svfgNodeIterB = svfgNodes.erase(svfgNodeIterB);
		else
			++svfgNodeIterB;
	}
}

void UseAfterReturnChecker::findSinks() {
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

void UseAfterReturnChecker::importSinks(StringSet &sinks) {
	std::string fileName = ImportSinks;
//	outs() << "Import Sinks from " << fileName << " ...\n";

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

bool UseAfterReturnChecker::isSink(const SVFGNode *node) const {
	const llvm::Function *F = getSVFGFunction(node);

	if(!F)
		return false;

	return sinks.find(F->getName()) != sinks.end();
}

void UseAfterReturnChecker::setCurAnalysisCxt(const SVFGNode* src) {
	if(curAnalysisCxt!=nullptr) {
		delete curAnalysisCxt;
		curAnalysisCxt = nullptr;
	}

	curAnalysisCxt = new UARAnalysisContext(src, getGraph());
}

bool UseAfterReturnChecker::inImportedBugs(const SVFGNode *node) {
	if(!(IMPORTBUGS))
		return false;

	std::string fileName = svfgAnalysisUtil::getSVFGSourceFileName(node);
	std::string funcName = svfgAnalysisUtil::getSVFGFuncName(node);
	u32_t line = svfgAnalysisUtil::getSVFGSourceLine(node);

	Location nodeLocation(fileName, funcName, line);

	for(auto iter = importedBugs.begin(); iter != importedBugs.end(); ++iter) {
		const UARBug *bug = dyn_cast<UARBug>(*iter);
		const Location &loc = bug->getLocalVarLocation();

		if(nodeLocation == loc)
			return true;
	}	

	return false;
}

void UseAfterReturnChecker::importBugs() {
	std::string fileName = ImportBugs;
	outs() << "Import Bugs from " << fileName << " ...\n";
	std::ifstream in(fileName.c_str(), std::ifstream::in);
	u32_t numBugs = 0;

	if(in) {
		// Check if file is empty.
		if(in.peek() == std::ifstream::traits_type::eof())
			return;

		in >> numBugs;

		for(int i=0; i < numBugs; ++i) {
			UARBug *bug = new UARBug();
			in >> *bug;

			importedBugs.insert(bug);

			in.ignore(numeric_limits<streamsize>::max(), '\n');
		}
			
		in.close();
	}

	outs() << "\t" << importedBugs.size() << " bugs imported\n";
}

void UseAfterReturnChecker::exportBugs() {
	//TODO
	assert(false && "TODO");
//	std::string fileName = ExportBugs;
//	outs() << "Export Bugs to " << fileName << " ...\n";
//	std::ofstream out(fileName.c_str(), std::ofstream::out);
//
//	bugs.insert(importedBugs.begin(), importedBugs.end());
//
//	out << bugs.size() << " ";
//
//	if(out) {
//		for(auto iter = bugs.begin(); iter != bugs.end(); ++iter) {
//			UARBug *bug = iter;
//			out << *bug << "\n";
//		}
//
//		out.close();
//	}
//
//	outs() << "\t" << bugs.size() << " bugs exported\n";
}
