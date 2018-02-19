#ifndef REPORT_PASS_H
#define REPORT_PASS_H

#include "Util/KMinerStat.h"
#include <omp.h>

#define REPORT_BANNER "######################################################\n" \
		      "////////////////////// REPORT ////////////////////////\n" \
		      "######################################################\n"

/**
 * Creates a report of all logs.
 */
class ReportPass : public llvm::ModulePass {
public:
	static char ID;
	double start_total_t;

	ReportPass(char id = ID): ModulePass(ID) { 
       		start_total_t = omp_get_wtime();
		startMonitoringSystem();	
		collectSystemInfo();
	}

	virtual ~ReportPass() { 
		stopMonitoringSystem();
	}

	virtual bool runOnModule(llvm::Module& module) {
		llvm::outs() << "\n\n" << getPassName() << ":\n";
		llvm::outs() << "====================================================\n";
		stat = KMinerStat::createKMinerStat();
		stat->setTotalTime((omp_get_wtime()-start_total_t));

		// Write the statistics to a file.
		makeReport();

		// Reset the statistics.
		stat->resetContextStat();
		stat->resetPartitionerStat();
		stat->resetCheckerStat();

		return false;
	}

	virtual const char* getPassName() const {
		return "ReportPass";
	}

	virtual void getAnalysisUsage(llvm::AnalysisUsage& au) const {
		au.setPreservesAll();
	}

private:
	/**
	 * Scans the exuting system in collect some system specific information
	 * and store them in the statistic class.
	 */
	void collectSystemInfo();

	/**
	 * Use a new thread to monitor the currently used system resources.
	 */
	void startMonitoringSystem();

	/**
	 * Stops the monitoring of the system by joining the corresponding thread.
	 */
	void stopMonitoringSystem();

	/**
	 * Writes all logged information to a log file.
	 */
	void makeReport();

	/**
	 * Write message to the report file.
	 */
	void writeToReport(std::string msg);

	// Contains all log information found.
	KMinerStat *stat;
};


#endif // REPORT_PASS_H
