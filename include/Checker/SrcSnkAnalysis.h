#ifndef SRCSNK_ANALYSIS_CHECKER_H_
#define SRCSNK_ANALYSIS_CHECKER_H_

#include "Util/KCFSolver.h"
#include "KernelCheckerAPI.h"
#include "Util/AnalysisContext.h"
#include "Util/AnalysisUtil.h"
#include "Util/SVFGAnalysisUtil.h"
#include "Util/KMinerStat.h"
#include "Util/Bug.h"
#include "KernelModels/KernelContext.h"
#include "KernelModels/KernelSVFGBuilder.h"
#include "SVF/MemoryModel/ConsG.h"
#include "SVF/WPA/Andersen.h"
#include "SVF/WPA/FlowSensitive.h"

typedef KCFSolver<SVFG*, KSrcSnkDPItem> CFLSrcSnkSolver;

/*!
 * Static Double Free Detector
 */
class SrcSnkAnalysis : public KCFSolver<SVFG*, KSrcSnkDPItem> {
public:
	typedef std::set<KSrcSnkDPItem> ItemSet;	
	typedef std::map<const SVFGNode*, ItemSet> SVFGNodeToDPItemsMap;
	typedef std::set<const SVFGNode*> SVFGNodeSet;
	typedef std::map<const SVFGNode*, SVFGNodeSet> SVFGNodeToSet;
	typedef std::pair<NodeID, NodeID> NodeIDPair;
	typedef std::vector<NodeIDPair> NodeIDPairVec;
	typedef std::set<NodeIDPair> NodeIDPairSet;

	typedef FIFOWorkList<DPItem> WorkList;

	SrcSnkAnalysis(): curAnalysisCxt(nullptr), ptaCallGraph(nullptr), svfg(nullptr) {
	}

	virtual ~SrcSnkAnalysis() {
		if (pathCondAllocator)
			delete pathCondAllocator;
		pathCondAllocator = NULL;

		if (svfgBuilder) {
			delete svfg;
			delete svfgBuilder;
		}

		svfg = NULL;
		svfgBuilder = NULL;
	}

	void initialize(llvm::Module& module) {
		kernelCxt = KernelContext::getKernelContext();
		AndersenWaveDiff* ander = AndersenWaveDiff::createAndersenWaveDiff(module);
		ptaCallGraph = ander->getPTACallGraph();

		if (svfgBuilder == NULL) {
			svfg = new SVFGOPT(ptaCallGraph);
			svfgBuilder = new KernelSVFGBuilder();
			svfgBuilder->build(svfg,ander);
		} else {
			svfg = svfgBuilder->getSVFG();
		}

		setGraph(svfgBuilder->getSVFG());
		consCG = ander->getConstraintGraph();
		pag = getGraph()->getPAG();

		// allocate control-flow graph branch conditions
		if (pathCondAllocator == NULL) {
			pathCondAllocator = new PathCondAllocator();
			pathCondAllocator->allocate(module);
		}

		ContextCond::setMaxCxtLen(3);
		VFPathCond::setMaxPathLen(25);
	}

	void analyze(const SVFGNodeSet &sources, const SVFGNodeSet &sinks);

	/**
	 * Checks if the node is represents a source.
	 */
	bool isSource(const SVFGNode *node) const {
		return sources.find(node) != sources.end();
	}

	/**
	 * Checks if the node is represents a sink.
	 */
	bool isSink(const SVFGNode *node) const {
		return sinks.find(node) != sinks.end();
	}

protected:

	PAG* getPAG() const {
		return PAG::getPAG();
	}

	NodeID getPAGNodeID(const SVFGNode *node) {
		if(const ActualParmSVFGNode *parmNode = llvm::dyn_cast<ActualParmSVFGNode>(node))
			return parmNode->getParam()->getId();
		else if(const StmtSVFGNode *stmtNode = llvm::dyn_cast<StmtSVFGNode>(node))
			return stmtNode->getPAGDstNodeID();

		return 0;

	}

	const SVFGNodeSet& getAllocNodes() const {
		return svfgBuilder->getAllocNodes();
	}

	const SVFGNodeSet& getDeallocNodes() const {
		return svfgBuilder->getDeallocNodes();
	}

	const SVFGNodeSet& getGlobalNodes() const {
		return svfgBuilder->getGlobalNodes();
	}

	const SVFGNodeSet& getLocalNodes() const {
		return svfgBuilder->getLocalNodes();
	}

	const SVFGNodeSet& getUseNodes() const {
		return svfgBuilder->getUseNodes();
	}

	const SVFGNodeSet& getLockObjNodes() const {
		return svfgBuilder->getLockObjNodes();
	}

	const SVFGNodeSet& getLockNodes() const {
		return svfgBuilder->getLockNodes();
	}

