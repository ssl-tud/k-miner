#ifndef USE_FREE_CHECKER_H_
#define USE_FREE_CHECKER_H_

#include "Util/KCFSolver.h"
#include "KernelCheckerAPI.h"
#include "Util/AnalysisUtil.h"
#include "Util/SVFGAnalysisUtil.h"
#include "Util/Bug.h"
#include "Checker/SrcSnkAnalysis.h"
#include "Checker/KernelChecker.h"
#include "KernelModels/KernelSVFGBuilder.h"
#include "SVF/WPA/Andersen.h"
#include "SVF/WPA/FlowSensitive.h"

/*!
 * Static Double Free Detector
 */
class UseAfterFreeChecker : public SrcSnkAnalysis, 
			    public KernelChecker {
public:
	typedef std::pair<NodeID, NodeID> NodeIDPair;
	typedef std::vector<NodeIDPair> NodeIDPairVec;
	typedef std::set<NodeIDPair> NodeIDPairSet;

	static char ID;

	UseAfterFreeChecker(char id = ID): SrcSnkAnalysis(), KernelChecker("Use-After-Free", ID) { }

	virtual ~UseAfterFreeChecker() { }

protected:
	/**
	 * Has to be implemented by the individual checkers.
	 */
	virtual void analyze(llvm::Module &module);

	/**
	 * Has to be implemented by the checkers.
	 */
	virtual void evaluate(SrcSnkAnalysisContext *curAnalysisCxt);

	/**
	 * Return the number of variables analyzed.
	 */
	virtual unsigned int getNumAnalyzedVars() const { 
		return numAnalyzedVars;
	}

	bool handleBug(const SVFGNode* danglingPtr, 
		    const KSrcSnkDPItem &freeItem, 
		    const KSrcSnkDPItem &useItem) {
		UseAfterFreeBug *bug = new UseAfterFreeBug();
		double t = omp_get_wtime() - getCurAnalysisCxt()->getStartTime();

		std::string danglingPtrFileName = svfgAnalysisUtil::getSVFGSourceFileName(danglingPtr);
		std::string danglingPtrFuncName = svfgAnalysisUtil::getSVFGFuncName(danglingPtr); 
		uint32_t danglingPtrLine = svfgAnalysisUtil::getSVFGSourceLine(danglingPtr);
		std::string danglingPtrName = svfgAnalysisUtil::getSVFGValueName(danglingPtr); 
		bug->setSourceLocation(danglingPtrFileName, danglingPtrFuncName, danglingPtrLine,danglingPtrName);

		NodeID sink1NodeID = freeItem.getCond().getVFEdges()[0].first;
		const SVFGNode *sink1 = getGraph()->getSVFGNode(sink1NodeID);
		std::string sink1FileName = svfgAnalysisUtil::getSVFGSourceFileName(sink1);
		std::string sink1Name = svfgAnalysisUtil::getSVFGFuncName(sink1); 
		uint32_t sink1Line = svfgAnalysisUtil::getSVFGSourceLine(sink1);
		bug->setSink1Location(sink1FileName, sink1Name, sink1Line);

		NodeID sink2NodeID = useItem.getCond().getVFEdges()[0].first;
		const SVFGNode *sink2 = getGraph()->getSVFGNode(sink2NodeID);
		std::string sink2FileName = svfgAnalysisUtil::getSVFGSourceFileName(sink2);
		std::string sink2Name = svfgAnalysisUtil::getSVFGFuncName(sink2); 
		uint32_t sink2Line = svfgAnalysisUtil::getSVFGSourceLine(sink2);
		bug->setSink2Location(sink2FileName, sink2Name, sink2Line); 

		StringList apiPath = getAPIPath();

		if (apiPath.empty())
			return false;

		bug->setAPIPath(apiPath);

		std::string sink1PathStr = 
			svfgAnalysisUtil::getFuncPathStr(getGraph(),freeItem.getCond().getVFEdges(), false); 
		bug->setSink1PathStr(sink1PathStr);

		std::string sink2PathStr = 
			svfgAnalysisUtil::getFuncPathStr(getGraph(),useItem.getCond().getVFEdges(), false); 
		bug->setSink2PathStr(sink2PathStr);

		bug->setDuration(t);

		return addBug(bug);
	}
private:
	unsigned int numAnalyzedVars = 0;
};

#endif // USE_FREE_CHECKER_H_
