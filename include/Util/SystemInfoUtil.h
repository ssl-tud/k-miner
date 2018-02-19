#ifndef SYSTEM_INFO_UTIL_H
#define SYSTEM_INFO_UTIL_H

#include "Util/BasicTypes.h"
#include <sys/utsname.h>
#include <fstream>
#include <thread>

namespace systemInfoUtil {

std::string getKernelName() {
	struct utsname buf;

	if(uname(&buf))
		return "";

	return std::string(buf.sysname);
}

unsigned long getMemTotal() {
	std::string token;
	std::ifstream file("/proc/meminfo");
	while(file >> token) {
		if(token == "MemTotal:") {
			unsigned long mem;
			if(file >> mem) {
				return mem;
			} else {
				return 0;       
			}
		}
		// ignore rest of the line
		file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	return 0; // nothing found
}

std::string getKernelRelease() {
	std::string token;
	std::ifstream file("/proc/version");

	while(file >> token) {
		if(token == "version") {
			std::string release;

			if(file >> release) {
				return release;
			} else {
				return "0.0.0-00-generic";       
			}
		}
	}

	return "0.0.0-00-generic";       
}

std::string getKernelVersion() {
	struct utsname buf;

	if(uname(&buf))
		return "";

	return std::string(buf.version);
}

std::string getMachine() {
	struct utsname buf;

	if(uname(&buf))
		return "";

	return std::string(buf.machine);
}

uint32_t getNumCores() {
	return std::thread::hardware_concurrency();
}

};

#endif // SYSTEM_INFO_UTIL_H
