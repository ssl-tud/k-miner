#ifndef DEBUG_UTIL_H
#define DEBUG_UTIL_H

#include "SVF/Util/BasicTypes.h"
#include "Util/KernelAnalysisUtil.h"
#include "Util/KDPItem.h"
#include "SVF/MSSA/SVFG.h"
#include "SVF/MSSA/SVFGStat.h"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>	
#include <llvm/IR/CallSite.h>
#include <llvm/IR/Dominators.h>	

namespace debugUtil {

typedef std::set<NodeID> NodeIdSet;
typedef std::list<NodeID> NodeIdList;
typedef std::set<std::string> StringSet;
typedef std::list<std::string> StringList;
typedef std::map<NodeID, NodeIdSet> NodeIdSetMap;
typedef std::set<const SVFGNode*> SVFGNodeSet;
typedef std::map<NodeID, SVFGNodeSet> SVFGNodeSetMap;
typedef std::map<NodeID, const SVFGNode*> SVFGNodeMap;
typedef std::set<UARDPItem> UARItemSet;

std::string getPAGNodeType(const PAGNode *pagNode);

void printNodeIdSet(SVFG *svfg, const NodeIdSet &set, std::string intro_str);

void printNodeIdList(SVFG *svfg, const NodeIdList &lst, std::string intro_str);

void printCxtNodeIdSet(SVFG *svfg, const CxtNodeIdSet &set, std::string intro_str);

void printCxtNodeIdSetSimple(SVFG *svfg, const CxtNodeIdSet &set, std::string intro_str);

void printSVFGNode(const SVFGNode *node);

void printSVFGNodeSet(const SVFGNodeSet &set, std::string intro_str);

void printSVFGNodeMap(const SVFGNodeMap &map, std::string intro_str); 

void printSVFGNodeSetMap(const SVFGNodeSetMap &map, std::string intro_str);

void printStringSet(const StringSet &set, std::string intro_str);

void printStringList(const StringList &lst, std::string intro_str);

void printSVFG(SVFG *svfg);

void printItemSet(const UARItemSet &set, std::string intro_str);

void printItemPath(SVFG *svfg, const CxtTraceDPItem &item, bool forward=true);

void printPath(SVFG *svfg, const CxtNodeIdList &path, bool forward=true);

void printCxtDeps(SVFG *svfg, int minNumCxts, int maxNumCxts);
};

#endif // DEBUG_UTIL_H
