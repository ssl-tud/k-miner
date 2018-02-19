#include "Util/SVFGAnalysisUtil.h"
#include <sstream>
#include <iostream>

using namespace llvm;

bool svfgAnalysisUtil::isNullStoreNode(const SVFGNode *node) {
	for(const auto &iter : node->getInEdges()) {
		if(iter->getSrcNode()->getId() == 0)
			return true;
	}

	return false;
}

bool svfgAnalysisUtil::isParamStore(const SVFGNode *node) {
	bool hasFormalParamNode = false;
	bool hasAddrNode = false;

	if(!llvm::isa<StoreSVFGNode>(node) && !node->hasIncomingEdge()) 
		return false;

	for(auto edgeIter = node->InEdgeBegin(); edgeIter != node->InEdgeEnd(); ++edgeIter) {
		const SVFGNode *srcNode = (*edgeIter)->getSrcNode();

		if(const InterPHISVFGNode *phiNode = llvm::dyn_cast<InterPHISVFGNode>(srcNode)) {
			if(phiNode->isFormalParmPHI())
				hasFormalParamNode= true;
		}

		if(llvm::isa<AddrSVFGNode>(srcNode))
			hasAddrNode = true;
	}
	
	return hasFormalParamNode && hasAddrNode;
}

bool svfgAnalysisUtil::isParamAddr(const SVFGNode *node) {
	if (!node->hasOutgoingEdge())
		return false;

	if(llvm::isa<AddrSVFGNode>(node)) {
		for(auto edgeIter = node->OutEdgeBegin(); edgeIter != node->OutEdgeEnd(); ++edgeIter)
			if(isParamStore((*edgeIter)->getDstNode()))
				return true;

	}

	return false;
}

bool svfgAnalysisUtil::isArg(const SVFGNode *node) {
	std::string valueName = getSVFGValueName(node);

	return valueName.find("arg") != std::string::npos;
}

bool svfgAnalysisUtil::isParamAddr(SVFG *svfg, const PAGNode *pagNode) {
	if(const SVFGNode *node = svfg->getDefSVFGNode(pagNode)) 
		return isParamAddr(node);
	
	return false;
}

const SVFGNode* svfgAnalysisUtil::getSVFGNodeDef(SVFG *svfg, const SVFGNode* node) {
	if(const StmtSVFGNode* stmtNode = llvm::dyn_cast<StmtSVFGNode>(node)) 
		return svfg->getDefSVFGNode(stmtNode->getPAGSrcNode());

	return nullptr;
}

std::string svfgAnalysisUtil::getSVFGNodeType(const SVFGNode *node) {
	if(isa<StmtSVFGNode>(node)) {
		if(isa<AddrSVFGNode>(node))
			return "AddrSVFGNode";
		if(isa<CopySVFGNode>(node))
			return "CopySVFGNode";
		if(isa<GepSVFGNode>(node))
			return "GepSVFGNode";
		if(isa<StoreSVFGNode>(node))
			return "StoreSVFGNode";
		if(isa<LoadSVFGNode>(node))
			return "LoadSVFGNode";
		
		return "StmtSVFGNode";
	}

	if(isa<ActualParmSVFGNode>(node))
		return "ActualParmSVFGNode";

	if(isa<FormalParmSVFGNode>(node))
		return "FormalParmSVFGNode";

	if(isa<ActualRetSVFGNode>(node))
		return "ActualRetSVFGNode";

	if(isa<FormalRetSVFGNode>(node))
		return "FormalRetSVFGNode";

	if(isa<MRSVFGNode>(node)) {
		if(isa<FormalINSVFGNode>(node))
			return "FormalINSVFGNode";
		if(isa<FormalOUTSVFGNode>(node))
			return "FormalOUTSVFGNode";
		if(isa<ActualINSVFGNode>(node))
			return "ActualINSVFGNode";
		if(isa<ActualOUTSVFGNode>(node))
			return "ActualOUTSVFGNode";
		if(isa<MSSAPHISVFGNode>(node)) {
			if(isa<IntraMSSAPHISVFGNode>(node))
				return "IntraMMSAPHISVFGNode"; 
			if(isa<InterMSSAPHISVFGNode>(node))
				return "InterMMSAPHISVFGNode"; 

			return "MSSAPHISVFGNode";
		}
	} 

	if(isa<PHISVFGNode>(node)) {
		if(isa<IntraPHISVFGNode>(node))
			return "IntraPHISVFGNode";
		if(isa<InterPHISVFGNode>(node))
			return "InterPHISVFGNode";

		return "PHISVFGNode";
	}

	if(isa<NullPtrSVFGNode>(node))
		return "NullPtrSVFGNode";	
	
	return "UNKNOWN TYPE";
}

std::string svfgAnalysisUtil::getSVFGValueName(const SVFGNode *node) {
	if(const StmtSVFGNode *stmtNode = dyn_cast<StmtSVFGNode>(node)) {
		const PAGNode* pagNode = stmtNode->getPAGSrcNode();
		std::string valueNameStr;
		raw_string_ostream valueNameSS(valueNameStr);

		if(pagNode->hasValue()) {
			const Value* v = pagNode->getValue();

			if(v->hasName()) {
				valueNameSS << v->getName();
				return valueNameSS.str();
			}
		}
	}
	
	return "no Name";
}

