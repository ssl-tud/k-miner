#include "Util/PAGAnalysis.h"
#include "Util/DebugUtil.h"
#include "Util/KernelAnalysisUtil.h"

using namespace llvm;
using namespace debugUtil;
using namespace analysisUtil;

PAGAnalysis* PAGAnalysis::pagAnalysis = nullptr;

const PAGNodeSet& PAGAnalysis::getGlobalObjPNodes() const {
	return globalObjPNodes;
}

const PAGNodeSet& PAGAnalysis::getLocalObjPNodes() const {
	return localObjPNodes;
}

bool PAGAnalysis::hasCallSite(std::string name) const {
	const PAG *pag = getGraph();
	const PAG::CallSiteSet &callsites = pag->getCallSiteSet();

	for(auto iter = callsites.begin(); iter != callsites.end(); ++iter) {
		const llvm::Function *F = (*iter).getCalledFunction();

		if(F && F->hasName() && F->getName() == name)
			return true;
	}

	return false;
}

const llvm::Function* PAGAnalysis::getFunctionOfEdge(const PAGEdge &edge) const {
	const llvm::Instruction *inst = edge.getInst();

	if(inst) {
		if(isExtCall(inst)) {
			llvm::CallSite CS = getLLVMCallSite(inst);
			return CS.getCalledFunction();
		}

		const llvm::BasicBlock *BB = inst->getParent();

		if(BB) {
			const llvm::Function *F = BB->getParent();	

			return F;
		}
	}

	return nullptr;
}

PAG::PAGEdgeSet PAGAnalysis::findEdgesOfFunction(std::string funcName) const {
	const PAG *pag = getGraph();
	PAG::PAGEdgeSet edges;
	
	for(auto pagIter = pag->begin(); pagIter != pag->end(); ++pagIter) {
		const PAGNode* curNode = pagIter->second;
		auto &outEdges = curNode->getOutEdges();

		for(auto iter = outEdges.begin(); iter != outEdges.end(); ++iter) {
			const PAGEdge *edge = *iter;
			const llvm::Function *F = getFunctionOfEdge(*edge);

			if(F && F->getName() == funcName) {
				edges.insert(*iter);
			}
		}
	}

	return edges;
}

void PAGAnalysis::splitNodesByMemObj() {
	PAG *pag = getGraph();

	for(auto pagIter = pag->begin(); pagIter != pag->end(); ++pagIter) {
		const PAGNode* pagNode = pagIter->second;
		
		if (!isa<DummyValPN>(pagNode) && !isa<DummyObjPN>(pagNode) && !isa<GepObjPN>(pagNode)) {
			if(const ObjPN* objPN = llvm::dyn_cast<ObjPN>(pagNode)) {
				const MemObj* memObj = objPN->getMemObj();
				if(memObj->isGlobalObj()) 
					globalObjPNodes.insert(pagNode);
				if(memObj->isHeap()) 
					globalObjPNodes.insert(pagNode);
				if(memObj->isStack()) 
					localObjPNodes.insert(pagNode);
			}
		} 
	}
}

std::string PAGAnalysis::getValueName(const PAGNode *node) {
	if(node->hasValue()) {
		const Value* v = node->getValue();

		if(v->hasName()) 
			return v->getName();
	}
	
	return "";
}
