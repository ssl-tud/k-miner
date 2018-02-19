#include "Checker/SrcSnkAnalysis.h"
#include "Util/AnalysisUtil.h"
#include <llvm/Support/CommandLine.h>
#include <omp.h>

using namespace llvm;
using namespace analysisUtil;
using namespace svfgAnalysisUtil;

static cl::opt<bool> PATH_SENSITIVE("path-sens", cl::init(false),
		cl::desc("The analysis will be performed path-sensitive."));

PathCondAllocator *SrcSnkAnalysis::pathCondAllocator = NULL;
KernelSVFGBuilder *SrcSnkAnalysis::svfgBuilder = NULL;

bool KSrcSnkDPItem::pathSensitive = false;

void SrcSnkAnalysis::analyze(const SVFGNodeSet &sources, const SVFGNodeSet &sinks) {
	this->sources = sources;
	this->sinks = sinks;

	for(const auto &iter : sources) {
		double tmp_t = omp_get_wtime();
		setCurSlice(iter);

		VFPathCond cond;
		VFPathVar pathVar(cond, iter->getId());
		KSrcSnkDPItem::setCxtSensitive();
		KSrcSnkDPItem item(pathVar, iter);
		forwardTraverse(item);
		clearVisitedMap();

		if(PATH_SENSITIVE)
			KSrcSnkDPItem::setPathSensitive();

		for (const auto &iter2 : curAnalysisCxt->getVisitedSinks()) {
			VFPathCond cond;
			VFPathVar pathVar(cond, iter2->getId());
			KSrcSnkDPItem item(pathVar, iter2);
			resetTimeout();
			backwardTraverse(item);
		}

		evaluate(curAnalysisCxt);
	}
}

void SrcSnkAnalysis::forwardProcess(const KSrcSnkDPItem& item) {
	const SVFGNode* node = getNode(item.getCurNodeID());
	const EdgeVec &tmp = item.getCond().getVFEdges();
	curAnalysisCxt->addToForwardSlice(node);
}

void SrcSnkAnalysis::forwardpropagate(const KSrcSnkDPItem& item, SVFGEdge* edge) {
	const SVFGNode* srcNode = edge->getSrcNode();
	const SVFGNode* dstNode = edge->getDstNode();
	std::string funcName = getSVFGFuncName(dstNode); //TODO RM

	VFPathCond cond = item.getCond();
	VFPathVar pathVar(cond, dstNode->getId());
	KSrcSnkDPItem newItem(pathVar, item.getLoc());

	if(isSink(dstNode)) {
		curAnalysisCxt->addVisitedSink(dstNode);
		return;
	}

	if (edge->isCallVFGEdge()) {
		CallSiteID csId = 0;
		if(const CallDirSVFGEdge* callEdge = dyn_cast<CallDirSVFGEdge>(edge))
			csId = callEdge->getCallSiteId();
		else
			csId = cast<CallIndSVFGEdge>(edge)->getCallSiteId();

		if(newItem.pushContext(csId) == false)
			return;
	} else if (edge->isRetVFGEdge()) {
		CallSiteID csId = 0;
		if(const RetDirSVFGEdge* callEdge = dyn_cast<RetDirSVFGEdge>(edge))
			csId = callEdge->getCallSiteId();
		else
			csId = cast<RetIndSVFGEdge>(edge)->getCallSiteId();

		if (newItem.matchContext(csId) == false)
			return;
	}

	if(visited(dstNode,newItem))
		return;
		
	addVisited(dstNode, newItem);

	pushIntoWorklist(newItem);
}

void SrcSnkAnalysis::backwardProcess(const KSrcSnkDPItem& item) {
	const SVFGNode* node = getNode(item.getCurNodeID());
	const EdgeVec &tmp = item.getCond().getVFEdges();

	if(getCurAnalysisCxt()->getSource()->getId() == node->getId()) {
		addSinkPath(item);
	}

	curAnalysisCxt->addToBackwardSlice(node);
}

