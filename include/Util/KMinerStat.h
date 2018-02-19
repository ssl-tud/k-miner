#ifndef KMINER_STAT_H
#define KMINER_STAT_H

#include "KernelModels/Systemcall.h"
#include "SVF/MSSA/SVFGStat.h"
#include "SVF/Util/BasicTypes.h"
#include "Util/Bug.h"
#include <string>
#include <set>
#include <ctime>

class ContextStat {
public:
	ContextStat() { }

	~ContextStat() { }

	void setCxtName(std::string name) {
		cxtName = name;
	}

	void setCxtType(std::string type) {
		cxtType = type;
	}

	std::string toString() const;
private:
	std::string cxtName;
	std::string cxtType;
};

class PartitionerStat {
public:
	PartitionerStat() { }

	~PartitionerStat() { }

	void setNumIR (uint32_t size) {
	       numIR = size;
	}

	void setNumIO (uint32_t size) {
		numIO = size;
	}

	void setNumIRFuncs(uint32_t size) {
	       numIRFuncs = size;
	}

	void setNumIOFuncs(uint32_t size) {
		numIOFuncs = size;
	}

	void setNumIRGVs(uint32_t size) {
		numIRGVs = size; 
	}

	void setNumIOGVs(uint32_t size) {
		numIOGVs = size;
	}

	void setNumIRNDGVs(uint32_t size) {
		numIRNDGVs = size; 
	}

	void setNumIONDGVs(uint32_t size) {
		numIONDGVs = size;
	}

	void setIRCGDepth(uint32_t size) {
		iRCGDepth = size;
	}

	void setIOCGDepth(uint32_t size) {
		iOCGDepth = size;
	}

	void setNumARFuncs(uint32_t size) {
	       numARFuncs = size;
	}

	void setNumAOFuncs(uint32_t size) {
		numAOFuncs = size;
	}

	void setNumARGVs(uint32_t size) {
		numARGVs = size; 
	}

	void setNumAOGVs(uint32_t size) {
		numAOGVs = size;
	}

	void setARCGDepth(uint32_t size) {
		aRCGDepth = size;
	}

	void setAOCGDepth(uint32_t size) {
		aOCGDepth = size;
	}

	void setNumKRFuncs(uint32_t size) {
	       numKRFuncs = size;
	}

	void setNumKOFuncs(uint32_t size) {
		numKOFuncs = size;
	}

	void setNumKRGVs(uint32_t size) {
		numKRGVs = size; 
	}

	void setNumKOGVs(uint32_t size) {
		numKOGVs = size;
	}

	void setFuncLimit(uint32_t size) {
		funcLimit = size;
	}

	void setInitcallFuncLimit(uint32_t size) {
		initcallFuncLimit = size;
	}

	std::string toString() const;

private:
	// Number of initcalls.
	uint32_t numIR = 0; 
	uint32_t numIO = 0; 

	// Number of initcall functions.
	uint32_t numIRFuncs = 0; 
	uint32_t numIOFuncs = 0;

	// Number of initcall global variables.
	uint32_t numIRGVs = 0; 
	uint32_t numIOGVs = 0;

	// Number of initcall non defined global variables.
	uint32_t numIRNDGVs = 0; 
	uint32_t numIONDGVs = 0;

	// Number of initcall call graph depths.
	uint32_t iRCGDepth = 0; 
	uint32_t iOCGDepth = 0;

	// Number of API functions.
	uint32_t numARFuncs = 0; 
	uint32_t numAOFuncs = 0;

	// Number of API global variables.
	uint32_t numARGVs = 0; 
	uint32_t numAOGVs = 0;

	// Number of API call graph depth.
	uint32_t aRCGDepth = 0; 
	uint32_t aOCGDepth = 0;

	// Number of kernel functions.
	uint32_t numKRFuncs = 0; 
	uint32_t numKOFuncs = 0;

	// Number of kernel global variables.
	uint32_t numKRGVs = 0; 
	uint32_t numKOGVs = 0;

	// Limits
	uint32_t funcLimit = 0;
	uint32_t initcallFuncLimit = 0;
};

/***
 * Collects some statistical information about the system.
 */
class SystemStat {
public:
	SystemStat() { }
	
	~SystemStat() { }

	void setKernelName(std::string name) {
		kernelName = name;	
	}

	void setKernelRelease(std::string name) {
		kernelRelease = name;	
	}

	void setKernelVersion(std::string name) {
		kernelVersion = name;
	}
	
	void setProcessor(std::string name) {
		processor = name;
	}

	void setNumCores(uint32_t size) {
		numCores = size;	
	}

	void setMemTotal(uint32_t size) {
		memTotal = size;
	}

	void updateUsedMem(uint32_t size) {
		if(size > maxUsedMem)
			maxUsedMem = size;
		usedMem = (usedMem+size)/2;
	}

	void setUsedMem(uint32_t size) {
		usedMem = size;
	}

