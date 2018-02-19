#ifndef SPAG_H
#define SPAG_H

#include "MemoryModel/PAG.h"
#include "SABER/CFLSolver.h"
#include "Util/DPItem.h"
#include "MSSA/SVFG.h"

typedef std::set<const SVFGNode*> SVFGNodeSet;
typedef std::set<const PAGNode*> PAGNodeSet;
typedef std::set<NodeID> NodeIdSet;
typedef std::map<NodeID, NodeIdSet> IdToSetMap;
typedef std::set<std::pair<NodeID, NodeID> > NodeToPairSet;
typedef std::map<NodeID, NodeToPairSet> NodeToPairSetMap;	

/*
 * Finds identical GEPs by forwarding through the SVFG.
 */
class GEPDepSolver : public CFLSolver<SVFG*, RelateCxtDPItem> {
public:
	GEPDepSolver(SVFG *svfg): svfg(svfg) {
		pag = PAG::getPAG();
		setGraph(svfg);	
	}

	virtual ~GEPDepSolver() { }

	void solve(IdToSetMap &objPNToGep, IdToSetMap &gepRelations);

protected:
	virtual void forwardTraverse(RelateCxtDPItem& it); 
	virtual void forwardProcess(const RelateCxtDPItem& item);
	virtual void forwardpropagate(const RelateCxtDPItem& item, SVFGEdge* edge); 

private:
	// Transforms the pagNodeIds of objPNGep into the corresponding svfgNodeIds.
	inline void setRelatedSvfgGEPs(IdToSetMap &objPNToGep);

	inline bool forwardVisited(const SVFGNode* node) {
		return visitedSet.find(node)!=visitedSet.end();
	}
	inline void addForwardVisited(const SVFGNode* node) {
		visitedSet.insert(node);
	}
	inline void clearVisitedMap() {
		visitedSet.clear();
	}

	// Contains all GEP-NodeIds by their svfgNodeIds.
	NodeIdSet svfgGepIds;

	// Maps the svfgNodeId of a GEP-Node to all identical GEP-Nodes
	IdToSetMap svfgSrcToGEP;

	// contains all visited SVFGNodes
	SVFGNodeSet visitedSet;	

	PAG *pag;
	SVFG *svfg;
};


class SPAG : public CFLSolver<PAG*, RelateCxtDPItem> {
public:
	enum Process {
		p0, p1, p2,
	};

	SPAG(SVFG *svfg): svfg(svfg), atStack(false), curProc(Process::p0) { 
		gepDepSolver = new GEPDepSolver(svfg);
		pag = PAG::getPAG();
		setGraph(pag);	
	}

	virtual ~SPAG() { 
		if(gepDepSolver != nullptr)
			delete gepDepSolver;
		gepDepSolver = nullptr;	
	}

	void build(); 	

	PAGNode* getPAGNode(NodeID id) const {
		return pag->getPAGNode(id);
	}

	const NodeToPairSetMap& getGlobalAssignments() const {
		return globalAssignments;
	}

	NodeToPairSetMap::iterator getPtsToGlobalBegin() {
		return globalAssignments.begin();
	}

	NodeToPairSetMap::iterator getPtsToGlobalEnd() {
		return globalAssignments.end();
	}

	NodeToPairSetMap::const_iterator getPtsToGlobalBegin() const {
		return globalAssignments.begin();
	}

	NodeToPairSetMap::const_iterator getPtsToGlobalEnd() const {
		return globalAssignments.end();
	}

	bool isGlobalNode(NodeID id) const;
	bool isStackNode(NodeID id) const;

	void print() const;

protected:
	virtual void forwardTraverse(RelateCxtDPItem& it); 
    	virtual void forwardpropagate(const RelateCxtDPItem& item, GEDGE* edge); 
	virtual void forwardProcess(const RelateCxtDPItem& item);

private:

	// Divides the PAG-Nodes into Global-, Heap- and Stack-Nodes.
	void splitNodesByMemObj();

	// Finds and adds the sturct-pointer elements to stack and global nodes.
	// They are also stored in the objPNtoGep SetMap to have a relation to the ObjPN. 
	void findGEPs(); 

	// Stores for a given NodeId all possible assigned nodes.
	void findAssignments();

	// Intersects the assignments of all nodes in dependence of the type (stack, heap, global). 
	void buildIntersections();

	// Intersects the assignments of two node-type maps.
	void intersectAssignments(const IdToSetMap &loadNodes);

	// Combines the assignments of two node-type sets.
	void combineAssignments(NodeID loadId, NodeIdSet &loadIdSet, NodeIdSet &storeIdSet, NodeToPairSet &pSet);

	// If nodeId is a GEP, add all related nodes to the set.
	void handleEquivalentGeps(NodeIdSet &set, NodeID nodeId); 
	
	// Collects all store and load nodes (assignments).
	void process1(const RelateCxtDPItem& item);

	// Collects the GEP nodes and the corresponding src node.
	void process2(const RelateCxtDPItem& item);

	// Checks if a node represents a pointer.
	bool isPointer(const PAGNode* node); 

	// Checks if the dst-node of the given edge is written.
	bool edgeStores(const PAGEdge* edge); 

	//Checks if the src-node of the edge is written to the dst-node. 
	bool edgeLoads(const PAGEdge* edge); 

	inline bool forwardVisited(const PAGNode* node) {
		return visitedSet.find(node)!=visitedSet.end();
	}

	inline void addForwardVisited(const PAGNode* node) {
		visitedSet.insert(node);
	}

	inline void clearVisitedMap() {
		visitedSet.clear();
	}

	void printSetMap(const IdToSetMap &nodes) const;
	void printPairSetMap(const NodeToPairSetMap &ptsToMap) const;

	PAG *pag;
	SVFG *svfg;

	GEPDepSolver *gepDepSolver;	

	// Determines the process that will be executed during forwarding.
	Process curProc;

	// contains all visited SVFGNodes
	PAGNodeSet visitedSet;	
	
	// Flag that indicates that a forwarding process is done for a stack-elment.
	bool atStack;

	// Contains all relevant nodes by its id.
	NodeIdSet globalNodeIds;
	NodeIdSet stackNodeIds;

	// Each node contains a set of nodes where data is possible written to a variable. 
	IdToSetMap globalStoreNodes;
	IdToSetMap stackStoreNodes; // dummy

	// Each node contains a set of nodes where data is possible load to a variable.
	IdToSetMap globalLoadNodes;
	IdToSetMap stackLoadNodes;

	// Maps the pagNodeId of the Src to the ElementPtrInst 
	IdToSetMap objPNToGep; 

	// Contains for each GEP-Node the equivalent GEP-Nodes
	IdToSetMap gepRelations;

	// Each node contains a set of pairs which consists of the node that writes into 
	// it a at which point in the path.
	NodeToPairSetMap globalAssignments;
//	NodeToPairSetMap ptsToStackNodes;
};

#endif // SPAG_H