// Returns the Location of a SVFGNode.
std::string svfgAnalysisUtil::getSVFGSourceLoc(const SVFGNode *node) {
	if(const StmtSVFGNode *stmtNode = dyn_cast<StmtSVFGNode>(node)) {
		const PAGNode* pagNode = stmtNode->getPAGSrcNode();
		const llvm::Value *V = dyn_cast<llvm::Value>(stmtNode->getInst());

		if(pagNode->hasValue())
			V = pagNode->getValue();

		return analysisUtil::getSourceLoc(V);
	} else if(const ActualParmSVFGNode *aParamNode = dyn_cast<ActualParmSVFGNode>(node)) {
		const Value* V = dyn_cast<llvm::Value>(aParamNode->getCallSite().getInstruction());
		return analysisUtil::getSourceLoc(V);
	}
	
	return "Unknown";
}

u32_t svfgAnalysisUtil::getSVFGSourceLine(const SVFGNode *node) {
	if(const StmtSVFGNode *stmtNode = dyn_cast<StmtSVFGNode>(node)) {
		const PAGNode* pagNode = stmtNode->getPAGDstNode();

		if(pagNode->hasValue()) {
			const Value* v = pagNode->getValue();
			return analysisUtil::getSourceLine(v);
		}
	} else if(const ActualParmSVFGNode *aParamNode = dyn_cast<ActualParmSVFGNode>(node)) {
		const Value* v = dyn_cast<llvm::Value>(aParamNode->getCallSite().getInstruction());
		return analysisUtil::getSourceLine(v);
	}
	
	return 0;
}

std::string svfgAnalysisUtil::getSVFGSourceFileName(const SVFGNode *node) {
	if(const StmtSVFGNode *stmtNode = dyn_cast<StmtSVFGNode>(node)) {
		const PAGNode* pagNode = stmtNode->getPAGSrcNode();

		if(pagNode->hasValue()) {
			const Value* v = pagNode->getValue();
			return analysisUtil::getSourceFileName(v);
		}
	} else if(const ActualParmSVFGNode *aParamNode = dyn_cast<ActualParmSVFGNode>(node)) {
		const Value* v = dyn_cast<llvm::Value>(aParamNode->getCallSite().getInstruction());
		return analysisUtil::getSourceFileName(v);
	}
	
	return "Unknown";
}

const llvm::Function* svfgAnalysisUtil::getSVFGFunction(const SVFGNode *node) {
	const llvm::BasicBlock *BB = node->getBB();

	if(BB != nullptr) 
		return BB->getParent();

	return nullptr;
}

std::string svfgAnalysisUtil::getSVFGFuncName(const SVFGNode* node) {
	const llvm::Function *F = getSVFGFunction(node);
	std::string funcName = "Unknown";

	// Get next function name if  possible.
	if(F)
		funcName = F->getName();

	return funcName;
}

void drawCall(std::stringstream &rawPath, std::string arrow, std::string funcName, int depth) {
	std::stringstream rawDepth;

	for(int i=0; i < depth; i++) {
		if(i == depth-1)
			rawDepth << "\t";
		else
			rawDepth << "\t.";
	}

	rawPath << rawDepth.str() << arrow << " " <<  funcName << "\n"; 
}

void drawAssign(std::stringstream &rawPath, int line, int depth) {
	std::stringstream rawDepth;

	for(int i=0; i < depth; i++) {
		if(i == depth-1)
			rawDepth << "\t";
		else
			rawDepth << "\t.";
	}

	rawPath << rawDepth.str() << "+\n" << rawDepth.str() 
		<< "++> assignment (ln: " << line << ")\n" << rawDepth.str() << "+\n";
}

