#ifndef DOUBLE_LOCK_CHECKER_H_
#define DOUBLE_LOCK_CHECKER_H_

#include "KernelCheckerAPI.h"
#include "KernelModels/KernelContext.h"
#include "KernelModels/KernelSVFGBuilder.h"
#include "Util/AnalysisContext.h"
#include "Util/AnalysisUtil.h"
#include "Util/SVFGAnalysisUtil.h"
#include "Util/Bug.h"
#include "Util/KCFSolver.h"
#include "Util/KMinerStat.h"
#include "Checker/KernelChecker.h"
#include "Checker/SrcSnkAnalysis.h"
#include "SVF/WPA/Andersen.h"
#include "SVF/WPA/FlowSensitive.h"

/*!
 * Static Double Lock Detector
 */
class DoubleLockChecker : public SrcSnkAnalysis, public KernelChecker {
public:
	typedef std::set<DoubleLockBug> DoubleLockBugSet;
	typedef std::set<const KSrcSnkDPItem*> DPItemSet;	
	typedef std::set<const SVFGNode*> SVFGNodeSet;
	typedef FIFOWorkList<const SVFGNode*> VFWorkList;

	static char ID;


	DoubleLockChecker(char id = ID): KernelChecker("Double-Lock", ID) {
	}

	virtual ~DoubleLockChecker() {
	}

protected:
	/**
	 * Has to be implemented by the individual checkers.
	 */
	virtual void analyze(llvm::Module &module);

	/**
	 * Has to be implemented by the checkers.
	 */
	void evaluate(SrcSnkAnalysisContext *curAnalysisCxt);

	/**
	 * Return the number of variables analyzed.
	 */
	virtual unsigned int getNumAnalyzedVars() const { 
		return numAnalyzedVars;
	}

	/**
	 * Creates a bug object and adds it to the list of bugs.
	 */
	bool handleBug(const SVFGNode* lockObj, 
		    const KSrcSnkDPItem &lockItem1, 
		    const KSrcSnkDPItem &lockItem2) {
		DoubleLockBug *bug = new DoubleLockBug();
		double t = omp_get_wtime() - getCurAnalysisCxt()->getStartTime();

		std::string lockObjFileName = svfgAnalysisUtil::getSVFGSourceFileName(lockObj);
		std::string lockObjFuncName = svfgAnalysisUtil::getSVFGFuncName(lockObj); 
		uint32_t lockObjLine = svfgAnalysisUtil::getSVFGSourceLine(lockObj);
		std::string lockObjName = svfgAnalysisUtil::getSVFGValueName(lockObj); 
		bug->setSourceLocation(lockObjFileName, lockObjFuncName, lockObjLine, lockObjName);

		NodeID lockFunc1NodeID = lockItem1.getCond().getVFEdges()[0].first;
		const SVFGNode *lockFunc1 = getGraph()->getSVFGNode(lockFunc1NodeID);
		std::string lockFunc1FileName = svfgAnalysisUtil::getSVFGSourceFileName(lockFunc1);
		std::string lockFunc1Name = svfgAnalysisUtil::getSVFGFuncName(lockFunc1); 
		uint32_t lockFunc1Line = svfgAnalysisUtil::getSVFGSourceLine(lockFunc1);
		bug->setSink1Location(lockFunc1FileName, lockFunc1Name, lockFunc1Line);

		NodeID lockFunc2NodeID = lockItem2.getCond().getVFEdges()[0].first;
		const SVFGNode *lockFunc2 = getGraph()->getSVFGNode(lockFunc2NodeID);
		std::string lockFunc2FileName = svfgAnalysisUtil::getSVFGSourceFileName(lockFunc2);
		std::string lockFunc2Name = svfgAnalysisUtil::getSVFGFuncName(lockFunc2); 
		uint32_t lockFunc2Line = svfgAnalysisUtil::getSVFGSourceLine(lockFunc2);
		bug->setSink2Location(lockFunc2FileName, lockFunc2Name, lockFunc2Line); 

		StringList apiPath = getAPIPath();

		if (apiPath.empty())
			return false;

		bug->setAPIPath(apiPath);

		std::string lockFunc1PathStr = 
			svfgAnalysisUtil::getFuncPathStr(getGraph(),lockItem1.getCond().getVFEdges(), false); 
		bug->setSink1PathStr(lockFunc1PathStr);

		std::string lockFunc2PathStr = 
			svfgAnalysisUtil::getFuncPathStr(getGraph(),lockItem2.getCond().getVFEdges(), false); 
		bug->setSink2PathStr(lockFunc2PathStr);

		bug->setDuration(t);

		return addBug(bug);
	}
	
private:
	unsigned int numAnalyzedVars = 0;
};

#endif // DOUBLE_LOCK_CHECKER_H_
