#ifndef SVFG_ANALYSIS_UTIL_H
#define SVFG_ANALYSIS_UTIL_H

#include "SVF/MemoryModel/PAG.h"
#include "SVF/MSSA/SVFG.h"
#include "SVF/Util/BasicTypes.h"
#include "SVF/Util/BasicTypes.h"
#include "Util/KDPItem.h"
#include "Util/KernelAnalysisUtil.h"

namespace svfgAnalysisUtil {

typedef std::pair<const SVFGNode*, const SVFGNode> SVFGNodepair;
typedef std::pair<NodeID, NodeID> CxtNodeIdVar;
typedef std::list<CxtNodeIdVar> CxtNodeIdList;
typedef std::vector<std::pair<NodeID,NodeID> > EdgeVec;

/**
 * Checks if the node assigns NULL.
 */
bool isNullStoreNode(const SVFGNode *node);

/**
 * Checks if the given node is a storeNode of 
 * a parameter node.
 */
bool isParamStore(const SVFGNode *node);

/**
 * Checks if a given node represents the addrNode
 * of a parameter.
 */
bool isParamAddr(const SVFGNode *node);

/**
 * Checks if a given pag node belongs to a svfgNode that 
 * represents the addrNode of a parameter.
 */
bool isParamAddr(SVFG *svfg, const PAGNode *node);

/**
 * Checks if a given node represents an argument that has been added as
 * local value.
 */
bool isArg(const SVFGNode *node);

/**
 * Counts the number of nodes with the given type.
 */
template<typename T=StmtSVFGNode>
unsigned int getNumNodes(const SVFG *svfg, const VFPathCond::EdgeSet &path) {
	unsigned int i = 0;

	for(const auto &iter : path) {
		NodeID id = iter.first;
		const SVFGNode *node= svfg->getSVFGNode(id);

		if(llvm::isa<T>(node))
			i++;
	}

	return i;
}

/**
 * Get the definition node of a given node.
 */
const SVFGNode* getSVFGNodeDef(SVFG *svfg, const SVFGNode* node);

/**
 * Get the type of a SVFGNode as a string.
 */
std::string getSVFGNodeType(const SVFGNode *node);

/**
 * Get the name of the value represented by the given node.
 */
std::string getSVFGValueName(const SVFGNode *node);

/**
 * Get the Location of a SVFGNode.
 */
std::string getSVFGSourceLoc(const SVFGNode *node);

/**
 * Get the line number of the instruction represented by the SVFGNode.
 */
u32_t getSVFGSourceLine(const SVFGNode *node);

/**
 * Get the file name of the instruction represented by the SVFGNode.
 */
std::string getSVFGSourceFileName(const SVFGNode *node);

/**
 * Get the function of the instruction represented by the SVFGNode.
 */
const llvm::Function* getSVFGFunction(const SVFGNode *node);

/**
 * Get the function name of the instruction represented by the SVFGNode.
 */
std::string getSVFGFuncName(const SVFGNode* node);

/**
 * Converts the given path into a string.
 */
std::string getFuncPathStr(SVFG *svfg, const CxtNodeIdList &path, bool forward);
std::string getFuncPathStr(SVFG *svfg, const ContextCond &funcPath, bool forward);
std::string getFuncPathStr(SVFG *svfg, const EdgeVec &funcPath, bool forward);
};

#endif // SVFG_ANALYSIS_UTIL_H