std::string svfgAnalysisUtil::getFuncPathStr(SVFG *svfg, const CxtNodeIdList &path, bool forward) {
	std::stringstream rawPath; 
	std::string firstFuncName = getSVFGFuncName(svfg->getSVFGNode(path.begin()->second));
	int depth = 1;

	rawPath << firstFuncName << "\n";
	
	for(auto iter = path.begin(); iter != path.end(); ++iter) {
		NodeID curId = iter->second;
		NodeID cxt = iter->first;
		const SVFGNode *curNode = svfg->getSVFGNode(curId);
		auto nextIter = std::next(iter, 1);

		if(nextIter != path.end()) {
			NodeID nextId = nextIter->second;
			const SVFGNode *nextNode= svfg->getSVFGNode(nextId);

			if(!forward) {
				const SVFGNode *tmp = curNode;
				curNode = nextNode;
				nextNode = tmp;
			}

			std::string curFuncName = getSVFGFuncName(curNode);
			std::string nextFuncName = getSVFGFuncName(nextNode);
			int curLine = getSVFGSourceLine(curNode);

			if(svfg->getSVFGEdge(curNode, nextNode, SVFGEdge::DirCall)) {
				drawCall(rawPath, "===>", nextFuncName, depth);

				if(isa<StoreSVFGNode>(curNode) && !isParamStore(curNode))
					drawAssign(rawPath, curLine, depth);

				depth++;
			} else if(svfg->getSVFGEdge(curNode, nextNode, SVFGEdge::IndCall)) {
				drawCall(rawPath, "--->", nextFuncName, depth);

				if(isa<StoreSVFGNode>(curNode) && !isParamStore(curNode))
					drawAssign(rawPath, curLine, depth);

				depth++;
			} else if(svfg->getSVFGEdge(curNode, nextNode, SVFGEdge::DirRet)) {
				depth = depth > 1 ? depth-1 : 0;

				if(isa<StoreSVFGNode>(curNode) && !isParamStore(curNode))
					drawAssign(rawPath, curLine, depth);

				drawCall(rawPath, "<===", curFuncName, depth);
			} else if(svfg->getSVFGEdge(curNode, nextNode, SVFGEdge::IndRet)) {
				depth = depth > 1 ? depth-1 : 0;

				if(isa<StoreSVFGNode>(curNode) && !isParamStore(curNode))
					drawAssign(rawPath, curLine, depth);

				drawCall(rawPath, "<---", curFuncName, depth);
			} else if(isa<StoreSVFGNode>(curNode) && !isParamStore(curNode))
					drawAssign(rawPath, curLine, depth);
		}
	}

	return rawPath.str();
}

std::string svfgAnalysisUtil::getFuncPathStr(SVFG *svfg, const ContextCond &funcPath, bool forward) {
	std::stringstream rawPath; 

	if(funcPath.cxtSize() < 1)
		return "";

	std::string firstFuncName = getSVFGFuncName(svfg->getSVFGNode(*funcPath.begin()));
	int depth = 1;

	rawPath << firstFuncName << "\n";
	
	for(auto iter = funcPath.begin(); iter != funcPath.end(); ++iter) {
		const SVFGNode *curNode = svfg->getSVFGNode(*iter);
		std::string curFuncName = getSVFGFuncName(curNode);
		auto nextIter = std::next(iter, 1);

		if(nextIter != funcPath.end()) {
			const SVFGNode *nextNode = svfg->getSVFGNode(*nextIter);
			std::string nextFuncName = getSVFGFuncName(nextNode);
			const auto edges = nextNode->getInEdges();

			for(const auto &iter2 : edges) {
				if(iter2->isCallDirectVFGEdge()) {
					drawCall(rawPath, "===>", nextFuncName, depth);
					depth++;
					break;
				} else if(iter2->isCallIndirectVFGEdge()) {
					drawCall(rawPath, "--->", nextFuncName, depth);
					depth++;
					break;
				} else if(iter2->isRetDirectVFGEdge()) {
					depth = depth > 1 ? depth-1 : 0;
					drawCall(rawPath, "<===", curFuncName, depth);
					break;
				} else if(iter2->isRetIndirectVFGEdge()) {
					depth = depth > 1 ? depth-1 : 0;
					drawCall(rawPath, "<---", curFuncName, depth);
					break;
				} 
			}
		}
	}

	return rawPath.str();
}

std::string svfgAnalysisUtil::getFuncPathStr(SVFG *svfg, const EdgeVec &funcPath, bool forward) {
	std::stringstream rawPath; 
	EdgeVec path = funcPath;

	if(!forward)
		std::reverse(path.begin(), path.end());

	if(path.size() < 1)
		return "";

	std::string firstFuncName = getSVFGFuncName(svfg->getSVFGNode(path.begin()->first));
	rawPath << firstFuncName << "\n";

	for(auto iter = path.begin(); iter != path.end(); ++iter) {
		const SVFGNode *curNode = svfg->getSVFGNode(iter->first);
		const SVFGNode *nextNode = svfg->getSVFGNode(iter->second);
		const SVFGNode *tmp;

		if(!forward) {
			tmp = curNode;
			curNode = nextNode;
			nextNode = tmp;
		}

		std::string curFuncName = getSVFGFuncName(curNode);
		std::string nextFuncName = getSVFGFuncName(nextNode);
		if(svfg->getSVFGEdge(curNode, nextNode, SVFGEdge::SVFGEdgeK::DirCall))
			drawCall(rawPath, "===>", nextFuncName, 0);
		else if(svfg->getSVFGEdge(curNode, nextNode, SVFGEdge::SVFGEdgeK::IndCall)) 
			drawCall(rawPath, "--->", nextFuncName, 0);
		else if(svfg->getSVFGEdge(curNode, nextNode, SVFGEdge::SVFGEdgeK::DirRet)) 
			drawCall(rawPath, "<===", nextFuncName, 0);
		else if(svfg->getSVFGEdge(curNode, nextNode, SVFGEdge::SVFGEdgeK::IndRet)) 
			drawCall(rawPath, "<---", nextFuncName, 0);
//		rawPath << "(" << curNode->getId() << ") " << getSVFGNodeType(curNode) << "\n";

	}

	return rawPath.str();
}
