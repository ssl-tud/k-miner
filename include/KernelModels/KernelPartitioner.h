#ifndef KERNEL_PARTITIONER_H
#define KERNEL_PARTITIONER_H

#include "Util/KMinerStat.h"
#include "Util/KernelAnalysisUtil.h"
#include "Util/DebugUtil.h"
#include "Util/CallGraphAnalysis.h"
#include "KernelModels/ContextBuilder.h"
#include "KernelModels/KernelContext.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

typedef std::set<std::string> StringSet;

/***
 * Calls the SystemcallMinimiizer and InitcallPartitioner and
 * reduce the kernel module to only the relevant functions
 * and global variables. It also adds a callsite of the initcalls
 * to the entry of the systemcall function in the module.
 */
class KernelPartitioner: public llvm::ModulePass {
public:
	static char ID;

	KernelPartitioner(char id = ID): ModulePass(ID) { 
		kernelCxt = KernelContext::getKernelContext();
	} 

	virtual ~KernelPartitioner() { }

	/**
	 * Define all elements.
	 */
	void initialize(llvm::Module &module);

	/**
	 * Set some statistics.
	 */
	void finalize();

	/**
	 * Creates a PAG an dumps some pointer statistic.
	 */
	void dumpPAGStat(llvm::Module &module);

	virtual bool runOnModule(llvm::Module& module) {

		llvm::outs() << "\n\nPartitioner:\n";
		llvm::outs() << "====================================================\n";
		
		initialize(module);

		mergeContexts();

//		handleAPIParameter();

		addInitcallsToAPICxt();

		finalize();

		return true;
	}

	virtual const char* getPassName() const {
		return "Module Partitioner Pass";
	}

	virtual void getAnalysisUsage(llvm::AnalysisUsage& au) const {
		/// do not intend to change the IR in this pass,
		au.setPreservesAll();
	}

private:

	/**
	 * Combines the contexts of the API and the initcalls.
	 */
	void mergeContexts();

	/**
	 * Removes the irrelevant initcalls.
	 */
	void filterRelevantInitcalls(InitcallMap &initcalls, const KernelContextObj *api);

	/** 
	 * Inserts all the relevant "__initcall_"-functions 
	 * to the systemcall function as a CallSite.
	 */
	void addInitcallsToAPICxt();

	/**
	 * Initialize the parameter as local variables.
	 */
	void handleAPIParameter();

	// Kernel relevant infos. (systemcall, initcalls, ...)
	KernelContext *kernelCxt;

	PartitionerStat partStat;

	llvm::Module *module;
};

#endif // MODULE_PARTITIONER_H_
