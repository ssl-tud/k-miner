#ifndef KDPITEM_H
#define KDPITEM_H

#include "SVF/Util/DPItem.h"

typedef std::list<NodeID> NodeIdList;

/**
 * CallGraph DPItem.
 */
class CGDPItem : virtual public DPItem {
public:
	CGDPItem(NodeID c) : DPItem(c), depth(0) { }

	CGDPItem(NodeID c, u32_t depth, NodeIdList path) : DPItem(c), depth(depth), path(path) { }

	CGDPItem(const CGDPItem &dps) : CGDPItem(dps.getCurNodeID(), dps.getDepth(), dps.getPath()) { }

	virtual ~CGDPItem() { }

	u32_t getDepth() const {
		return depth;
	}

	bool validDepth() const {
		return depth <= DPItem::maximumBudget;
	}

	void incDepth() {
		depth++;
	}

	void decDepth() {
		depth--;
	}

	NodeIdList getPath() const {
		return path;
	}

	void addToVisited(NodeID id) {
		path.push_back(id);
	}

	inline CGDPItem& operator= (const CGDPItem& rhs) {
		if(*this!=rhs) {
			DPItem::operator=(rhs);
			depth = rhs.depth;
			path = rhs.path;
		}
		return *this;
	}

private:
	// The number of nodes (functions) the item visited.
	u32_t depth;

	// Contains the nodes the item visited in the correct order.
	NodeIdList path;
};


/**
 * Captures the first node the traversion started with.
 */
class RootDPItem : virtual public DPItem {
public:
	RootDPItem(NodeID c, const SVFGNode *root) : DPItem(c), rootNode(root) { }

	RootDPItem(NodeID c) : DPItem(c), rootNode(nullptr) { }

	RootDPItem() : DPItem(0), rootNode(nullptr) { }

	RootDPItem(const RootDPItem &dps) : RootDPItem(dps.getCurNodeID(), dps.getRoot()) { }

	virtual ~RootDPItem() {
	}

	void setRoot(const SVFGNode *root) {
		this->rootNode = root;
	}

	const SVFGNode* getRoot() const {
		return rootNode;
	}

	NodeID getRootID() const {
		if(rootNode)
			return rootNode->getId();
		return 0;
	}	       

	inline RootDPItem& operator= (const RootDPItem& rhs) {
		if(*this!=rhs) {
			DPItem::operator=(rhs);
			rootNode = rhs.rootNode;
		}
		return *this;
	}

protected:
	const SVFGNode *rootNode;
};


typedef std::pair<NodeID, NodeID> CxtNodeIdVar;
typedef std::set<CxtNodeIdVar> CxtNodeIdSet;
typedef std::list<CxtNodeIdVar> CxtNodeIdList;
typedef std::map<CxtNodeIdVar, int> CxtNodeIdOrder;

/**
 * Captures information of the current path in a context sensitive manner. 
 */
class CxtTraceDPItem : public CxtDPItem, public RootDPItem {
public:	
	CxtTraceDPItem(NodeID c, 
		       const ContextCond &cxt, 
		       const SVFGNode *root, 
		       NodeID prevNodeId, 
		       CxtNodeIdSet cxtVisitedNodes, 
		       CxtNodeIdOrder nodesOrder, 
		       bool firstAsgnm, 
		       bool deref) : 
		       DPItem(c), 
		       CxtDPItem(c, cxt), 
		       RootDPItem(c, root), 
		       prevNodeId(prevNodeId), 
		       cxtVisitedNodes(cxtVisitedNodes), 
		       nodesOrder(nodesOrder), 
		       firstAsgnm(firstAsgnm), 
		       deref(deref) { } 

	CxtTraceDPItem(NodeID c, 
		       const ContextCond &cxt, 
		       const SVFGNode *root=nullptr, 
		       NodeID prevNodeId=0,  
		       bool firstAsgnm=false, 
		       bool deref=false) : 
		       DPItem(c), 
		       CxtDPItem(c, cxt), 
		       RootDPItem(c, root), 
		       prevNodeId(prevNodeId), 
		       firstAsgnm(firstAsgnm), 
		       deref(deref) { } 

	CxtTraceDPItem() : DPItem(), 
			   CxtDPItem(), 
			   RootDPItem(), 
			   prevNodeId(0), 
			   firstAsgnm(false), 
			   deref(false) { } 

