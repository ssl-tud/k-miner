#include "Util/KMinerStat.h"
#include "Util/DebugUtil.h"
#include "Util/KernelAnalysisUtil.h"
#include "llvm/Support/raw_ostream.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace llvm;
using namespace debugUtil;
using namespace analysisUtil;

KMinerStat* KMinerStat::kminerStat = nullptr;

std::string CheckerStat::toString() const {
	std::stringstream rawStat; 

	rawStat << name << " Analysis Statistic:\n";
	rawStat << "======================================================\n";
	rawStat << "______________________________________________________\n";
	rawStat << "Time (sec)" << std::setw(42-NUMDIGITS(totalDuration)) << " " 
		  << (int)totalDuration << "\n";
	rawStat << "______________________________________________________\n";
	rawStat << "Num analyzed variables" 
		  << std::setw(32-NUMDIGITS(numAnalyzedVar)) << " " 
		  << numAnalyzedVar << "\n";
	rawStat << "______________________________________________________\n";
	rawStat << "Num bugs found" << std::setw(40-NUMDIGITS(bugs.size())) << " " 
		  << bugs.size() << "\n";
	rawStat << "\n\n\n\n";

	int i=1;
	for(auto iter = bugs.begin(); iter != bugs.end(); ++iter, ++i) {
		const Bug *bug = *iter;

		rawStat << i << ". " << bug->getTypeName() << ":\n";
		rawStat << bug->toString();
		rawStat << "\n\n";
	}

	return rawStat.str();
}

std::string KMinerStat::checkerInfoToString() const {
	std::stringstream rawStat; 

	for(const auto &iter : checkerStats) {
		const CheckerStat &checkerStat = iter;
		rawStat << checkerStat.toString();
	}

	return rawStat.str();
}

std::string ContextStat::toString() const {
	std::stringstream rawStat; 

	rawStat << "______________________________________________________\n";
	if(cxtType == "SYSCALL") {
		rawStat << "API" << std::setw(52) << "SYSCALL\n";
		rawStat << "______________________________________________________\n";
		rawStat << "System call" << std::setw(43-cxtName.size()) << " " 
			<< cxtName << "\n";
	} else if (cxtType == "DRIVER") {
		rawStat << "API" << std::setw(52) << "DRIVER\n";
		rawStat << "______________________________________________________\n";
		rawStat << "Driver" << std::setw(48-cxtName.size()) << " " 
			<< cxtName << "\n";
	}

	return rawStat.str();
}

std::string KMinerStat::generalInfoToString() const {
	std::stringstream rawStat; 

	rawStat << "General Info:\n";
	rawStat << "======================================================\n";
	rawStat << "______________________________________________________\n";
	rawStat << "Module" << std::setw(48-moduleName.size()) << " " 
		<< moduleName << "\n";

	rawStat << contextStat.toString();

	rawStat << "______________________________________________________\n";
	rawStat << "Num Threads" << std::setw(43-NUMDIGITS(numThreads)) << " " 
		<< numThreads << "\n";

	rawStat << "\n\n\n";

	return rawStat.str();
}

std::string SystemStat::toString() const {
	std::stringstream rawStat; 

	rawStat << "\n";
	rawStat << "System Info:\n";
	rawStat << "======================================================\n";
	rawStat << "______________________________________________________\n";
	rawStat << "Kernel Name" << std::setw(43-kernelName.size()) << " " 
		<< kernelName << "\n";
	rawStat << "______________________________________________________\n";
	rawStat << "Kernel Release" << std::setw(40-kernelRelease.size()) << " " 
		<< kernelRelease<< "\n";
	rawStat << "______________________________________________________\n";
	rawStat << "Kernel Version\n" << std::setw(0) << " " 
		<< kernelVersion << "\n";
	rawStat << "______________________________________________________\n";
	rawStat << "Processor" << std::setw(45-processor.size()) << " " 
		<< processor << "\n";
	rawStat << "______________________________________________________\n";
	rawStat << "Num Cores" << std::setw(45-NUMDIGITS(numCores)) << " " 
		<< numCores << "\n";
	rawStat << "______________________________________________________\n";
	rawStat << "Used/Max/Total-Mem (kB)" 
		<< std::setw(29-NUMDIGITS(usedMem)-NUMDIGITS(maxUsedMem)-NUMDIGITS(memTotal)) << " " 
		<< usedMem << "/" << maxUsedMem << "/" << memTotal << "\n";

	rawStat << "\n\n\n";

	return rawStat.str();
}

