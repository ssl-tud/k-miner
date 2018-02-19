#ifndef CONTEXT_BUILDER_H
#define CONTEXT_BUILDER_H

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/Cloning.h> 
#include "SVF/Util/PTACallGraph.h"
#include "Util/KernelAnalysisUtil.h"
#include "Util/CallGraphAnalysis.h"
#include "WPA/Andersen.h"
#include <omp.h>

/**
 * Abstract class which defines the functions required to handle contexts.
 */
class ContextBuilder {
public:
	ContextBuilder(llvm::Module &module): context(nullptr), module(module) { 
		pta = new PTACallGraph(&module);
	}

	virtual ~ContextBuilder() { 
		if(pta != nullptr)
			delete pta;	
		pta = nullptr;

		if(context != nullptr)
			delete context;	
		context = nullptr;
	}

	/**
	 * Determine the functions and variable (context) that have to be analyzed. 
	 */
	virtual void build(std::string cxtRootName) = 0;

	/**
	 * Remove irrelevant functions and variables from the context.
	 */
	virtual void optimizeContext() = 0;

	/**
	 * Set the Root that preceeds the contexts.
	 */
	void setContext(KernelContextObj *context) {
		if(this->context != nullptr)
			delete this->context;	
		this->context = context;
	}

	/**
	 * Get the Root that preceeds the contexts.
	 */
	KernelContextObj* getContext() const {
		return context;
	}

protected:

	/**
	 * Determines the context of the given function.
	 */
	void createContext(KernelContextObj *context);

	/**
	 * Remove irrelevant functions and variables using an
	 * andersen pointer analysis.
	 */
	void anderOpt(llvm::Module &module, KernelContextObj *context);

	/**
	 * Remove irrelevant functions and variables using a
	 * flow-sensitive pointer analysis.
	 */
	void flowOpt(llvm::Module &module, KernelContextObj *context);

	/**
	 * Reduces the depth of the call graph to stay below a given number of functions.
	 */
	void minimizeContext(llvm::Module &module, KernelContextObj *context);

	PTACallGraph *pta;
	KernelContextObj *context;
	llvm::Module &module;
};


#endif // CONTEXT_BUILDER_H