	CxtTraceDPItem(const CxtTraceDPItem &dps) : CxtTraceDPItem(dps.getCurNodeID(), 
								   dps.getContexts(), 
								   dps.getRoot(), 
								   dps.getPrevNodeID(), 
								   dps.getCxtVisitedNodes(), 
								   dps.getNodesOrder(), 
								   dps.hasFirstAsgnmVisited(), 
								   dps.isDeref()) { }

	virtual ~CxtTraceDPItem() { }

	void addToVisited(NodeID cxt, NodeID id) {
		CxtNodeIdVar cxtVar(cxt,id);
		cxtVisitedNodes.insert(cxtVar);
		nodesOrder[cxtVar] = cxtVisitedNodes.size()-1;
	}

	void addToVisited(const CxtNodeIdSet &set) {
		cxtVisitedNodes.insert(set.begin(), set.end());
	}

	bool hasVisited(NodeID cxt, NodeID id) const {
		CxtNodeIdVar cxtVar(cxt,id);
		return cxtVisitedNodes.find(cxtVar) != cxtVisitedNodes.end();
	}

	bool hasVisited(NodeID id) const {
		// context insensitive check
		auto iter = find_if(cxtVisitedNodes.begin(), cxtVisitedNodes.end(),
				[&](const CxtNodeIdVar& va1) -> bool {
					return va1.second == id;
				});

		return iter != cxtVisitedNodes.end();
	}

	bool hasVisitedCxt(NodeID cxt) const {
		auto iter = find_if(cxtVisitedNodes.begin(), cxtVisitedNodes.end(),
				[&](const CxtNodeIdVar& va1) -> bool {
					return va1.first == cxt;
				});

		return iter != cxtVisitedNodes.end();
	}

	/***
	 * Replace all unkown contexts(=0) with the given one.
	 * Returns false if the node was already visited with this context.
	 */
	bool resolveUnkownCxts(NodeID cxt) {
		for(auto iter = cxtVisitedNodes.begin(); iter != cxtVisitedNodes.end();) {
			CxtNodeIdVar cxtVar = *iter;

			// Because the set is in order and the nodes with context=0 will be at the
			// beginning, we are done as soon as a context is != 0.
			if(cxtVar.first != 0)
				break;

			int pos = nodesOrder[cxtVar];
			nodesOrder.erase(cxtVar);
			cxtVar.first = cxt;
			nodesOrder[cxtVar] = pos;

			if(!cxtVisitedNodes.insert(cxtVar).second)
				return false;

			iter = cxtVisitedNodes.erase(iter);
		}

		return true;
	}

	void clearCxtVisitedSet() {
		cxtVisitedNodes.clear();
	}

	const CxtNodeIdSet& getCxtVisitedNodes() const {
		return cxtVisitedNodes;
	}

	size_t getNumVisitedNodes() const {
		return cxtVisitedNodes.size();
	}

	const CxtNodeIdOrder& getNodesOrder() const {
		return nodesOrder;
	}

	static std::pair<int, CxtNodeIdVar> flip_pair(const std::pair<CxtNodeIdVar, int> &p) {
		return std::pair<int, CxtNodeIdVar>(p.second, p.first);
	}

	CxtNodeIdList getVisitedPath() const {
		std::map<int, CxtNodeIdVar> orderMap;
		CxtNodeIdList path;
		std::transform(nodesOrder.begin(), nodesOrder.end(), std::inserter(orderMap, orderMap.begin()), 
				flip_pair);
		
		for(auto iter = orderMap.begin(); iter != orderMap.end(); ++iter)
			path.push_back(iter->second);

		return path;
	}

	void setPrevNodeID(NodeID prevNodeId) {
		this->prevNodeId = prevNodeId;
	}

	NodeID getPrevNodeID() const {
		return prevNodeId;
	}

	void firstAsgnmVisited() {
		firstAsgnm = true;
	}

	bool hasFirstAsgnmVisited() const {
		return firstAsgnm;
	}

	void setDeref() {
		deref = true;
	}

	void unsetDeref() {
		deref = false;
	}

	bool isDeref() const {
		return deref;
	}

	inline CxtTraceDPItem& operator= (const CxtTraceDPItem& rhs) {
		if(*this!=rhs) {
			CxtDPItem::operator=(rhs);
			RootDPItem::operator=(rhs);
			prevNodeId = rhs.prevNodeId;
			cxtVisitedNodes = rhs.cxtVisitedNodes;
			nodesOrder = rhs.nodesOrder;
			firstAsgnm = rhs.firstAsgnm; 
			deref = rhs.deref; 
		}
		return *this;
	}

protected:
	// Contains the nodes currently visited with the context they were visited in.
	CxtNodeIdSet cxtVisitedNodes;

