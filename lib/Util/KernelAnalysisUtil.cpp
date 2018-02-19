#include "Util/KernelAnalysisUtil.h"
#include <llvm/Transforms/Utils/Local.h>
#include <llvm/IR/Module.h>	
#include <llvm/Support/CommandLine.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Analysis/CFG.h>	
#include <llvm/IR/CFG.h>
#include "SVF/Util/Conditions.h"
#include <sys/resource.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace llvm;


bool analysisUtil::isValueTypePointer(const llvm::Value *v) {
	if(!v)
		return false;

	// GlobalVariables have always type pointer, even if they aren't.
	// Therefore we have to check the underlying value type.
	if(const llvm::GlobalVariable *gv = dyn_cast<llvm::GlobalVariable>(v))
		return gv->getValueType()->isPointerTy();

	return v->getType()->isPointerTy();
}

bool analysisUtil::hasValueTypePointer(const llvm::Value *v) {
	llvm::Type *ty;

	if(!v)
		return false;

	if(isValueTypePointer(v))
		return true;

	// GlobalVariables have always type pointer, even if they aren't.
	// Therefore we have to check the underlying value type.
	if(const llvm::GlobalVariable *gv = dyn_cast<llvm::GlobalVariable>(v))
		ty = gv->getValueType();
	else
		ty = v->getType();
	
	if(llvm::StructType *structTy = dyn_cast<llvm::StructType>(ty))
		return hasStructTypePointer(structTy);	
	
	return false;	
}

bool analysisUtil::hasStructTypePointer(llvm::StructType *structTy) {
	for(auto iter = structTy->element_begin(); iter != structTy->element_end(); ++iter) {
		llvm::Type *ty = *iter;

		if(ty->isPointerTy())
			return true;

		if(llvm::StructType *structTy = dyn_cast<llvm::StructType>(ty))
			return hasStructTypePointer(structTy);
	}

	return false;
}

/*!
 * Get Subprogram of a function according to debug info
 */
DISubprogram* analysisUtil::getDISubprogramOfFunction(const llvm::Function *F) 
{
	NamedMDNode* CU_Nodes = F->getParent()->getNamedMetadata("llvm.dbg.cu");
	if(CU_Nodes) {
		for (unsigned i = 0, e = CU_Nodes->getNumOperands(); i != e; ++i) {
			DICompileUnit *CUNode = cast<DICompileUnit>(CU_Nodes->getOperand(i));
			for (DISubprogram *SP : CUNode->getSubprograms()) {
				if (SP->describes(F))
					return SP;
			}
		}

	}

	return NULL;
}

/*!
 * Get source code line number of a function according to debug info
 */
u32_t analysisUtil::getSourceLineOfFunction(const llvm::Function *F) 
{
	u32_t line = 0;

	DISubprogram *SP = getDISubprogramOfFunction(F);

	if(SP)
		line = SP->getLine();

	return line;
}

/*!
 * Get the line number info of a LLVM value
 */
u32_t analysisUtil::getSourceLine(const Value* val) {
	if(val==NULL)  return 0;

	u32_t line = 0;

	if (const Instruction *inst = dyn_cast<Instruction>(val)) {
		if (isa<AllocaInst>(inst)) {
			DbgDeclareInst* DDI = llvm::FindAllocaDbgDeclare(const_cast<Instruction*>(inst));
			if (DDI) {
				DIVariable *DIVar = cast<DIVariable>(DDI->getVariable());
				line = DIVar->getLine();
			}
		}
		else if (MDNode *N = inst->getMetadata("dbg")) { // Here I is an LLVM instruction
			DILocation* Loc = cast<DILocation>(N);                   // DILocation is in DebugInfo.h
			line = Loc->getLine();
		}
	}
	else if (const GlobalVariable* gvar = dyn_cast<GlobalVariable>(val)) {
		NamedMDNode* CU_Nodes = gvar->getParent()->getNamedMetadata("llvm.dbg.cu");
		if(CU_Nodes) {
			for (unsigned i = 0, e = CU_Nodes->getNumOperands(); i != e; ++i) {
				DICompileUnit *CUNode = cast<DICompileUnit>(CU_Nodes->getOperand(i));
				for (DIGlobalVariable *GV : CUNode->getGlobalVariables()) {
					if (gvar == GV->getVariable())
						line = GV->getLine();
				}
			}
		}
	}
	else if (const Function* func = dyn_cast<Function>(val)) {
		line = getSourceLineOfFunction(func);
	}

	return line;
}

/*!
 * Get source code file name of a function according to debug info
 */
std::string analysisUtil::getSourceFileNameOfFunction(const llvm::Function *F) 
{
	std::string fileName = "Unknown";

	DISubprogram *SP = getDISubprogramOfFunction(F);

	if(SP)
		fileName = SP->getFilename();

	return fileName;
}


/*!
 * Get the file name info of a LLVM value
 */