void SrcSnkAnalysis::backwardpropagate(const KSrcSnkDPItem& item, SVFGEdge* edge) {
	const SVFGNode* srcNode = edge->getSrcNode();
	const SVFGNode* dstNode = edge->getDstNode();

	if(!isInCurForwardSlice(srcNode) || srcNode->getId() == dstNode->getId()) {
		return;
	}

	VFPathCond cond = item.getCond();
	VFPathVar pathVar(cond, srcNode->getId());
	KSrcSnkDPItem newItem(pathVar, item.getLoc());

	if(const GepSVFGNode *gepNode = dyn_cast<GepSVFGNode>(srcNode)) {
		if(const NormalGepPE *edge= dyn_cast<NormalGepPE>(gepNode->getPAGEdge())) {
			newItem.pushFieldContext(edge->getOffset());
		}
	} else {
	       	newItem.setFieldCxt(item.getFieldCxt());
	}

	if (!newItem.addVFPath(pathCondAllocator, 
		  PathCondAllocator::trueCond(),
		  edge->getDstID(), 
		  edge->getSrcID()))  {
		return;
	}

	if(visited(srcNode, newItem)) {
		return;
	}

	if(reachedMaxNumPath()) {
		clearWorklist();
		return;
	}

	if(timeout()) {
		return;
	}

	addVisited(srcNode, newItem);

	pushIntoWorklist(newItem);
}

bool SrcSnkAnalysis::verifySrcSnk(const KSrcSnkDPItem &srcSnkItem) {
	const SVFGNode *srcNode = getCurAnalysisCxt()->getSource();
	const SVFGNode *snkNode = srcSnkItem.getLoc();

	SVFGNodeSet &srcSet = sinkToSrcNodes[snkNode];
	// PAG and Constraint Nodes have the same ID.
	NodeID constNodeId = getPAGNodeID(snkNode);
	DPItem item(constNodeId);	
	WorkList worklist;
	std::set<DPItem> visitedSet;	

	// The sink has been already analyzed if srSet.size > 0.
	// We skip nullStoreNodes as they are not correctly mapped to gep nodes.
	if(srcSet.size() > 0 && srcSet.find(srcNode) != srcSet.end() || isNullStoreNode(snkNode))
		return true;

	worklist.push(item);

	// Backward through ConstraintGraph
	while(!worklist.empty()) {
            	DPItem item = worklist.pop();
		ConstraintNode* consNode = consCG->getConstraintNode(item.getCurNodeID());
		auto EI = consNode->directInEdgeBegin();
		auto EE = consNode->directInEdgeEnd();

		// A source has been found
		if(EI == EE) {
			// Convert the ConstraintNode back into a SVFGNode.
			const PAGNode *pagNode =  pag->getPAGNode(consNode->getId());

			if(getGraph()->hasDef(pagNode)) {
				const SVFGNode *svfgNode = getGraph()->getDefSVFGNode(pagNode);
				srcSet.insert(svfgNode);
			}
		}

		for (; EI != EE; ++EI) {
			const ConstraintEdge *edge = *EI;
			const ConstraintNode* nextNode = edge->getSrcNode();
			DPItem newItem(item);
			newItem.setCurNodeID(edge->getSrcID());

			if(visitedSet.find(newItem) != visitedSet.end())
				continue;

			visitedSet.insert(newItem);
			worklist.push(newItem);
		}
	}

	return srcSet.find(srcNode) != srcSet.end();
}	

StringList SrcSnkAnalysis::getAPIPath() {
	StringList apiPath = curAnalysisCxt->getAPIPath();
	std::string funcName = getSVFGFuncName(curAnalysisCxt->getSource());
	StringSet filter = kernelCxt->getAPIFunctions();

	if(!apiPath.empty())
		return apiPath;

	LocalCallGraphAnalysis LCGA(ptaCallGraph);
	// Dont backward further than the API.
	LCGA.setBackwardFilter(filter);
	LCGA.analyze(funcName, false, false, std::numeric_limits<uint32_t>::max());
	const StrListSet &paths = LCGA.getBackwardFuncPaths();

	for(const auto &iter : paths) {
		const StringList &path = iter;
		std::string srcFunc = path.back();

		if(kernelCxt->isAPIFunction(srcFunc)) {
			apiPath = path;
			break;
		}
	}

	//TODO the reason could be that we don't use FSPtr-Analysis.
	//assert(!apiPath.empty() && "No path to the API found");

	curAnalysisCxt->setAPIPath(apiPath);

	return apiPath;
}

void SrcSnkAnalysis::setCurSlice(const SVFGNode* src) {
	if(curAnalysisCxt!=nullptr) {
		delete curAnalysisCxt;
		curAnalysisCxt = nullptr;
		// TODO clearVisitedMap();
	}

	curAnalysisCxt = new SrcSnkAnalysisContext(src, pathCondAllocator, getGraph());
}

