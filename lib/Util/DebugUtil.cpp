#include "Util/DebugUtil.h"
#include "Util/SVFGAnalysisUtil.h"
#include <sstream>

using namespace llvm;
using namespace svfgAnalysisUtil;

std::string debugUtil::getPAGNodeType(const PAGNode *pagNode) {
	if (!isa<DummyValPN>(pagNode) && !isa<DummyObjPN>(pagNode) && !isa<GepObjPN>(pagNode)) {
		if(const ObjPN* objPN = llvm::dyn_cast<ObjPN>(pagNode)) {
			const MemObj* memObj = objPN->getMemObj();
			if(memObj->isGlobalObj()) 
				return "GlobalObj";
			if(memObj->isHeap()) 
				return "Heap";
			if(memObj->isStack()) 
				return "Stack";
			if(memObj->isFunction()) 
				return "Function";
			if(memObj->isStaticObj()) 
				return "Static";
			if(memObj->isStruct()) 
				return "Struct";
			if(memObj->isArray()) 
				return "Array";
			if(memObj->isVarStruct()) 
				return "VarStruct";
			if(memObj->isVarArray()) 
				return "VarArray";
			if(memObj->isConstStruct()) 
				return "ConstStruct";
			if(memObj->isConstArray()) 
				return "ConstArray";
			if(memObj->isConstant()) 
				return "Constant";
			if(memObj->isFieldInsensitive()) 
				return "FieldInsensitive";
			if(memObj->hasPtrObj()) 
				return "hasPtrObj";
		}
	} 
	return "no type";
}

void debugUtil::printNodeIdSet(SVFG *svfg, const NodeIdSet &set, std::string intro_str) {
	llvm::outs() << intro_str << ":\n";

	for(auto iter = set.begin(); iter != set.end(); ++iter) {
		llvm::outs() << *iter << "(" << getSVFGSourceLoc(svfg->getSVFGNode(*iter)) << ")" << " ";
		SVFGNode *node = svfg->getSVFGNode(*iter);
		llvm::outs() << getSVFGNodeType(node) << " ";
		if(node->getBB()) {
			llvm::outs() << " in FUNCTION= " << node->getBB()->getParent()->getName();
//			node->getBB()->dump();
		}
		outs() << "\n";
	}

	llvm::outs() << "--------------------------------------------------------\n";
}

void debugUtil::printNodeIdList(SVFG *svfg, const NodeIdList &lst, std::string intro_str) {
	llvm::outs() << intro_str << ":\n";

	for(auto iter = lst.begin(); iter != lst.end(); ++iter) {
		llvm::outs() << *iter << "(" << getSVFGSourceLoc(svfg->getSVFGNode(*iter)) << ")" << " ";
		SVFGNode *node = svfg->getSVFGNode(*iter);
		llvm::outs() << getSVFGNodeType(node) << " ";
		if(node->getBB()) {
			llvm::outs() << " in FUNCTION= " << node->getBB()->getParent()->getName();
//			node->getBB()->dump();
		}
		outs() << "\n";
	}

	llvm::outs() << "--------------------------------------------------------\n";
}

void debugUtil::printCxtNodeIdSet(SVFG *svfg, const CxtNodeIdSet &set, std::string intro_str) {
	llvm::outs() << intro_str << ":\n";

	for(auto iter = set.begin(); iter != set.end(); ++iter) {
		llvm::outs() << "(" << iter->first << "," << iter->second << "):"  
			<< "(" << getSVFGSourceLoc(svfg->getSVFGNode(iter->second)) << ")" << " ";
		SVFGNode *node = svfg->getSVFGNode(iter->second);
		llvm::outs() << getSVFGNodeType(node) << " ";
		if(node->getBB()) {
			llvm::outs() << " in FUNCTION= " << node->getBB()->getParent()->getName();
//			node->getBB()->dump();
		}
		outs() << "\n";
	}

	llvm::outs() << "--------------------------------------------------------\n";
}

