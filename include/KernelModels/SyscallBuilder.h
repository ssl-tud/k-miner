#ifndef SYSCALL_BUILDER_H
#define SYSCALL_BUILDER_H

#include "KernelModels/ContextBuilder.h"
#include "KernelModels/Systemcall.h"

/**
 * Abstract class which defines the functions required to handle contexts.
 */
class SyscallBuilder: public ContextBuilder {
public:
	SyscallBuilder(llvm::Module &module): ContextBuilder(module) { }

	virtual ~SyscallBuilder() { }

	/**
	 * Determine the functions and variable (context) that have to be analyzed. 
	 */
	virtual void build(std::string cxtRootName);

	/**
	 * Remove irrelevant functions and variables from the context.
	 */
	virtual void optimizeContext();
};


#endif // SYSCALL_BUILDER_H
