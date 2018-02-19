#include "Util/PtrCallSetAnalysis.h"
#include "Util/PAGAnalysis.h"
#include "Util/KernelAnalysisUtil.h"

using namespace llvm;
using namespace analysisUtil;

bool PtrCallSetAnalysis::analyze(std::string globalVariableName) {
	const PAGNodeSet &globalObjSet = pagAnalysis->getGlobalObjPNodes();
	const PAGNode *globalNode = nullptr;
	const ConstraintNode *consNode = nullptr;
	reset();

	for(auto iter = globalObjSet.begin(); iter != globalObjSet.end(); ++iter) {
		if(globalVariableName == PAGAnalysis::getValueName(*iter))
			globalNode = *iter;
	}

	if(globalNode == nullptr)
		return false;

	consNode = getGraph()->getConstraintNode(globalNode->getId());

	if(consNode == nullptr)
		return false;

	DPItem item(consNode->getId()); 

	forwardTraverse(item);

	for(auto iter = backwardNodes.begin(); iter != backwardNodes.end(); ++iter) {
		DPItem item((*iter)->getId()); 
		backwardTraverse(item);
	}

	return true;
}

void PtrCallSetAnalysis::collectFunction(const llvm::Function *F) {
	if(!F) return;

	std::string funcName = F->getName(); 
	relevantFunctions.insert(funcName);
}

void PtrCallSetAnalysis::collectFunction(llvm::CallSite CS) {
	const llvm::Function *F = CS.getCalledFunction();
	collectFunction(F);
}

void PtrCallSetAnalysis::collectVariables(const DPItem &item, ConstraintEdge *edge) {
	const PAGNode* curNode = pag->getPAGNode(item.getCurNodeID());
	auto &inEdges = curNode->getInEdges();
//	const llvm::Function *F = pagAnalysis->getFunctionOfEdge(*edge);

	if (!isa<DummyValPN>(curNode) && !isa<DummyObjPN>(curNode) && !isa<GepObjPN>(curNode)) {
		if(const ObjPN* objPN = llvm::dyn_cast<ObjPN>(curNode)) {
			const MemObj* memObj = objPN->getMemObj();
			std::string name = PAGAnalysis::getValueName(curNode);

			if(memObj->isGlobalObj() || memObj->isHeap()) { 
				relevantGlobalVars.insert(name);
			} 
//			else if(memObj->isStack() && curNode->hasValue() && F) {
//				relevantLocalVars[F->getName()].insert(name);
//			}
		}
	}

}

void PtrCallSetAnalysis::collectBackwardNodes(ConstraintNode *curNode, ConstraintNode *prevNode) {
	auto &inEdges = curNode->getInEdges();

	if(inEdges.size() > 1) {
		for(auto iter = inEdges.begin(); iter != inEdges.end(); ++iter) {
			ConstraintEdge *edge = *iter;

			if(prevNode->getId() != edge->getSrcID())
				backwardNodes.insert(edge->getSrcNode());
		}
	}
}

inline void PtrCallSetAnalysis::forwardProcess(const DPItem &item) {
//	const ConstraintNode *curNode = getNode(item.getCurNodeID());
//	auto &outEdges = curNode->getOutEdges();

	if(!pag->findPAGNode(item.getCurNodeID()))
		return;

	const PAGNode *curNode = pag->getPAGNode(item.getCurNodeID());
	auto &outEdges = curNode->getOutEdges();
	
	for(auto iter = outEdges.begin(); iter != outEdges.end(); ++iter) {
		PAGEdge *edge = *iter;
		const llvm::Function *F = pagAnalysis->getFunctionOfEdge(*edge);
		collectFunction(F);
	}
}

void PtrCallSetAnalysis::forwardpropagate(const DPItem &item, ConstraintEdge *edge) {
	ConstraintNode* curNode = getNode(item.getCurNodeID());
	ConstraintNode* nextNode = edge->getDstNode();
        DPItem newItem(item);
        newItem.setCurNodeID(edge->getDstID());

	if(hasForwardVisited(nextNode->getId()))
		return;

	addToForwardVisited(nextNode->getId());

	collectBackwardNodes(nextNode, curNode);

        pushIntoWorklist(newItem);
}

inline void PtrCallSetAnalysis::backwardProcess(const DPItem &item) {

	if(!pag->findPAGNode(item.getCurNodeID()))
		return;

	const PAGNode* curNode = pag->getPAGNode(item.getCurNodeID());
	auto &inEdges = curNode->getInEdges();

	for(auto iter = inEdges.begin(); iter != inEdges.end(); ++iter) {
		PAGEdge *edge = *iter;
		const llvm::Function *F = pagAnalysis->getFunctionOfEdge(*edge);
		collectFunction(F);
	}
}

void PtrCallSetAnalysis::backwardpropagate(const DPItem &item, ConstraintEdge *edge) {
	const ConstraintNode* curNode = getNode(item.getCurNodeID());
	const ConstraintNode* nextNode = edge->getSrcNode();
        DPItem newItem(item);
        newItem.setCurNodeID(edge->getSrcID());

	if(hasBackwardVisited(nextNode->getId()))
		return;

	addToBackwardVisited(nextNode->getId());

	collectVariables(item, edge);

        pushIntoWorklist(newItem);
}