	void setMaxUsedMem(uint32_t size) {
		maxUsedMem = size;
	}

	std::string toString() const;

public:
	std::string kernelName = "";	
	std::string kernelRelease = "";	
	std::string kernelVersion = "";
	std::string processor = "";
	uint32_t numCores = 0;	
	uint32_t memTotal = 0;
	uint32_t maxUsedMem = 0;
	uint32_t usedMem = 0;
};

/***
 * Collects some statistical information about the checker.
 */
class CheckerStat {
public:
	CheckerStat(std::string name, 
		    double totalDuration, 
		    unsigned int numAnalyzedVar, 
		    const Bug::BugSet &bugs): 
		    name(name), 
		    totalDuration(totalDuration), 
		    numAnalyzedVar(numAnalyzedVar), 
		    bugs(bugs) { }

	~CheckerStat() { }

	std::string getName() const {
		return name;
	}

	void setName(std::string name) {
		this->name = name;
	}

	double getTotalDuration() const {
		return totalDuration;
	}

	void setTotalDuration(double totalDuration) {
		this->totalDuration = totalDuration;
	}

	unsigned int getNumAnalyzedVar() const {
		return numAnalyzedVar;	
	}

	void setNumAnalyzedVar(unsigned int numAnalyzedVar) {
		this->numAnalyzedVar = numAnalyzedVar;
	}

	const Bug::BugSet& getBugs() const {
		return bugs;	
	}

	std::string toString() const;

private:
	std::string name;
	double totalDuration;	
	unsigned int numAnalyzedVar;
	const Bug::BugSet &bugs;
};

/***
 * Collects some statistical information about the analysis.
 */
class KMinerStat {
private:
	static KMinerStat *kminerStat;

	uint32_t getNumberOfDigits(uint32_t i) {
		return i > 0 ? (int) log10 ((double) i) + 1 : 1;
	}

	KMinerStat() { }

	~KMinerStat() { }

public:
	typedef std::set<std::string> StringSet;
	typedef std::list<std::string> StringList;
	typedef std::list<CheckerStat> CheckerStatList;

	static KMinerStat* createKMinerStat() {
		if(kminerStat == nullptr)
			kminerStat = new KMinerStat();
		return kminerStat;
	}

	static void releaseKMinerStat() {
		if(kminerStat != nullptr)
			delete kminerStat;
		kminerStat = nullptr;
	}

	/**
	 * Get the checker statistic as a string.
	 */
	std::string checkerInfoToString() const;

	/**
	 * Get Module, Systemcalls, NumThreads and Timeout as a string.
	 */
	std::string generalInfoToString() const;

	/**
	 * Get KernelVersion, NumCores, MemTotal, ... as a string.
	 */
	std::string systemInfoToString() const;

	/**
	 * Get the function names that are blacklisted.
	 */
	std::string blacklistToString() const;

	/**
	 * Get the sinks that are used.
	 */
	std::string sinksToString() const;

	/**
	 * Get the total time.
	 */	
	std::string totalTimeToString() const;

	/**
	 * Get the statistic as a string.
	 */
	std::string toString() const;

	void setSystemStat(const SystemStat &systemStat) {
		this->systemStat = systemStat;
	}

	void setContextStat(const ContextStat &contextStat) {
		this->contextStat = contextStat;
	}

	void setPartitionerStat(const PartitionerStat &partitionerStat) {
		this->partitionerStat = partitionerStat;
	}

	void addToCheckerStat(const CheckerStat &checkerStat) {
		checkerStats.push_back(checkerStat);	
	}

	void setNumThreads(uint32_t size) {
		numThreads = size;
	}
	
	void setBlackList(const StringSet &blacklist) {
		this->blacklist = blacklist;
	}

	void setSinks(const StringSet &sinks) {
		this->sinks= sinks;
	}

	void setModuleName(std::string str) {
		moduleName = str;
	}

	void resetSystemStat() {
		systemStat = SystemStat();
	}

	void resetContextStat() {
		contextStat = ContextStat();
	}

	void resetPartitionerStat() {
		partitionerStat = PartitionerStat();
	}

	void resetCheckerStat() {
		checkerStats.clear();
	}

	void reset() {
		blacklist.clear();
		sinks.clear();
		moduleName = ""; 
		numThreads = 0;
		total_t = 0.0;

		resetSystemStat();
		resetContextStat();
		resetPartitionerStat();
		resetCheckerStat();
	}

	void setTotalTime(double t) {
		total_t = t;
	}

private:
	std::string moduleName = ""; 
	uint32_t numThreads = 0;
	StringSet blacklist;
	StringSet sinks;
	double total_t = 0.0;

	// System info
	SystemStat systemStat;	

	// Context info 
	ContextStat contextStat;

	// Partitioner info
	PartitionerStat partitionerStat;

	// Checker info
	CheckerStatList checkerStats;
};

#endif // KMINER_STAT_H
