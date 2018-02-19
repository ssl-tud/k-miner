#ifndef INSTRUMENTATION_UTIL_H
#define INSTRUMENTATION_UTIL_H

#include "SVF/Util/BasicTypes.h"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstrTypes.h>	
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>	
#include <llvm/IR/CallSite.h>
#include <llvm/ADT/SparseBitVector.h>	
#include <llvm/IR/Dominators.h>	
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include <llvm/Support/Debug.h>	

namespace instrumentationUtil {

/**
 * Inserts a CallSite to a BasicBlock.
 */
void addCallSite(llvm::Function *F, llvm::BasicBlock *entryBlock, llvm::BasicBlock::iterator pos); 

/**
 * Instruments a single pointer instruction.
 */
llvm::AllocaInst* initLocal(llvm::PointerType *ptrTy, 
			llvm::BasicBlock *label_entry, 
			llvm::Instruction *posInst,
			int rec_counter); 
/**
 * Instruments a single structure type instruction.
 */
void initStructType(llvm::StructType *structTy, 
			llvm::BasicBlock *label_entry, 
			llvm::Instruction *posInst,
			int rec_counter);

/**
 * Replaces the parameter of a function with new local variables.
 */
void initializeParameterLocal(llvm::Function *F);

};

#endif // INSTRUMENTATION_UTIL_H
