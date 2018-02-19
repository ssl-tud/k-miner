#ifndef DRIVER_H
#define DRIVER_H

#include "Util/BasicTypes.h"
#include "KernelModels/KernelContextObj.h"
#include <algorithm> 

class Driver;

typedef std::map<std::string, Driver> DriverMap;

/***
 * Contains the used functions, global variables of the driver.
 */
class Driver: public KernelContextObj {
public:
	Driver(): KernelContextObj(CONTEXT_TYPE::DRIVER) { }

	Driver(std::string name): KernelContextObj(CONTEXT_TYPE::DRIVER, name) { }

	Driver(std::string name, 
		const StringSet &cxtRoot,
		const StringSet &functions, 
		const StringSet &globalvars,
		uint32_t maxCGDepth): 
		KernelContextObj(CONTEXT_TYPE::DRIVER, name) { 
			setContextRoot(cxtRoot);	
			setFunctions(functions);
			setGlobalVars(globalvars);
			setMaxCGDepth(maxCGDepth);
		}

	Driver(const Driver &driver): 
		Driver(driver.name, 
			   driver.cxtRoot, 
			   driver.functions, 
			   driver.globalvars,
			   driver.maxCGDepth) { }

	virtual ~Driver() { }

	static inline bool classof(const Driver *driver) {
		return true;
	}

	static inline bool classof(const KernelContextObj *cxtObj) {
		return cxtObj->getContextType() == CONTEXT_TYPE::DRIVER;
	}
	
	virtual void dump() const {
		llvm::outs() << "==================\n";
		llvm::outs() << "Driver " << name << ":\n";
		llvm::outs() << "Num API functions= " << cxtRoot.size() << "\n";
		llvm::outs() << "Num functions= " << functions.size() << "\n";
		llvm::outs() << "Num globalvars= " << globalvars.size() << "\n";
		llvm::outs() << "MaxCGDepth= " << maxCGDepth << "\n";
		llvm::outs() << "------------------\n";
	}
};

#endif // DRIVER_H
