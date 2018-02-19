#include "Util/ReportPass.h"
#include "Util/SystemInfoUtil.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <thread>

using namespace llvm;
using namespace systemInfoUtil;
using namespace analysisUtil;

char ReportPass::ID = 0;

static cl::opt<std::string> ReportFile("report", cl::init(""),
		cl::desc("Write all reports to this file"));

static cl::opt<bool> MONITOR_SYSTEM("monitor-system", cl::init(false),
		cl::desc("Captures the resources used by the process"));


thread *monitorThread;
SystemStat systemStat;


void ReportPass::makeReport() {
	writeToReport(REPORT_BANNER);
	writeToReport(stat->toString());
}

void ReportPass::writeToReport(std::string msg) {
	if(ReportFile != "") {
		std::ofstream file;
		file.open(ReportFile, std::ios::app);
		file << msg;
		file.close();
	}
}

void ReportPass::collectSystemInfo() {
	std::string kernelName = getKernelName();	
	std::string kernelRelease = getKernelRelease();	
	std::string kernelVersion = getKernelVersion();
	std::string processor = getMachine();
	uint32_t numCores = getNumCores();	
	uint32_t memTotal = getMemTotal();

	systemStat.setKernelName(kernelName);
	systemStat.setKernelRelease(kernelRelease);
	systemStat.setKernelVersion(kernelVersion);
	systemStat.setProcessor(processor);
	systemStat.setNumCores(numCores);
	systemStat.setMemTotal(memTotal);
	KMinerStat::createKMinerStat()->setSystemStat(systemStat);
}

/**
 * Helper function that monitors the memory of the current process.
 */
void monitorMemory() {
	KMinerStat *syscallStat = KMinerStat::createKMinerStat();

	while(MONITOR_SYSTEM) {
		FILE *file = fopen("/proc/self/status", "r");
		int result = -1;
		char line[128];

		while(fgets(line, 128, file) != nullptr) {
			if(strncmp(line, "VmSize:", 7) == 0) {
				int i = strlen(line);
				const char *p = line;
				while(*p < '0' || *p > '9') p++;
				line[i-3] = '\0';
				result = atoi(p);
				break;
			}
		}

		systemStat.updateUsedMem(result);
		std::this_thread::sleep_for (std::chrono::seconds(10));
		fclose(file);
	}
}

void ReportPass::startMonitoringSystem() {
	if(MONITOR_SYSTEM)
		monitorThread = new thread(monitorMemory);
}

void ReportPass::stopMonitoringSystem() {
	MONITOR_SYSTEM = false;	
	if(monitorThread) {
		monitorThread->join();	
		delete monitorThread;
	}

	monitorThread = nullptr;
}