	// Because the cxtVisitedNodes are stored in a set, nodesOrder maps the actual
	// order of the visited nodes with their contexts.
	CxtNodeIdOrder nodesOrder;

	// The id of the node previousely visited.
	NodeID prevNodeId;

	// Is set during backwarding if the first StoreSVFGNode has been visited and 
	// the node is also in the oosForwardSlice.
	bool firstAsgnm;

	// Is set during backwarding if a LoadSVFGNode is visited. The next AddrSVFGNode
	// that is visited with this item has to switch the PAGSrcNode and PAGDstNode
	// before checking if the PAGDstNodes of both nodes match.
	bool deref;
};

/**
 * Captures the valid scope of the current path. 
 */
class UARDPItem : public CxtTraceDPItem {
public:
	typedef std::set<NodeID> NodeIdSet;
	typedef std::pair<int, int> IntPair;
	typedef std::map<NodeID, IntPair> NodeIdToIntPairMap;

	UARDPItem(NodeID c, 
		  const ContextCond &cxt, 
		  const SVFGNode *root, 
		  NodeID prevNodeId, 
		  CxtNodeIdSet cxtVisitedNodes, 
		  CxtNodeIdOrder nodesOrder, 
		  int scope, 
		  bool firstAsgnm, 
		  bool deref, 
		  bool directRet) :
		  CxtTraceDPItem(c, cxt, root, prevNodeId, cxtVisitedNodes, nodesOrder, firstAsgnm, deref), 
		  DPItem(c), 
		  scope(scope), 
		  directRet(directRet) { } 

	UARDPItem(NodeID c, 
		  const ContextCond &cxt, 
		  const SVFGNode *root) :
		  CxtTraceDPItem(c, cxt, root, 0, false, false), 
		  DPItem(c), 
		  scope(0), 
		  directRet(false) { } 

	UARDPItem() : CxtTraceDPItem(), DPItem(), scope(0), directRet(false) { }

	UARDPItem(const UARDPItem &dps) : UARDPItem(dps.getCurNodeID(), 
						    dps.getContexts(), 
						    dps.getRoot(), 
						    dps.getPrevNodeID(), 
						    dps.getCxtVisitedNodes(), 
						    dps.getNodesOrder(), 
						    dps.getScope(), 
						    dps.hasFirstAsgnmVisited(), 
						    dps.isDeref(), 
						    dps.isDirRet()) { }

	virtual ~UARDPItem() { }

	void incScope() {
		scope++;
	}

	void decScope() {
		scope--;
	}

	int getScope() const {
		return scope;
	}

	bool hasValidScope() const {
		return scope >= 0;
	}

	void setValidScope() {
		scope = 0;
	}

	void setInvalidScope() {
		scope = -1;
	}

	void setDirRet() {
		directRet = true;
	}

	void setIndRet() {
		directRet = false;
	}
	
	bool isDirRet() const {
		return directRet;
	}

	bool mergeVisitedNodes(const CxtNodeIdSet &set) {
		for(auto iter = set.begin(); iter != set.end(); ++iter) {
			auto ret = cxtVisitedNodes.insert(*iter);
			
			if(!ret.second)
				return false;
		}

		return true;
	}

	bool merge(const UARDPItem &item) {
		cur = item.getCurNodeID();
		prevNodeId = item.getPrevNodeID();
		firstAsgnm = item.hasFirstAsgnmVisited();
		deref = item.isDeref();
		directRet = item.isDirRet();

		CxtNodeIdOrder orderMap = item.getNodesOrder();
		for(auto iter = orderMap.begin(); iter != orderMap.end(); ++iter)
			iter->second += cxtVisitedNodes.size();

		// Is only valid if non of the nodes inside the map has the same position.
		nodesOrder.insert(orderMap.begin(), orderMap.end());

		const CxtNodeIdSet &set = item.getCxtVisitedNodes();
		return mergeVisitedNodes(set);
	}

	inline UARDPItem& operator= (const UARDPItem& rhs) {
		if(*this!=rhs) {
			cur = rhs.cur;
			context = rhs.context;
			rootNode = rhs.rootNode;
			prevNodeId = rhs.prevNodeId;
			cxtVisitedNodes = rhs.cxtVisitedNodes;
			nodesOrder = rhs.nodesOrder;
			scope = rhs.scope;
			directRet = rhs.directRet;
			firstAsgnm = rhs.firstAsgnm;
			deref = rhs.deref;
		}
		return *this;
	}

