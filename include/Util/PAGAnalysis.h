#ifndef PAG_ANALYSIS_H_
#define PAG_ANALYSIS_H_

#include "Util/KCFSolver.h"
#include "Util/SVFGAnalysisUtil.h"

typedef std::set<const PAGNode*> PAGNodeSet;

/***
 * Does PAG related analyses.
 */
class PAGAnalysis : public KCFSolver<PAG*,DPItem> {
private:
	PAGAnalysis(PAG *pag) { 
		setGraph(pag);	
		splitNodesByMemObj();
	}

	virtual ~PAGAnalysis() { }

	/**
	 * Finds local and global nodes.
	 */
	void splitNodesByMemObj();
public:
	static PAGAnalysis* createPAGAnalysis() {
		PAG *pag = PAG::getPAG();

		if(pagAnalysis == nullptr)
			pagAnalysis = new PAGAnalysis(pag);

		return pagAnalysis;
	}

	static PAGAnalysis* createPAGAnalysis(PAG *pag) {
		if(pagAnalysis == nullptr)
			pagAnalysis = new PAGAnalysis(pag);
		
		return pagAnalysis;
	}

	static void releasePAGAnalysis() {
		if(pagAnalysis)
			delete pagAnalysis;
		pagAnalysis = nullptr;
	}

	/**
	 * Checks if the PAG has a function call with the given
	 * function name.
	 */
	bool hasCallSite(std::string name) const;

	/**
	 * Get the function of the given PAGEdge.
	 */
	const llvm::Function* getFunctionOfEdge(const PAGEdge &edge) const;

	/**
	 * Get all edges that call a function with the given name.
	 */
	PAG::PAGEdgeSet findEdgesOfFunction(std::string name) const;

	/**
	 * Get the value of the given node.
	 */
	static std::string getValueName(const PAGNode *node);

	/**
	 * Get the global nodes found during splitNodesByMemObj.
	 */
	const PAGNodeSet& getGlobalObjPNodes() const;

	/**
	 * Get the global nodes found during splitNodesByMemObj.
	 */
	const PAGNodeSet& getLocalObjPNodes() const;

private:
	static PAGAnalysis *pagAnalysis;	
	PAGNodeSet localObjPNodes, globalObjPNodes;
};

#endif // PAG_ANALYSIS_H_
