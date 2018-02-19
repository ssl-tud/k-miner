#ifndef KERNEL_EXP_PASS_H_ 
#define KERNEL_EXP_PASS_H_ 

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "SVF/Util/BasicTypes.h"
#include "SVF/Util/ExtAPI.h"

typedef std::set<std::string> StringSet;

/***
 * Replaces the kernel specific allocation/free instruction of a module.
 */
class KernelExtPass : public llvm::ModulePass {		
private:
	/** 
	 * For all the kernel related allocations/frees we
	 * remove the function definitions and replace them
	 * with a declaration.
	 */
	bool transform();

	/** 
	 * Replaces a llvm function with another llvm function.
	 */
	void replaceFuncDefByDec(llvm::Function *F);

public:
	static char ID;

	KernelExtPass(char id = ID): ModulePass(ID), extAPI(ExtAPI::getExtAPI()) { }

	virtual ~KernelExtPass() { }

	virtual bool runOnModule(llvm::Module& module) {
		this->module = &module;
		return transform();
	}

	virtual const char* getPassName() const {
		return "Kernel Alloc Transform Pass";
	}

	virtual void getAnalysisUsage(llvm::AnalysisUsage& au) const {
		/// do not intend to change the IR in this pass,
		au.setPreservesAll();
	}

private:
	const ExtAPI *extAPI;

	llvm::Module *module;
};

#endif // KERNEL_EXP_PASS_H_ 
