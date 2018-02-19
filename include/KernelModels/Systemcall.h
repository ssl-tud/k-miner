#ifndef SYSTEMCALL_H
#define SYSTEMCALL_H

#include "Util/BasicTypes.h"
#include "KernelModels/KernelContextObj.h"
#include <algorithm> 

class Systemcall;

typedef std::map<std::string, Systemcall> SyscallMap;

/***
 * Contains the used functions, global variables of the systemcall.
 */
class Systemcall: public KernelContextObj {
public:
	Systemcall(): KernelContextObj(CONTEXT_TYPE::SYSCALL) { }

	Systemcall(std::string name): KernelContextObj(CONTEXT_TYPE::SYSCALL, name) { 
		addToContextRoot(name);	
	}

	Systemcall(std::string 	name, 
		   const StringSet &functions, 
		   const StringSet &globalvars,
		   uint32_t maxCGDepth): 
		   KernelContextObj(CONTEXT_TYPE::SYSCALL, name) { 
		addToContextRoot(name);	
		setFunctions(functions);
		setGlobalVars(globalvars);
		setMaxCGDepth(maxCGDepth);
	}

	Systemcall(const Systemcall &syscall): 
		Systemcall(syscall.name, 
			   syscall.functions, 
			   syscall.globalvars,
			   syscall.maxCGDepth) { }

	virtual ~Systemcall() { }
		
	/**
	 * Get the alias of a systemcall. (e.g. sys_futex alias SyS_futex)
	 */
	std::string getAlias() const {
		return alias;
	}

	/**
	 * Set the alias of a systemcall.
	 */
	void setAlias(std::string alias) {
		this->alias = alias;	
		addToContextRoot(alias);
	}

	static inline bool classof(const Systemcall *syscall) {
		return true;
	}

	static inline bool classof(const KernelContextObj *cxtObj) {
		return cxtObj->getContextType() == CONTEXT_TYPE::SYSCALL;
	}

	virtual void dump() const {
		llvm::outs() << "==================\n";
		llvm::outs() << "Systemcall " << name << ":\n";
		llvm::outs() << "Num functions= " << functions.size() << "\n";
		llvm::outs() << "Num globalvars= " << globalvars.size() << "\n";
		llvm::outs() << "MaxCGDepth= " << maxCGDepth << "\n";
		llvm::outs() << "------------------\n";
	}

protected:
	std::string alias;
};

#endif // SYSTEMCALL_H
