#include "Util/CallGraphAnalysis.h"
#include "Util/AnalysisUtil.h"
#include "Util/DebugUtil.h"
#include <llvm/IR/InstIterator.h>	// for inst iteration
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Operator.h>
#include <sstream>
#include <fstream>

StringSet LocalCallGraphAnalysis::blacklist;

static cl::opt<std::string> CGBlacklist("cg-ignore", cl::init(""),
		cl::desc("The Call-Graph Analysis will ignore this functions."));

void LocalCallGraphAnalysis::analyze(std::string startFunc, bool forward, 
						bool broad, uint32_t maxDepth) {

	this->broad = broad;
	const llvm::Function *F = getGraph()->getModule()->getFunction(startFunc);

	if(F == nullptr) {
		#pragma omp critical (findAliasFunc)
		F = findAliasFunc(startFunc);

		if(F == nullptr) {
//			errs() << "Warning: No Function \"" << startFunc << "\" found!\n";
			reset();
			return;
		}
	}

	#pragma omp critical (item)
	DPItem::setMaxBudget(maxDepth);
	const PTACallGraphNode *node = getGraph()->getCallGraphNode(F);	
	CGDPItem item(node->getId());
	item.addToVisited(node->getId());

	relevantGlobalVars.clear();
	callerToCallSiteInst.clear();

	if(forward)
		forwardTraverse(item);
	else
		backwardTraverse(item);
}

bool LocalCallGraphAnalysis::forwardChecks(const CGDPItem &item) {
	const PTACallGraphNode *node = getNode(item.getCurNodeID());
	std::string funcName = node->getFunction()->getName();

	if(!item.validDepth())
		return false;

	if(forwardFilter && forwardFilter->find(funcName) != forwardFilter->end())
		return false;

	// maybe we dont want to forward the whole graph.
	if(inBlackList(funcName)) 
		return false;

	return true;
}

void LocalCallGraphAnalysis::forwardTraverse(CGDPItem& item) {
	forwardFuncSlice.clear();
	forwardFuncPaths.clear();
	clearVisitedSet();
	pushIntoWorklist(item);

	while (!isWorklistEmpty()) {
		CGDPItem item = popFromWorklist();

		if(!forwardChecks(item)) {
			saveFuncPath(item, true);
			continue;
		}

		item.incDepth();

		forwardProcess(item);

		GNODE* v = getNode(item.getCurNodeID());
		auto EI = GTraits::child_begin(v);
		auto EE = GTraits::child_end(v);

		for (; EI != EE; ++EI) {
			forwardpropagate(item, *(EI.getCurrent()) );
		}
	}
}

void LocalCallGraphAnalysis::forwardProcess(const CGDPItem& item) {
	const PTACallGraphNode *node = getNode(item.getCurNodeID());
	const llvm::Function *F = node->getFunction();

//	outs() << "forward " << node->getId() << ": " << node->getFunction()->getName() << "\n";

	// add the function name to the relevantFuntion set.
	forwardFuncSlice.insert(F->getName());

	// If there are no further nodes on this path, save it.
	if(!node->hasOutgoingEdge())
		saveFuncPath(item, true);

	if(item.getDepth() > actualDepth) {
		#pragma omp critical (actualDetph)
		{
		if(item.getDepth() > actualDepth)
			actualDepth = item.getDepth();
		}
	}

	// Scans all the instructions in order to find global variables and function pointers.
	// if a function calls another function with a function as a parameter add this func. too.
        for (auto II = inst_begin(*F); II != inst_end(*F); ++II) {
		llvm::Instruction *inst = const_cast<llvm::Instruction*>(&*II);

		bool res = false; 

		res = analysisUtil::isCallSite(inst);

		// is it a function call?
            	if(res) {
			llvm::CallSite CS(cast<Value>(inst));
			handleCallSite(item, &CS);
		} else {
			handleAssignments(item, inst);
		}
	}
}

