#ifndef KERNEL_CONTEXT_FACTORY_H
#define KERNEL_CONTEXT_FACTORY_H

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include "Util/BasicTypes.h"
#include "Util/KMinerStat.h"
#include "KernelModels/KernelContext.h"
#include "KernelModels/KernelContextObj.h"
#include "KernelModels/ContextBuilder.h"
#include "KernelModels/InitcallFactory.h"

/**
 * Defines the API that is to be analyzed and builds the contexts of it. 
 * The same is done for the initcalls.
 */
class KernelContextFactory {
public:
	KernelContextFactory(llvm::Module &module): 
		kernelCxt(nullptr), apiBuilder(nullptr), initcallFactory(nullptr), module(module) {
		initialize();
	}

	~KernelContextFactory() { 
		if(kernelCxt != nullptr)
			KernelContext::releaseKernelContext();
		kernelCxt = nullptr;

		if(apiBuilder != nullptr)
			delete apiBuilder;	
		apiBuilder = nullptr;

		if(initcallFactory != nullptr)
			InitcallFactory::releaseInitcallFactory();
		initcallFactory = nullptr;
	}

	/**
	 * Create the context for the next API functions to be analyzed and reset the initcalls.
	 */
	bool updateContext();

private:
	
	/**
	 * Sets up the KernelContextFactory and builds initcalls.
	 */
	void initialize();

	/**
	 * Set the original values of the kernel, before the module (kernel) is minimized.
	 * Determines the number of functions, global variables and non defined variables
	 * and adds them to the KernelContext.
	 */
	void initKernelContext();

	/**
	 * Determine the functions that should be analyzed next.
	 * Either a system call or multiple functions of a driver api.
	 */
	std::string getNextCxtRoot();

	StringList rawAPIList;
	ContextBuilder *apiBuilder; 
	InitcallFactory *initcallFactory;
	KernelContext *kernelCxt;
	ContextStat cxtStat;
	llvm::Module &module;
};

#endif // KERNEL_FACTORY_BUILDER_H
