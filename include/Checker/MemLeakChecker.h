#ifndef MEM_LEAK_CHECKER_H_
#define MEM_LEAK_CHECKER_H_

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
 * Static Memory-Leak Checker 
 */
class MemLeakChecker : public SrcSnkAnalysis, public KernelChecker {
public:
	typedef std::pair<NodeID, NodeID> NodeIDPair;
	typedef std::vector<NodeIDPair> NodeIDPairVec;
	typedef std::set<NodeIDPair> NodeIDPairSet;
	typedef std::map<NodeIDPair, KSrcSnkDPItem> ItemMap;


	static char ID;

	MemLeakChecker(char id = ID): SrcSnkAnalysis(), KernelChecker("Memory-Leak", ID) {
	}

	virtual ~MemLeakChecker() { }

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

	/**
	 * Creates a bug object and adds it to the list of bugs.
	 */
	bool handleBug(const SVFGNode* leakPtr) {
		NeverFreeBug *bug = new NeverFreeBug();
		double t = omp_get_wtime() - getCurAnalysisCxt()->getStartTime();

		std::string leakPtrFileName = svfgAnalysisUtil::getSVFGSourceFileName(leakPtr);
		std::string leakPtrFuncName = svfgAnalysisUtil::getSVFGFuncName(leakPtr); 
		uint32_t leakPtrLine = svfgAnalysisUtil::getSVFGSourceLine(leakPtr);
		std::string leakPtrName = svfgAnalysisUtil::getSVFGValueName(leakPtr); 
		bug->setSourceLocation(leakPtrFileName, leakPtrFuncName, leakPtrLine, leakPtrName);

		StringList apiPath = getAPIPath();

		if (apiPath.empty())
			return false;

		bug->setAPIPath(apiPath);

		bug->setDuration(t);
		return addBug(bug);
	}
private:
	unsigned int numAnalyzedVars = 0;
};

#endif // MEM_LEAK_CHECKER_H_
