#include "KernelModels/KernelExtPass.h"
#include "Util/DebugUtil.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

char KernelExtPass::ID = 0;

static RegisterPass<KernelExtPass> KERNELALLOCTRANSFORMPASS("kernel-alloc-transform-pass",
								"KernelExtPass");

bool KernelExtPass::transform() {
	StringSet workset;
	StringSet kernelAllocFunctions = extAPI->get_functions(ExtAPI::EFT_KALLOC);
	StringSet kernelFreeFunctions = extAPI->get_functions(ExtAPI::EFT_KFREE);
	StringSet kernelLockFunctions = extAPI->get_functions(ExtAPI::EFT_KLOCK);
	StringSet kernelUnlockFunctions = extAPI->get_functions(ExtAPI::EFT_KUNLOCK);
//	StringSet kernelDelFunctions = extAPI->get_functions(ExtAPI::EFT_KDEL);

	workset.insert(kernelAllocFunctions.begin(), kernelAllocFunctions.end());
	workset.insert(kernelFreeFunctions.begin(), kernelFreeFunctions.end());
	workset.insert(kernelLockFunctions.begin(), kernelLockFunctions.end());
	workset.insert(kernelUnlockFunctions.begin(), kernelUnlockFunctions.end());

	std::set<llvm::Function*> irrelevantFunctions;

	for(auto iter = workset.begin(); iter != workset.end(); ++iter) {
		std::string funcSubName = *iter; 

		for(auto iter2 = module->begin(); iter2 != module->end(); ++iter2) {
			llvm::Function *F = &*iter2;

			if(F == nullptr)
				continue;

			std::string funcName = F->getName();

			if(funcName.find(funcSubName) != std::string::npos) {
				irrelevantFunctions.insert(F);
			}
		}
	}

	for(auto iter = irrelevantFunctions.begin(); iter != irrelevantFunctions.end(); ++iter)
		replaceFuncDefByDec(*iter);

	return !workset.empty();
}

void KernelExtPass::replaceFuncDefByDec(llvm::Function *F) {
	std::string FName = F->getName();
	llvm::FunctionType *FT = F->getFunctionType();

	llvm::Function *newF = Function::Create(FT, Function::ExternalLinkage, FName, module);

	F->replaceAllUsesWith(newF);
	// Might be necessary
//	F->deleteBody();
//	F->drepAllReferences();
	F->eraseFromParent();

	// We have to set the name of the new function back to the original,
	// because it will be created with "<function name>.1".
	newF->setName(FName);
}