void debugUtil::printCxtNodeIdSetSimple(SVFG *svfg, const CxtNodeIdSet &set, std::string intro_str) {
	llvm::outs() << intro_str << ":\n";

	for(auto iter = set.begin(); iter != set.end(); ++iter) {
		llvm::outs() << "(" << iter->first << "," << iter->second << "), ";  
	}

	outs() << "\n";
}

void debugUtil::printSVFGNodeSet(const SVFGNodeSet &set, std::string intro_str) {
	llvm::outs() << intro_str << ":\n";

	for(auto setIter = set.begin(); setIter != set.end(); ++setIter) {
		outs() << "\t";
		printSVFGNode(*setIter);
	}
}

void debugUtil::printSVFGNode(const SVFGNode *node) {
	llvm::outs() << node->getId() << "=: ";
	if(const StmtSVFGNode *stmtNode = dyn_cast<StmtSVFGNode>(node)) {
		llvm::outs() << getSVFGValueName(stmtNode);
		llvm::outs() << " ("<< stmtNode->getPAGSrcNodeID() << " -> " 
		       << stmtNode->getPAGDstNodeID() << ")"; 
	}
	llvm::outs() << " " << getSVFGNodeType(node) << " ";
	llvm::outs() << "\n\tLocation=: " << getSVFGSourceLoc(node) << "\n";
}

void debugUtil::printSVFGNodeMap(const SVFGNodeMap &map, std::string intro_str) {
	outs() << intro_str << ":\n";

	for(auto mapIter = map.begin(); mapIter != map.end(); ++mapIter) {
		NodeID id = mapIter->first;
		const SVFGNode *node = mapIter->second;

		outs() << "\t" << id << ": " << node->getId() << "\n";
	}
}

void debugUtil::printSVFGNodeSetMap(const SVFGNodeSetMap &map, std::string intro_str) {
	outs() << intro_str << ":\n";

	for(auto mapIter = map.begin(); mapIter != map.end(); ++mapIter) {
		NodeID id = mapIter->first;
		const SVFGNodeSet &set = mapIter->second;

		outs() << "\t" << id << ": ";

		for(auto setIter = set.begin(); setIter != set.end(); ++setIter) {
			outs() << (*setIter)->getId() << ", ";
		}

		outs() << "\n";
	}
}

void debugUtil::printStringSet(const StringSet &set, std::string intro_str) {
	llvm::outs() << intro_str << ":\n";
	int i = 0;

	for(auto iter = set.begin(); iter != set.end(); ++iter, i++) {
		llvm::outs() << i << ", " << *iter << "\n";	
	}
}

void debugUtil::printStringList(const StringList &lst, std::string intro_str) {
	llvm::outs() << intro_str << ":\n";
	int i = 0;

	for(auto iter = lst.begin(); iter != lst.end(); ++iter, i++) {
		llvm::outs() << i << ", " << *iter << "\n";	
	}
}

void debugUtil::printSVFG(SVFG *svfg) {
	///Define the GTraits and node iterator for printing
	typedef GraphTraits<SVFG*> GTraits;
	typedef typename GTraits::NodeType NodeType;
	typedef typename GTraits::nodes_iterator node_iterator;
	typedef typename GTraits::ChildIteratorType child_iterator;

	llvm::outs() << "SVFG Graph" << "'...\n";
	// Print each node name and its edges
	node_iterator I = GTraits::nodes_begin(svfg);
	node_iterator E = GTraits::nodes_end(svfg);
	for (; I != E; ++I) {
		NodeType *Node = *I;
		SVFGNode *svfgNode = dyn_cast<SVFGNode>(Node);
		llvm::outs() << "node :" << svfgNode->getId() 
			<< "'\tType= " << getSVFGNodeType(svfgNode) 
			<< "'\tFunction= " << getSVFGFuncName(svfgNode) 
			<< "\tLoc= " << getSVFGSourceLoc(svfgNode) << "\n";
		child_iterator EI = GTraits::child_begin(Node);
		child_iterator EE = GTraits::child_end(Node);
		for (unsigned i = 0; EI != EE && i != 64; ++EI, ++i) {
			SVFGNode *childSvfgNode = dyn_cast<SVFGNode>(*EI);
			llvm::outs() << "child :" << childSvfgNode->getId() 
				<< "'\tType= " << getSVFGNodeType(childSvfgNode) 
				<< "'\tFunction= " << getSVFGFuncName(childSvfgNode) 
				<< "\tLoc= " << getSVFGSourceLoc(svfgNode) << "\n";
		}
		llvm::outs() << "-----------------------------------\n";
	}
}

