#ifndef KCF_SOLVER_H
#define KCF_SOLVER_H

#include "SVF/Util/WorkList.h"
#include "SVF/MemoryModel/GenericGraph.h"
#include "Util/KDPItem.h"
#include <llvm/ADT/GraphTraits.h>

template<class GraphType, class KDPItem>
class KCFSolver {
public:
	typedef llvm::GraphTraits<GraphType> GTraits;
	typedef typename GTraits::NodeType GNODE;
	typedef typename GTraits::EdgeType GEDGE;
    	typedef llvm::GraphTraits<llvm::Inverse<GNODE *> > InvGTraits;

	KCFSolver() { } 
	virtual ~KCFSolver() { } 

	virtual void forwardTraverse(KDPItem &item) {
		pushIntoWorklist(item);

		while(!isWorklistEmpty()) {
			KDPItem item = popFromWorklist();
			forwardProcess(item);

			GNODE *v = graph->getGNode(item.getCurNodeID());
			auto EI = GTraits::child_begin(v);
			auto EE = GTraits::child_end(v);

			for(; EI != EE; ++EI) {
				forwardpropagate(item, *(EI.getCurrent()));
			}
		}
	}

	virtual void backwardTraverse(KDPItem &item) {
		pushIntoWorklist(item);

		while(!isWorklistEmpty()) {
			KDPItem item = popFromWorklist();
			backwardProcess(item);

			GNODE *v = graph->getGNode(item.getCurNodeID());
			auto EI = InvGTraits::child_begin(v);
			auto EE = InvGTraits::child_end(v);

			for(; EI != EE; ++EI) {
				backwardpropagate(item, *(EI.getCurrent()));
			}
		}
	}

	virtual void forwardProcess(const KDPItem &item) {}
	virtual void backwardProcess(const KDPItem &item) {}

	virtual bool forwardChecks(const KDPItem &item, GEDGE *edge) {
		return true;
	}

	virtual bool backwardChecks(const KDPItem &item, GEDGE *edge) {
		return true;
	}

	virtual void forwardpropagate(const KDPItem &item, GEDGE *edge) {
		KDPItem newItem(item);
		newItem.setCurNodeID(edge->getDstID());	

		if(!forwardChecks(newItem, edge))
			return;

		pushIntoWorklist(newItem);
	}

	virtual void backwardpropagate(const KDPItem &item, GEDGE *edge) {
		KDPItem newItem(item);
		newItem.setCurNodeID(edge->getSrcID());	

		if(!backwardChecks(newItem, edge))
			return;

		pushIntoWorklist(newItem);
	}

	void setGraph(GraphType g) {
		graph = g;
	}

	const GraphType getGraph() const {
		return graph;
	}

	KDPItem popFromWorklist() {
		return worklist.pop();
	}

	bool pushIntoWorklist(KDPItem& item) {
		return worklist.push(item);
	}

	bool isWorklistEmpty() {
		return worklist.empty();
	}

	bool isInWorklist(KDPItem& item) {
		return worklist.find(item);
	}

	void clearWorklist() {
		worklist.clear();
	}

	GNODE* getNode(NodeID id) const {
		return graph->getGNode(id);
	}

private:
	GraphType graph;
	FIFOWorkList<KDPItem> worklist;
};

#endif // KCF_SOLVER_H