	const SVFGNodeSet& getUnlockNodes() const {
		return svfgBuilder->getUnlockNodes();
	}

	const SVFGNodeSet& getNullStoreNodes() const {
		return svfgBuilder->getNullStoreNodes();
	}

	bool isAllocNode(const SVFGNode *node) const {
		return svfgBuilder->isAllocNode(node);
	}

	bool isDeallocNode(const SVFGNode *node) const {
		return svfgBuilder->isDeallocNode(node);
	}

	bool isGlobalNode(const SVFGNode *node) const {
		return svfgBuilder->isGlobalNode(node);
	}

	bool isLocalNode(const SVFGNode *node) const {
		return svfgBuilder->isLocalNode(node);
	}

	bool isUseNode(const SVFGNode *node) const {
		return svfgBuilder->isUseNode(node);
	}

	bool isLockObjNode(const SVFGNode *node) const {
		return svfgBuilder->isLockObjNode(node);
	}

	bool isLockNode(const SVFGNode *node) const {
		return svfgBuilder->isLockNode(node);
	}

	bool isUnlockNode(const SVFGNode *node) const {
		return svfgBuilder->isUnlockNode(node);
	}

	bool isNullStoreNode(const SVFGNode *node) const {
		return svfgBuilder->isNullStoreNode(node);
	}

	bool isInCurForwardSlice(const SVFGNode* node) {
		return curAnalysisCxt->inForwardSlice(node);
	}

	bool isInCurBackwardSlice(const SVFGNode* node) {
		return curAnalysisCxt->inBackwardSlice(node);
	}

	void addSinkPath(const KSrcSnkDPItem &item) {
		if(verifySrcSnk(item)) {
			curAnalysisCxt->addSinkPath(item);
		}
	}

	void definePathOrders() {
		curAnalysisCxt->sortPaths();
	}

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

	/**
	 * Check if the max number of paths has been reached.
	 */
	bool reachedMaxNumPath() const {
		return curAnalysisCxt->getSinkItems().size() > maxNumPaths;
	}

	/**
	 * Returns the current analysis context.
	 */
	SrcSnkAnalysisContext* getCurAnalysisCxt() const {
		return curAnalysisCxt;
	}

	void setCurSlice(const SVFGNode* src);

	virtual void forwardProcess(const KSrcSnkDPItem& item);

	virtual void backwardProcess(const KSrcSnkDPItem& item);

	virtual void forwardpropagate(const KSrcSnkDPItem& item, SVFGEdge* edge);

	virtual void backwardpropagate(const KSrcSnkDPItem& item, SVFGEdge* edge);

	bool visited(const SVFGNode* node, const KSrcSnkDPItem& item) {
		SVFGNodeToDPItemsMap::iterator it = nodeToDPItemsMap.find(node);
		if(it!=nodeToDPItemsMap.end())
			return it->second.find(item)!=it->second.end();
		else
			return false;
	}

	void addVisited(const SVFGNode* node, const KSrcSnkDPItem& item) {
		nodeToDPItemsMap[node].insert(item);
	}

	void clearVisitedMap() {
		nodeToDPItemsMap.clear();
	}

	/**
	 * Returnss the time the analysis took.
	 */
	double getAnalysisDurationTotal() const {
		return analysisDurationTotal;
	}

	/**
	 * Returns the path from an API function to the current source node.
	 */
	StringList getAPIPath();

	/**
	 * Has to be implemented by the checkers.
	 */
	virtual void evaluate(SrcSnkAnalysisContext *curAnalysisCxt){ }

	/**
	 * Checks if the sink that was found actualy does free the current memory object.
	 * This is not always true, since the SVFG is not field-sensitive and might direct
	 * source nodes in to sinks, which actualy are not freed at all.
	 */
	bool verifySrcSnk(const KSrcSnkDPItem &srcSnkItem);

private:
	SVFGNodeSet sources;

	SVFGNodeSet sinks;

	SrcSnkAnalysisContext *curAnalysisCxt;
	PTACallGraph *ptaCallGraph;
	SVFG *svfg;
	KernelContext *kernelCxt;

	// Nodes that were forward visited.
	SVFGNodeToDPItemsMap nodeToDPItemsMap;

	ConstraintGraph *consCG;
	PAG* pag;

	// Contains the sinks mapped to their source nodes.
	SVFGNodeToSet sinkToSrcNodes;

	// Duration of the whole analysis.
	double analysisDurationTotal;

	// Time limit.
	double max_t = 5; 

	// Start of timer.
	double start_t = 0;

	unsigned int maxNumPaths = 100;

	static PathCondAllocator *pathCondAllocator;
	static KernelSVFGBuilder *svfgBuilder;
};

#endif // SRCSNK_ANALYSIS_CHECKER_H_
