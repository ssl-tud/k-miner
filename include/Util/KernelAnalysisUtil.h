#ifndef KERNEL_ANALYSIS_UTIL_H
#define KERNEL_ANALYSIS_UTIL_H

#include "SVF/Util/ExtAPI.h"
#include "SVF/Util/AnalysisUtil.h"
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
#include <llvm/Support/Debug.h>	
#include <time.h>

#define NUMDIGITS(n) ({n > 0 ? (int) log10 ((double) n) + 1 : 1;})

namespace analysisUtil {

/// Return true if the call is a lock or not
inline bool isLockExtFun(const llvm::Function *fun) {
    return fun && (ExtAPI::getExtAPI()->is_lock(fun));
}

inline bool isLockExtCall(const llvm::CallSite cs) {
    return isLockExtFun(getCallee(cs));
}

inline bool isLockExtCall(const llvm::Instruction *inst) {
    return isLockExtFun(getCallee(inst));
}

/// Return true if the call is a unlock or not
inline bool isUnlockExtFun(const llvm::Function *fun) {
    return fun && (ExtAPI::getExtAPI()->is_unlock(fun));
}

inline bool isUnlockExtCall(const llvm::CallSite cs) {
    return isUnlockExtFun(getCallee(cs));
}

inline bool isUnlockExtCall(const llvm::Instruction *inst) {
    return isUnlockExtFun(getCallee(inst));
}

/// Removes the irrelevant functions and global variables from the module.
void minimizeModule(llvm::Module &module, const StringSet &relevantFuncs, const StringSet &relevantGVs);

/// Removes the variables that are not a structure pointer or an array nor a structure containing a
//  nother structure pointer.
void filterNonStructTypes(const llvm::Module &module, StringSet &globalvars);

// Pointer checks on value type
bool isValueTypePointer(const llvm::Value *v);
bool hasValueTypePointer(const llvm::Value *v);
bool hasStructTypePointer(llvm::StructType *structTy);

/// Get Subprogram of a function according to debug info
llvm::DISubprogram* getDISubprogramOfFunction(const llvm::Function *F);

/// Return source code including line number and file name from debug information
std::string  getSourceFileName(const llvm::Value *val);
std::string  getSourceFileNameOfFunction(const llvm::Function *F);
u32_t getSourceLine(const llvm::Value *val);
u32_t getSourceLineOfFunction(const llvm::Function *F);

/// Seeks for an alias of a function name.
std::string getAlias(const llvm::Module &module, std::string funcName);
};

#endif // KERNEL_ANALYSIS_UTIL_H