void debugUtil::printItemSet(const UARItemSet &set, std::string intro_str) {
	outs() << "===================================================\n";
	outs() << intro_str << ":\n";

	for(auto iter = set.begin(); iter != set.end(); ++iter) {
		llvm::outs() << "-----------------------------------\n";
		llvm::outs() << (*iter).toString() << "\n";
	}
	outs() << "===================================================\n";
}

void debugUtil::printItemPath(SVFG *svfg, const CxtTraceDPItem &item, bool forward) {
	const CxtNodeIdList &path = item.getVisitedPath();
	printPath(svfg, path, forward);	
}

void debugUtil::printPath(SVFG *svfg, const CxtNodeIdList &path, bool forward) {
	llvm::outs() << "Path: \n";

	for(auto iter = path.begin(); iter != path.end(); ++iter) {
		NodeID id = iter->second;
		NodeID cxt = iter->first;
		SVFGNode *node = svfg->getSVFGNode(id);
		llvm::outs() << id << "(" << cxt << ")" << ": " << getSVFGNodeType(node) << "\n";
		auto nextIter = std::next(iter, 1);

		if(nextIter != path.end()) {
			NodeID id2 = nextIter->second;
			SVFGNode *node2 = svfg->getSVFGNode(id2);

			if(!forward) {
				SVFGNode *tmp = node;
				node = node2;
				node2 = tmp;
			}

			if(svfg->getSVFGEdge(node, node2, SVFGEdge::DirCall))
				llvm::outs() << "===> " << node2->getBB()->getParent()->getName() 
					<< ": " << getSVFGSourceLoc(node2) << "\n"; 
			else if(svfg->getSVFGEdge(node, node2, SVFGEdge::IndCall))
				llvm::outs() << "---> " << node2->getBB()->getParent()->getName() 
					<< ": " << getSVFGSourceLoc(node2) << "\n"; 
			else if(svfg->getSVFGEdge(node, node2, SVFGEdge::DirRet))
				llvm::outs() << "<===\n"; 
			else if(svfg->getSVFGEdge(node, node2, SVFGEdge::IndRet))
				llvm::outs() << "<---\n"; 
		}
	}
}

void debugUtil::printCxtDeps(SVFG *svfg, int minNumCxts, int maxNumCxts) {
	SVFGStat *stat = svfg->getStat();
    	const NodeIdSetMap &nodesToContexts = stat->getCxtDepNodes();
	outs() << "================================================\n";
	outs() << "Number of contexts that are related with a node: \n";
	outs() << "Node : Num Contexts\n";
	
	for(auto iter = nodesToContexts.begin(); iter != nodesToContexts.end(); ++iter) {
		NodeID nodeId = (*iter).first;
		const SVFGNode *node = svfg->getSVFGNode(nodeId);
		const NodeIdSet &contexts = (*iter).second;
		const llvm::Function *F = node->getBB()->getParent();
		int numCxts = contexts.size();

		if(minNumCxts < numCxts && numCxts < maxNumCxts) {
			outs() << "  " << nodeId << " : " << numCxts << " in Func= " 
				<< F->getName() << "\n";
		}
	}
	outs() << "================================================\n";
}

void printModuleStat(llvm::Module &m) {
	outs() << "================================================\n";
	outs() << "Module Statistic:\n";
	outs() << "\tName= " << m.getName() << "\n";
	outs() << "\tSize= " << m.size() << "\n";	
	outs() << "\tNum Functions= " << m.getFunctionList().size() << "\n";
	outs() << "\tNum GlobalVars= " << m.getGlobalList().size() << "\n";
	outs() << "\tNum Aliases= " << m.getAliasList().size() << "\n";
	outs() << "================================================\n";
}	