std::string analysisUtil::getSourceFileName(const Value* val) {
	if(val==NULL)  return 0;

	std::string fileName = "Unknown";

	if (const Instruction *inst = dyn_cast<Instruction>(val)) {
		if (isa<AllocaInst>(inst)) {
			DbgDeclareInst* DDI = llvm::FindAllocaDbgDeclare(const_cast<Instruction*>(inst));
			if (DDI) {
				DIVariable *DIVar = cast<DIVariable>(DDI->getVariable());
				fileName = DIVar->getFilename();
			}
		}
		else if (MDNode *N = inst->getMetadata("dbg")) { // Here I is an LLVM instruction
			DILocation* Loc = cast<DILocation>(N);                   // DILocation is in DebugInfo.h
			fileName = Loc->getFilename();
		}
	}
	else if (const GlobalVariable* gvar = dyn_cast<GlobalVariable>(val)) {
		NamedMDNode* CU_Nodes = gvar->getParent()->getNamedMetadata("llvm.dbg.cu");
		if(CU_Nodes) {
			for (unsigned i = 0, e = CU_Nodes->getNumOperands(); i != e; ++i) {
				DICompileUnit *CUNode = cast<DICompileUnit>(CU_Nodes->getOperand(i));
				for (DIGlobalVariable *GV : CUNode->getGlobalVariables()) {
					if (gvar == GV->getVariable())
						fileName = GV->getFilename();
				}
			}
		}
	}
	else if (const Function* func = dyn_cast<Function>(val)) {
		fileName = getSourceFileNameOfFunction(func);
	}

	return fileName;
}

/*!
 * Removes the irrelevant functions and global variables from the module.
 */
void analysisUtil::minimizeModule(llvm::Module &module, const StringSet &relevantFuncs, const StringSet &relevantGVs) {
	std::set<llvm::Function*> irrelevantFunctions;
	std::set<llvm::GlobalVariable*> irrelevantGlobalVars;

	for(auto iter = module.global_begin(); iter != module.global_end(); ++iter) {
		llvm::GlobalVariable &GV = *iter;

		if(relevantGVs.find(GV.getName()) == relevantGVs.end()) 
			irrelevantGlobalVars.insert(&GV);

		//TODO Cause problems during the creation of the SVFG. Seems to be a bug in SVF. 
		if(GV.getName() == "dev_attr_tolerant" || 
		   GV.getName() == "dev_attr_monarch_timeout" || 
		   GV.getName() == "dev_attr_ignore_ce" || 
		   GV.getName() == "dev_attr_cmci_disabled" ||
		   GV.getName() == "irq_stack_ptr" || 
		   GV.getName() == "init_per_cpu__irq_stack_union")
			irrelevantGlobalVars.insert(&GV);
	}	

		// Is necessary because of a bug in the eraseFromParent function.
	for(auto iter = irrelevantGlobalVars.begin(); iter != irrelevantGlobalVars.end(); ++iter) {
		llvm::GlobalVariable *GV = *iter;

		GV->replaceAllUsesWith(UndefValue::get(GV->getType()));
		GV->eraseFromParent();
	}	

	for(auto iter = module.begin(); iter != module.end(); ++iter) {
		llvm::Function &F = *iter;

		if(relevantFuncs.find(F.getName()) == relevantFuncs.end()) 
			irrelevantFunctions.insert(&F);
	}	

	// Is necessary because of a bug in the eraseFromParent function.
	for(auto iter = irrelevantFunctions.begin(); iter != irrelevantFunctions.end(); ++iter) {
		llvm::Function *F = *iter;

		// TODO Cause problems if we want to remove it. seems to be a bug in llvm
		if(F->getName() == "__bpf_prog_run")
			continue;
		// Doesn't work in all cases. (seems to be a bug)	
		F->deleteBody();
		F->replaceAllUsesWith(UndefValue::get(F->getType()));

		F->eraseFromParent();
	}	
}
/*!
 * Removes the variables that are not a structure pointer or an array nor a structure containing a 
 * nother structure pointer.
 */
void analysisUtil::filterNonStructTypes(const llvm::Module &module, StringSet &globalvars) {
	for(auto iter = globalvars.begin(); iter != globalvars.end();) {
		const GlobalVariable *GV = module.getNamedGlobal(*iter);
		llvm::Type *ty = GV->getValueType();

		if(llvm::PointerType *ptrTy = dyn_cast<llvm::PointerType>(ty)) {
			if(isa<llvm::StructType>(ty->getPointerElementType()) ||
				isa<llvm::ArrayType>(ty->getPointerElementType()))
				++iter;
			else
				iter = globalvars.erase(iter);
		} else if(llvm::StructType *structTy = dyn_cast<llvm::StructType>(ty)) {
			if(analysisUtil::hasStructTypePointer(structTy))
				++iter;
			else
				iter = globalvars.erase(iter);
		} else
			iter = globalvars.erase(iter);
	}
}

std::string analysisUtil::getAlias(const llvm::Module &module, std::string funcName) {
	std::string alias = "";
	auto aliasIterB = module.alias_begin();
	auto aliasIterE = module.alias_end();

	// It might be an alias
	for(;aliasIterB != aliasIterE; ++aliasIterB) {
		if((*aliasIterB).getName() == funcName) {
			std::string aliaseeName = 
				analysisUtil::stripAllCasts((*aliasIterB).getAliasee())->getName();

			if(module.getFunction(aliaseeName) != nullptr) {
				alias = aliaseeName;
				break;
			}
		}
	}

	return alias;
}
