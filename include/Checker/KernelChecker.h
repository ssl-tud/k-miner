#ifndef KERNEL_CHECKER_H
#define KERNEL_CHECKER_H

#include "Util/KMinerStat.h"
#include <omp.h>

class KernelChecker: public llvm::ModulePass {
public:
	KernelChecker(std::string checkerName, char id): 
		ModulePass(id), checkerName(checkerName) { }

	virtual ~KernelChecker() { 
		destroyBugs();
	}

	virtual bool runOnModule(llvm::Module& module) {
		llvm::outs() << "\n\n" << checkerName << ":\n";
		llvm::outs() << "====================================================\n";

		double analysisStart_t = omp_get_wtime();
		this->module = &module;

		analyze(module);

		analysisDurationTotal = omp_get_wtime() - analysisStart_t;

		report();

		// Checker Statistic.
		CheckerStat checkerStat(checkerName, analysisDurationTotal, 
					getNumAnalyzedVars(), bugs);
		KMinerStat::createKMinerStat()->addToCheckerStat(checkerStat);
		return false;
	}

	virtual const char* getPassName() const {
		return checkerName.c_str();	
	}

	virtual void getAnalysisUsage(llvm::AnalysisUsage& au) const {
		au.setPreservesAll();
	}

protected:
	/**
	 * Returnss the time the analysis took.
	 */
	double getAnalysisDurationTotal() const {
		return analysisDurationTotal;
	}

	/**
	 * Has to be implemented by the individual checkers.
	 */
	virtual void analyze(llvm::Module &module) { }

	/**
	 * Prints a report to the console.
	 */
	virtual void report() {
		std::stringstream rawReport; 

		rawReport << "____________________________________________________\n";
		rawReport << "Time (sec)" << std::setw(42-NUMDIGITS((int)analysisDurationTotal)) << " " 
			  << (int)analysisDurationTotal << "\n";
		rawReport << "____________________________________________________\n";
		rawReport << "Num analyzed variables" 
			  << std::setw(30-NUMDIGITS(getNumAnalyzedVars())) << " " 
			  << getNumAnalyzedVars() << "\n";
		rawReport << "____________________________________________________\n";
		rawReport << "Num bugs found" << std::setw(38-NUMDIGITS(bugs.size())) 
			  << " " << bugs.size() << "\n";
		rawReport << "____________________________________________________\n";
	//	rawReport << "Double-Free Reports:\n\n";

	//	int i=1;
	//	for(auto iter = bugs.begin(); iter != bugs.end(); ++iter, ++i) {
	//		const DoubleFreeBug &bug = *iter;
	//
	//		rawReport << i << ". ";
	//		rawReport << bug.toOnlinerString();
	//		rawReport << "\n";
	//	}
		llvm::outs() << rawReport.str();
	}

	/**
	 * Return the number of variables analyzed.
	 */
	virtual unsigned int getNumAnalyzedVars() const { }

	/**
	 * Adds a bug to the set of bugs.
	 */
	bool addBug(Bug *bug) {
		return bugs.insert(bug).second;
	}

	/**
	 * Return the set of bugs.
	 */
	const Bug::BugSet& getBugs() const {
		return bugs;
	}

	/**
	 * Deletes all allocated bugs.
	 */
	void destroyBugs() {
		for(auto &iter : bugs) {
			const Bug *bug = iter;

			if(bug)
				delete bug;
			bug = nullptr;
		}

		bugs.clear();
	}	

	/**
	 * Returns a const module.
	 */
	const llvm::Module& getModule() const {
		return *module;
	}

private:
	// Duration of the whole analysis.
	double analysisDurationTotal;

	// Name of the checker.
	std::string checkerName;

	// Contains all bugs found.
	Bug::BugSet bugs;

	llvm::Module *module;
};

#endif // KERNEL_CHECKER_H
