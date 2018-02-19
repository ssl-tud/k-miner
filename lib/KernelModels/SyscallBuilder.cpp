#include "KernelModels/SyscallBuilder.h"

using namespace llvm;
using namespace analysisUtil;

void SyscallBuilder::build(std::string cxtRootName) {
	outs() << "____________________________________________________\n";
	outs() << "SYSCALLHANDLING:\n\n";

	// Will be removed by the ContextBuilder.
	Systemcall *syscall = new Systemcall(cxtRootName);

	// System calls typically have an alias (e.g SYSC_mmap)
	std::string alias = getAlias(module, syscall->getName());
	syscall->setAlias(alias);

	createContext(syscall);
	optimizeContext();
}

void SyscallBuilder::optimizeContext() {
	std::unique_ptr<llvm::Module> optModule = CloneModule(&module);
	anderOpt(*optModule.get(), context);
	flowOpt(*optModule.get(), context);
	minimizeContext(*optModule.get(), context);
	optModule.reset();
}