	std::string toString() const {
		std::string str;
		llvm::raw_string_ostream rawstr(str);
		rawstr << "UARDPItem: " << cur << "\n";
		rawstr << "\tRoot=" << rootNode->getId() << "\n";
		rawstr << "\tPrev=" << prevNodeId << "\n";
		rawstr << "\tContext= " << context.toString() << "\n";
		rawstr << "\tScope= " << scope << "\n";
		rawstr << "\thas first Asgnm= " << firstAsgnm << "\n";
		rawstr << "\tis deref= " << deref << "\n";
		rawstr << "\tis dirRet= " << directRet << "\n";

		rawstr << "\tCxt Visited Nodes= [";
		for(auto iter = cxtVisitedNodes.begin(); iter != cxtVisitedNodes.end(); ++iter) {
			rawstr << "(" << iter->first << "," << iter->second << ")" << ", ";
		}	
		rawstr << "]\n";

		return rawstr.str();
	}

	/// Enable compare operator to avoid duplicated item insertion in map or set
	/// to be noted that two vectors can also overload operator()
	inline bool operator< (const UARDPItem& rhs) const {
		if(cur != rhs.cur)
			return cur < rhs.cur;
		else if(getRootID() != rhs.getRootID())
			return getRootID() < rhs.getRootID();
		else if(prevNodeId != prevNodeId)
			return prevNodeId < rhs.prevNodeId;
		else 
			return cxtVisitedNodes < rhs.cxtVisitedNodes;
	}

protected:
	// If we enter a function we increase the scope and otherwise we decrease it.
	int scope;

	// If the last return was direct or indirect. Gets important if an item leaves
	// the valid scope. If it was a direct return we wont found any dangling ptr
	// during backwarding. We have to use the SimplePtrAssignmentAnalysis. 
	bool directRet;
};

/**
 * Captures the function path.
 */
class FuncPathDPItem : public CxtLocDPItem {
public:
	FuncPathDPItem(const CxtVar &var, 
		       const LocCond* locCond, 
		       const ContextCond &funcPath): CxtLocDPItem(var, locCond), funcPath(funcPath) { }

	FuncPathDPItem(const FuncPathDPItem &dps) : FuncPathDPItem(dps.getCondVar(), 
			             		    dps.getLoc(), 
						    dps.getFuncPath()) { }

	virtual ~FuncPathDPItem() { }

	const ContextCond& getFuncPath() const {
		return funcPath;
	}

	void addFuncEntry(NodeID id) {
		funcPath.pushContext(id);
	}

	void addFuncReturn(NodeID id) {
		funcPath.pushContext(id);
	}

	inline FuncPathDPItem& operator= (const FuncPathDPItem& rhs) {
		if(*this!=rhs) {
			CxtLocDPItem::operator=(rhs);
			funcPath = rhs.funcPath;
		}
		return *this;
	}

	virtual std::string toString() const {
		std::string str;
		llvm::raw_string_ostream rawstr(str);
		rawstr << "FuncPathDPItem: " << cur << "\n";
		rawstr << "\tfuncPath= " << funcPath.toString() << "\n";

		return rawstr.str();
	}

protected:
	ContextCond funcPath;
};


/**
 * A light weight version of the UARDPItem.
 * Only considers function nodes.
 */
class UARLiteDPItem : public FuncPathDPItem {
public:
	UARLiteDPItem(const CxtVar &var, 
		      const LocCond* locCond, 
		      const ContextCond &funcPath, 
		      NodeID prevNodeId=0, 
		      bool firstAsgnm=false, 
		      bool deref=false) : 
		      FuncPathDPItem(var, locCond, funcPath), prevNodeId(prevNodeId), 
		      firstAsgnm(firstAsgnm), 
		      deref(deref) { } 

	UARLiteDPItem(const UARLiteDPItem &dps) : UARLiteDPItem(dps.getCondVar(), 
								dps.getLoc(), 
								dps.getFuncPath(), 
								dps.getPrevNodeID(), 
								dps.hasFirstAsgnmVisited(), 
								dps.isDeref()) { }

	virtual ~UARLiteDPItem() { }


	void setPrevNodeID(NodeID prevNodeId) {
		this->prevNodeId = prevNodeId;
	}

	NodeID getPrevNodeID() const {
		return prevNodeId;
	}

	void firstAsgnmVisited() {
		firstAsgnm = true;
	}

	bool hasFirstAsgnmVisited() const {
		return firstAsgnm;
	}

	void setDeref() {
		deref = true;
	}