void LocalCallGraphAnalysis::forwardpropagate(const CGDPItem& item, GEDGE* edge) {
	const PTACallGraphNode *srcNode = getNode(edge->getSrcID());
	const PTACallGraphNode *dstNode = getNode(edge->getDstID());

	// Collect the callsite instructions of the current function.
	for(auto iter = edge->directCallsBegin(); iter != edge->directCallsEnd(); ++iter)
		callerToCallSiteInst[srcNode->getFunction()->getName()].insert(*iter);

	if(hasVisited(dstNode)) {
		saveFuncPath(item, true);
		return;
	}

	addVisited(dstNode);

	CGDPItem newItem(item);
	newItem.setCurNodeID(edge->getDstID());
	newItem.addToVisited(edge->getDstID());
	pushIntoWorklist(newItem);
}

void LocalCallGraphAnalysis::forwardpropagate(const CGDPItem& item, const PTACallGraphNode *node) {
	if(hasVisited(node)) {
		saveFuncPath(item, true);
		return;
	}
	
	addVisited(node);

	CGDPItem newItem(item);
	newItem.setCurNodeID(node->getId());
	newItem.addToVisited(node->getId());
	pushIntoWorklist(newItem);
}

void LocalCallGraphAnalysis::forwardpropagate(const CGDPItem& item, FuncSet &functions) {
	for(auto funcIter = functions.begin(); funcIter != functions.end(); ++funcIter) {
		const PTACallGraphNode *node =  getGraph()->getCallGraphNode(*funcIter);
		forwardpropagate(item, node);	
	}		
}

bool LocalCallGraphAnalysis::backwardCheck(const CGDPItem &item) {
	const PTACallGraphNode *node = getNode(item.getCurNodeID());
	std::string funcName = node->getFunction()->getName();

	// usually we dont want to backward the whole graph.
	if(backwardFilter && backwardFilter->find(funcName) != backwardFilter->end())
		return false;

	return true;
}


void LocalCallGraphAnalysis::backwardTraverse(CGDPItem& it) {
	backwardFuncSlice.clear();
	backwardFuncPaths.clear();
	clearVisitedSet();
	pushIntoWorklist(it);

	while (!isWorklistEmpty()) {
		CGDPItem item = popFromWorklist();

		if(!backwardCheck(item)) {
			saveFuncPath(item, false);
			continue;
		}

		backwardProcess(item);

		GNODE* v = getNode(item.getCurNodeID());
		auto EI = InvGTraits::child_begin(v);
		auto EE = InvGTraits::child_end(v);

		for (; EI != EE; ++EI) {
			backwardpropagate(item,*(EI.getCurrent()) );
		}
	}
}

void LocalCallGraphAnalysis::backwardProcess(const CGDPItem& item) {
	const PTACallGraphNode *node = getNode(item.getCurNodeID());
	std::string funcName = node->getFunction()->getName();

//	outs() << "backward " << node->getId() << ": " << node->getFunction()->getName() << "\n";

	// add the function name to the relevantFuntion set.
	backwardFuncSlice.insert(funcName);

	// If there are no further nodes on this path, save it.
	if(!node->hasIncomingEdge())
		saveFuncPath(item, false);
}

void LocalCallGraphAnalysis::backwardpropagate(const CGDPItem& item, GEDGE* edge) {
	const PTACallGraphNode *node = getNode(edge->getSrcID());

	if(hasVisited(node)) {
		saveFuncPath(item, false);
		return;
	} 
		
	addVisited(node);

	CGDPItem newItem(item);
	newItem.setCurNodeID(edge->getSrcID());
	newItem.addToVisited(edge->getSrcID());
	pushIntoWorklist(newItem);
}

void LocalCallGraphAnalysis::handleCallSite(const CGDPItem &item, llvm::CallSite *CS) {
	unsigned int num_args = CS->arg_size();

	// Check all arguments.
	for(int i=0; i < num_args; i++) {
		llvm::Value *arg = CS->getArgument(i);
		const Value* ref;
		#pragma omp critical (stripConstantCasts)
		ref = analysisUtil::stripConstantCasts(arg);
		handleCE(item, ref);
	}
}