std::string PartitionerStat::toString() const {
	std::stringstream rawStat; 
	
	rawStat << "Partitioner Statistic:\n";
	rawStat << "======================================================\n";
	rawStat << " Type      |                             |  R  |  O\n";
	rawStat << "======================================================\n";
	rawStat << " Initcalls |                             |" 
							<< std::setw(5) << numIR << "|" 
							<< std::setw(5) << numIO << "\n";
	rawStat << std::setw(11) << " " << "| Functions                   |" 
							<< std::setw(5) << numIRFuncs << "|" 
							<< std::setw(5) << numIOFuncs << "\n";
	rawStat << std::setw(11) << " " << "| Global Variables            |" 
							<< std::setw(5) << numIRGVs << "|" 
							<< std::setw(5) << numIOGVs << "\n";;
	rawStat << std::setw(11) << " " << "| Non Defined Variables       |" 
						 	<< std::setw(5) << numIRNDGVs << "|" 
							<< std::setw(5) << numIONDGVs << "\n";;
	rawStat << std::setw(11) << " " << "| Call Graph Depth            |" 
							<< std::setw(5) << iRCGDepth << "|" 
							<< std::setw(5) << iOCGDepth << "\n";;
	rawStat << "-----------+-----------------------------+-----+-----\n";
	
	rawStat << " API";

	rawStat << std::setw(6) << " " << " | Functions                   |" 
							<< std::setw(5) << numARFuncs << "|" 
							<< std::setw(5) << numAOFuncs << "\n";
	rawStat << std::setw(11) << " " << "| Global Variables            |" 
							<< std::setw(5) << numARGVs << "|" 
							<< std::setw(5) << numAOGVs << "\n";;
	rawStat << std::setw(11) << " " << "| Call Graph Depth            |" 
							<< std::setw(5) << aRCGDepth << "|" 
							<< std::setw(5) << aOCGDepth << "\n";;
	rawStat << "-----------+-----------------------------+-----+-----\n";
	rawStat << " Kernel    | Functions                   |" 
							<< std::setw(5) << numKRFuncs << "|" 
							<< std::setw(5) << numKOFuncs << "\n";
	rawStat << std::setw(11) << " " << "| Global Variables            |" 
							<< std::setw(5) << numKRGVs << "|" 
							<< std::setw(5) << numKOGVs << "\n";;
	rawStat << "======================================================\n";
	rawStat << "Legend:	R= relevant ; O= original\n";
	rawStat << "______________________________________________________\n";
	rawStat << "API Function Limit" << std::setw(36-NUMDIGITS(funcLimit)) << " " 
		<< funcLimit << "\n";

	rawStat << "\n\n\n";

	return rawStat.str();
}

std::string KMinerStat::blacklistToString() const {
	std::stringstream rawStat; 

	rawStat << "Blacklisted (Sub-)Function Names:\n";
	rawStat << "======================================================\n";

	int i = 1;
	for(auto iter = blacklist.begin(); iter != blacklist.end(); ++iter, ++i) {
		rawStat << *iter << ", ";

		if(i % 6 == 0)
			rawStat << "\n";
	}

	rawStat << "\n\n\n\n";

	return rawStat.str();
}

std::string KMinerStat::sinksToString() const {
	std::stringstream rawStat; 

	rawStat << "Sinks: \n";
	rawStat << "======================================================\n";

	int i=1;
	for(auto iter = sinks.begin(); iter != sinks.end(); ++iter, ++i) {
		rawStat << *iter << ", ";

		if(i % 3 == 0)
			rawStat << "\n";
	}
		
	rawStat << "\n\n";

	return rawStat.str();
}

std::string KMinerStat::totalTimeToString() const {
	std::stringstream rawStat; 

	rawStat << "======================================================\n";
	rawStat << "Total (sec) " << std::setw(42-NUMDIGITS(total_t)) << "  " << total_t << "\n";
	rawStat << "======================================================\n\n\n";

	return rawStat.str();
}

std::string KMinerStat::toString() const {
	std::stringstream rawStat; 

	rawStat << generalInfoToString();

	rawStat << checkerInfoToString();
	
	rawStat << systemStat.toString();

	rawStat << partitionerStat.toString();

	rawStat << blacklistToString();

	rawStat << sinksToString();

	rawStat << totalTimeToString();

	return rawStat.str();
}
