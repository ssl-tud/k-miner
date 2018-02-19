#include "Util/InstrumentationUtil.h"

using namespace llvm;

// How many fields of an argument should be recursively allocated.
static const int arg_rec_limit = 3;

void instrumentationUtil::addCallSite(llvm::Function *F, llvm::BasicBlock *entryBlock, 
					llvm::BasicBlock::iterator pos) {
	llvm::IRBuilder<> builder(entryBlock, pos);
	std::vector<llvm::Value*> args;

	if (!F) return;
	
	for(auto argIter = F->arg_begin(); argIter != F->arg_end(); ++argIter) {
		llvm::Value *arg = llvm::Constant::getNullValue(argIter->getType());
		args.push_back(arg);	
	}

	builder.CreateCall(F, args);
}


void instrumentationUtil::initializeParameterLocal(llvm::Function *F) {
	llvm::BasicBlock &label_entry = F->getEntryBlock();

	if (!F) return;

	for(auto &iter : F->getArgumentList()) {
		std::string argName = iter.getName();
		llvm::Type *ty = iter.getType();
		int rec_counter = 0;

		if(llvm::PointerType *ptrTy = dyn_cast<llvm::PointerType>(ty)) {
			llvm::Instruction *I = initLocal(ptrTy, &label_entry, 
						&*label_entry.begin(), rec_counter);
			if(I) {
				iter.replaceAllUsesWith(I);
			}
		}
	}
}

llvm::AllocaInst* instrumentationUtil::initLocal(llvm::PointerType *ptrTy, 
				      llvm::BasicBlock *label_entry, 
				      llvm::Instruction *posInst,
				      int rec_counter) {
	llvm::Type *elemTy = ptrTy->getElementType();
	AllocaInst* ptr_arg = nullptr;
	std::string newArgName = "arg";

	if(llvm::StructType *structTy = dyn_cast<llvm::StructType>(elemTy)) {
  		ptr_arg = new AllocaInst(structTy, newArgName);
		ptr_arg->insertAfter(posInst);
		ptr_arg->setAlignment(8);
		initStructType(structTy, label_entry, ptr_arg, rec_counter);
	} 
// TODO if we want allow arguments other than structures.
//	else {
// 		ptr_arg = new AllocaInst(elemTy, newArgName);
//		ptr_arg->insertAfter(posInst);
//		ptr_arg->setAlignment(8);
//	}

	return ptr_arg;
}

void instrumentationUtil::initStructType(llvm::StructType *structTy, 
					 llvm::BasicBlock *label_entry, 
					 llvm::Instruction *posInst,
					 int rec_counter) {

	ConstantInt* const_int0 = ConstantInt::get(label_entry->getContext(),
						   APInt(32, StringRef("0"), 10));

	int i = 0;

	if(rec_counter > arg_rec_limit)
		return;
	else
		rec_counter++;

	for(auto iter = structTy->element_begin(); iter != structTy->element_end(); ++iter) {
		llvm::Type *ty = *iter;
		std::string i_str = std::to_string(i);

		// Only handle pointer fields.
		if(llvm::PointerType *ptrTy = dyn_cast<llvm::PointerType>(ty)) {
			AllocaInst *ptrInst = initLocal(ptrTy, label_entry, posInst, rec_counter);
			
			if(!ptrInst) {
				i++;
				continue;
			}

			// Index of the current field.
			ConstantInt* const_int_index = ConstantInt::get(label_entry->getContext(), 
								APInt(32, StringRef(i_str), 10));
			std::vector<Value*> constants;
			constants.push_back(const_int0);
			constants.push_back(const_int_index);

			GetElementPtrInst* subInst = GetElementPtrInst::Create(structTy, 
									       posInst, 
									       constants, 
									       "sub_arg");
			subInst->insertAfter(ptrInst);

 			StoreInst* storeInst = new StoreInst(ptrInst, 
							     subInst, 
							     false, 
							     subInst->getNextNode());
		}

		i++;
	}
}