void LocalCallGraphAnalysis::handleAssignments(const CGDPItem &item, llvm::Instruction *inst) {
	for(auto iter = inst->value_op_begin(); iter != inst->value_op_end(); ++iter) {
		llvm::Value *val = *iter;
		const Value* ref;
		#pragma omp critical (stripConstantCasts)
		ref = analysisUtil::stripConstantCasts(val);
		handleCE(item, ref);
	}
}

void LocalCallGraphAnalysis::handleCE(const CGDPItem &item, const Value *val) {
	const llvm::Module *M = getGraph()->getModule();

	if (const Constant* conVal = dyn_cast<Constant>(val)) {
		const Value* ref;
		#pragma omp critical (stripConstantCasts)
		ref = analysisUtil::stripConstantCasts(conVal);

		const ConstantExpr* ce;
		#pragma omp critical (isGepConstantExpr)
		ce = analysisUtil::isGepConstantExpr(ref);

		if (ce) {
			// handle the recursive constant express case
			// like (gep (bitcast (gep X 1)) 1); the inner gep is ce->getOperand(0)
			handleCE(item, ce->getOperand(0));
		} else {
			const llvm::Function *fun = M->getFunction(ref->getName());

			if(fun != nullptr && broad) 
				forwardpropagate(item, getGraph()->getCallGraphNode(fun));

			if(isa<GlobalVariable>(ref) && !isRelevantGlobalVar(ref) && !isFunctionPtr(ref)) {
				relevantGlobalVars.insert(ref->getName());
				handleGlobalCE(item, dyn_cast<GlobalVariable>(ref));
			}
		}
	} else {
		const llvm::Function *fun = M->getFunction(val->getName());

		if(fun != nullptr && broad) 
			forwardpropagate(item, getGraph()->getCallGraphNode(fun));
	}
}

void LocalCallGraphAnalysis::handleGlobalCE(const CGDPItem &item, const GlobalVariable *G) {
	assert(G);

	//The type this global points to
	const Type *T = G->getType()->getContainedType(0);
	bool is_array = 0;

	//An array is considered a single variable of its type.
	while(const ArrayType *AT = dyn_cast<ArrayType>(T)) {
		T = AT->getElementType();
		is_array = 1;
	}

	if(isa<StructType>(T)) {
		//A struct may be used in constant GEP expr.
		for (Value::const_user_iterator it = G->user_begin(), ie = G->user_end();
				it != ie; ++it) {
			handleCE(item, *it);
		}
	} else if(is_array) {
		for (Value::const_user_iterator it = G->user_begin(), ie =
				G->user_end(); it != ie; ++it) {
			handleCE(item, *it);
		}
	}

	if (G->hasInitializer()) {
		handleGlobalInitializerCE(item, G->getInitializer(), 0);
	}
}

void LocalCallGraphAnalysis::handleGlobalInitializerCE(const CGDPItem &item, const Constant *C, u32_t offset) {

	if (C->getType()->isSingleValueType() && isa<PointerType>(C->getType())) {
		const Value *C1;
		#pragma omp critical (stripConstantCasts)
		C1 = analysisUtil::stripConstantCasts(C);

		if (const ConstantExpr *E = dyn_cast<ConstantExpr>(C1)) {
			handleCE(item, E);
		}
		else {
			handleCE(item, C1);
		}
	} else if (isa<ConstantArray>(C)) {
		for (u32_t i = 0, e = C->getNumOperands(); i != e; i++) {
			handleGlobalInitializerCE(item, cast<Constant>(C->getOperand(i)), offset);
		}
	} else if (isa<ConstantStruct>(C)) {
		const StructType *sty = cast<StructType>(C->getType());
		const std::vector<u32_t> *offsetvect;

		#pragma omp critical (symbolTableInfo)
	       	offsetvect = &SymbolTableInfo::Symbolnfo()->getStructOffsetVec(sty);
			
		for (u32_t i = 0, e = C->getNumOperands(); i != e; i++) {
			u32_t off = (*offsetvect)[i];
			handleGlobalInitializerCE(item, cast<Constant>(C->getOperand(i)),
					offset + off);
		}
	}
}

