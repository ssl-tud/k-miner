#ifndef KERNEL_CHECKER_API_H
#define KERNEL_CHECKER_API_H

#include "Util/KernelAnalysisUtil.h"

class KernelCheckerAPI {
private:
	static KernelCheckerAPI* ckAPI;

	void init();
public:
	KernelCheckerAPI() { 
		init();	
	}

	~KernelCheckerAPI() { }
	
	enum CHECKER_TYPE {
		CK_DUMMY = 0,		/// dummy type
		CK_KALLOC,		/// kernel memory allocation
		CK_KFREE,      		/// kernel memory deallocation
		CK_KLOCK,		/// File close
		CK_KUNLOCK,		/// File close
    	};

	typedef llvm::StringMap<CHECKER_TYPE> TDAPIMap;

	static KernelCheckerAPI* getCheckerAPI() {
		if(ckAPI == NULL) {
		    ckAPI = new KernelCheckerAPI();
		}

		return ckAPI;
	}

	bool isKAlloc(const llvm::Function* fun) const {
		return getType(fun) == CK_KALLOC;
	}

	bool isKAlloc(const llvm::Instruction *inst) const {
		return getType(analysisUtil::getCallee(inst)) == CK_KALLOC;
	}
	
	bool isKAlloc(llvm::CallSite cs) const {
		return isKAlloc(cs.getInstruction());
	}

	bool isKDealloc(const llvm::Function* fun) const {
		return getType(fun) == CK_KFREE;
	}

	bool isKDealloc(const llvm::Instruction *inst) const {
		return getType(analysisUtil::getCallee(inst)) == CK_KFREE;
	}

	bool isKDealloc(llvm::CallSite cs) const {
		return isKDealloc(cs.getInstruction());
	}

	bool isKLock(const llvm::Function* fun) const {
		return getType(fun) == CK_KLOCK;
	}

	bool isKLock(const llvm::Instruction *inst) const {
		return getType(analysisUtil::getCallee(inst)) == CK_KLOCK;
	}

	bool isKLock(llvm::CallSite cs) const {
		return isKLock(cs.getInstruction());
	}

	bool isKUnlock(const llvm::Function* fun) const {
		return getType(fun) == CK_KUNLOCK;
	}

	bool isKUnlock(const llvm::Instruction *inst) const {
		return getType(analysisUtil::getCallee(inst)) == CK_KUNLOCK;
	}

	bool isKUnlock(llvm::CallSite cs) const {
		return isKUnlock(cs.getInstruction());
	}

private:
	TDAPIMap tdAPIMap;

	CHECKER_TYPE getType(const llvm::Function* F) const {
		if(F) {
			TDAPIMap::const_iterator it= tdAPIMap.find(F->getName().split('.').first.str());
			if(it != tdAPIMap.end())
				return it->second;
		}

		return CK_DUMMY;
	}


};

#endif // KERNEL_CHECKER_API_H