	void unsetDeref() {
		deref = false;
	}

	bool isDeref() const {
		return deref;
	}

	inline UARLiteDPItem& operator= (const UARLiteDPItem& rhs) {
		if(*this!=rhs) {
			FuncPathDPItem::operator=(rhs);
			prevNodeId = rhs.prevNodeId;
			firstAsgnm = rhs.firstAsgnm; 
			deref = rhs.deref; 
		}
		return *this;
	}

	virtual std::string toString() const {
		std::string str;
		llvm::raw_string_ostream rawstr(str);
		rawstr << "UARDPItem: " << cur << "\n";
		rawstr << "\tfuncPath= " << funcPath.toString() << "\n";
		rawstr << "\tPrev=" << prevNodeId << "\n";
		rawstr << "\thas first Asgnm= " << firstAsgnm << "\n";
		rawstr << "\tis deref= " << deref << "\n";

		return rawstr.str();
	}

protected:
	NodeID prevNodeId;

	// Is set during backwarding if the first StoreSVFGNode has been visited and 
	// the node is also in the oosForwardSlice.
	bool firstAsgnm;

	// Is set during backwarding if a LoadSVFGNode is visited. The next AddrSVFGNode
	// that is visited with this item has to switch the PAGSrcNode and PAGDstNode
	// before checking if the PAGDstNodes of both nodes match.
	bool deref;
};

/***
 * Item used by the Double-Free Checker.
 */
class KSrcSnkDPItem : public PathDPItem  {
public:
	KSrcSnkDPItem(const VFPathVar& var, const LocCond* locCond) : PathDPItem(var, locCond) {
	}

	KSrcSnkDPItem(const KSrcSnkDPItem& dps) : PathDPItem(dps), fieldCxt(dps.getFieldCxt()) {
	}

	virtual ~KSrcSnkDPItem() {
	}

	unsigned int getHammingDistance(const KSrcSnkDPItem& rhs) const {
		unsigned int hd = 0;
		const auto &path1 = getCond().getVFEdges();
		const auto &path2 = rhs.getCond().getVFEdges();
		int i=path1.size()-1, j=path2.size()-1;

		for(; i > 0 && j > 0; i--, j--) {
			if(path1[i].first != path2[j].first) {
				hd++;
			}
		}

		hd += i + j;

		return hd;
	}

	const CallStrCxt& getFieldCxt() const {
		return fieldCxt;
	}

	CallStrCxt& getFieldCxt() {
		return fieldCxt;
	}

	void setFieldCxt(const CallStrCxt &cxt) {
		fieldCxt = cxt;
	}

	unsigned int fieldCxtSize() const {
		return fieldCxt.size();
	}

	bool pushFieldContext(size_t offset) {
		if(fieldCxt.size() < 5) {
			fieldCxt.push_back(offset);
			return true;
		} else { 
			return false;
		}
	}

	static inline void setPathSensitive() {
		pathSensitive = true;
	}

	static inline void setCxtSensitive() {
		pathSensitive = false;
	}

	/**
	 * Enable compare operator to avoid duplicated item insertion in map or set 
	 * to be noted that two vectors can also overload operator()
	 */
	inline bool operator< (const KSrcSnkDPItem& rhs) const {
		if(pathSensitive)
			return PathDPItem::operator<(rhs);

		if (cur != rhs.cur)
			return cur < rhs.cur;
		else if(curloc != rhs.getLoc())
			return curloc < rhs.getLoc();
		else
			return getCond().getContexts() < rhs.getCond().getContexts();
	}

	/**
	 * Overloading operator=
	 */
	inline KSrcSnkDPItem& operator= (const KSrcSnkDPItem& rhs) {
		if(*this!=rhs) {
			PathDPItem::operator=(rhs);
			fieldCxt = rhs.getFieldCxt();
		}
		return *this;
	}

	/*
	 * Overloading operator==
	 */
	inline bool operator== (const KSrcSnkDPItem& rhs) const {
		if(pathSensitive)
			return PathDPItem::operator==(rhs);

//		return (cur == rhs.cur && curloc == rhs.getLoc() && vfpath==rhs.getCond());
		return (cur == rhs.cur && curloc == rhs.getLoc() && getCond().getContexts() == rhs.getCond().getContexts());
	}

	/* 
	 * Overloading operator!=
	 */
	inline bool operator!= (const KSrcSnkDPItem& rhs) const {
		return !(*this==rhs);
	}

private:
	static bool pathSensitive;

	CallStrCxt fieldCxt;
};

#endif // KDPITEM_H