void LocalCallGraphAnalysis::saveFuncPath(const CGDPItem &item, bool forward) {
	NodeIdList itemPath = item.getPath();	
	StringList funcPath;

	for(auto iter : itemPath) {
		const PTACallGraphNode *node = getNode(iter);
		const llvm::Function *F = node->getFunction();

		funcPath.push_back(F->getName());	
	}

	if(forward)
		forwardFuncPaths.insert(funcPath);
	else
		backwardFuncPaths.insert(funcPath);

}

const StringSet& LocalCallGraphAnalysis::getForwardFuncSlice() const {
	return forwardFuncSlice;
}

const StringSet& LocalCallGraphAnalysis::getBackwardFuncSlice() const {
	return backwardFuncSlice;
}

const StrListSet& LocalCallGraphAnalysis::getForwardFuncPaths() const {
	return forwardFuncPaths;
}

const StrListSet& LocalCallGraphAnalysis::getBackwardFuncPaths() const {
	return backwardFuncPaths;
}

const StringSet& LocalCallGraphAnalysis::getRelevantGlobalVars() const {
	return relevantGlobalVars;
}

bool LocalCallGraphAnalysis::isRelevantGlobalVar(const llvm::Value *v) const {
	return relevantGlobalVars.find(v->getName()) != relevantGlobalVars.end();
}

const InstSet& LocalCallGraphAnalysis::getCallSiteInstSet(std::string callerName) {
	return callerToCallSiteInst[callerName];	
}

bool LocalCallGraphAnalysis::isFunctionPtr(const llvm::Value *v) const {
	const llvm::Module *M = getGraph()->getModule();
	std::string valueName = v->getName();

	if(v->hasName() && valueName.find("descriptor") != std::string::npos)
		return true;

	return M->getFunction(v->getName()) != nullptr;
}

llvm::Function* LocalCallGraphAnalysis::findAliasFunc(std::string funcName) {
	llvm::Module *M = getGraph()->getModule();
	std::string aliaseeName;

	for(auto iter = M->alias_begin(); iter != M->alias_end(); ++iter) {
		if((*iter).getName() == funcName) {
			aliaseeName = analysisUtil::stripAllCasts((*iter).getAliasee())->getName();
			return M->getFunction(aliaseeName);
		}
	}
	
	return nullptr;
}

bool LocalCallGraphAnalysis::inBlackList(std::string funcName) const {
	for(auto iter = LocalCallGraphAnalysis::blacklist.begin(); 
			iter !=  LocalCallGraphAnalysis::blacklist.end(); ++iter) {
		if(funcName.find(*iter) != std::string::npos)
			return true;
	}
	return false;
//	return LocalCallGraphAnalysis::blacklist.find(funcName) != 
//		LocalCallGraphAnalysis::blacklist.end();	
}

bool LocalCallGraphAnalysis::hasBlackList() const {
	return !LocalCallGraphAnalysis::blacklist.empty(); 
}

void LocalCallGraphAnalysis::importBlackList() {
	std::string fileName = CGBlacklist;

	#pragma omp critical (import_blacklist)
	{
	if(!hasBlackList() && fileName != ""/*&& CGBLACKLIST*/) {
		std::ifstream ifs(fileName);

		assert(ifs.good() && "No blacklist found");

		std::stringstream buffer;
		buffer << ifs.rdbuf();
		std::string funcName;

		while(std::getline(buffer, funcName, '\n')) {
			LocalCallGraphAnalysis::blacklist.insert(funcName);	
		}
	}
	kminerStat->setBlackList(blacklist);
	}
}
