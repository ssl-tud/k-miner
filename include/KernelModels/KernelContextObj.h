#ifndef KERNEL_CONTEXT_OBJ_H
#define KERNEL_CONTEXT_OBJ_H

#include "Util/BasicTypes.h"
#include <algorithm> 

class KernelContextObj;

typedef std::map<std::string, KernelContextObj> KernelContextObjMap;

/***
 * Contains the used functions, global variables of an abstract kernel context object (e.g. system call, driver).
 */
class KernelContextObj {
public:
	enum CONTEXT_TYPE {
		SYSCALL,
		DRIVER,	
		INITCALL,
		UNKNOWN
	};

	const std::string ContextTypeNames[4] = {"SYSCALL", "DRIVER", "INITCALL", "UNKNOWN"};

	KernelContextObj(CONTEXT_TYPE type): type(type), name(""), maxCGDepth(0) { }

	KernelContextObj(CONTEXT_TYPE type, std::string name): type(type), name(name), maxCGDepth(0) { }

	KernelContextObj(CONTEXT_TYPE type,
		std::string name, 
		const StringSet &cxtRoot,
		const StringSet &functions, 
		const StringSet &globalvars,
		uint32_t maxCGDepth):
		type(type),
		name(name), 
		cxtRoot(cxtRoot), 
		functions(functions), 
		globalvars(globalvars),
	       	maxCGDepth(maxCGDepth) { }

	KernelContextObj(const KernelContextObj &kernelCxtObj): 
		KernelContextObj(kernelCxtObj.type,
			   kernelCxtObj.name, 
			   kernelCxtObj.cxtRoot, 
			   kernelCxtObj.functions, 
			   kernelCxtObj.globalvars,
			   kernelCxtObj.maxCGDepth) { }

	virtual ~KernelContextObj() { }
		
	/**
	 * Get the contexts root name.
	 */
	std::string getName() const {
		return name;
	}
	
	/**
	 * Get the relevant ContextRoot-functions.
	 */
	StringSet& getContextRoot() {
		return cxtRoot;
	}

	/**
	 * Get the relevant ContextRoot-functions.
	 */
	const StringSet& getContextRoot() const {
		return cxtRoot;
	}

	void addToContextRoot(std::string funcName) {
		cxtRoot.insert(funcName);
	}

	/**
	 * Set the relevant ContextRoot-functions.
	 */
	void setContextRoot(const StringSet &set) {
		cxtRoot = set;
	}


	/**
	 * Returns true if the given function is part of the context Root.
	 */
	bool inContextRoot(std::string name) const {
		return cxtRoot.find(name) != cxtRoot.end();
	}


	/**
	 * Get the functions.
	 */
	StringSet& getFunctions() {
		return functions;
	}

	/**
	 * Get the functions.
	 */
	const StringSet& getFunctions() const {
		return functions;
	}

	/**
	 * Set the functions.
	 */
	void setFunctions(const StringSet &set) {
		functions = set;
	}

	/**
	 * Checks if the system call has a certain functions.
	 */
	bool hasFunction(std::string name) const {
		return functions.find(name) != functions.end();
	}

	/**
	 * Add relevant functions.
	 */
	void addFunctions(const StringSet &set) {
		functions.insert(set.begin(), set.end());
	}

	/**
	 * Intersects the given functions with the 
	 * stored one.
	 */
	void intersectFunctions(const StringSet &set) {
		StringSet interSet;
		set_intersection(functions.cbegin(), functions.cend(), 
				 set.begin(), set.end(), 
				 std::inserter(interSet, interSet.begin()));

		functions = interSet;
	}

	/**
	 * Get the used global variables.
	 */
	StringSet& getGlobalVars() {
		return globalvars;
	}

	/**
	 * Get the used global variables.
	 */
	const StringSet& getGlobalVars() const {
		return globalvars;
	}

	/**
	 * Set the used global variables.
	 */
	void setGlobalVars(const StringSet &set) {
		globalvars = set;
	}

	/**
	 * Add relevant global variable.
	 */
	void addGlobalVars(const StringSet &set) {
		globalvars.insert(set.begin(), set.end());
	}

	/**
	 * Checks if the system call has a certain global variable.
	 */
	bool hasGlobalVar(std::string name) const {
		return globalvars.find(name) != globalvars.end();
	}

	/**
	 * Intersects the given global variables with the 
	 * stored one.
	 */
	void intersectGlobalVars(const StringSet &set) {
		StringSet interSet;
		set_intersection(globalvars.cbegin(), globalvars.cend(), 
				 set.begin(), set.end(), 
				 std::inserter(interSet, interSet.begin()));

		globalvars = interSet;
	}

	/**
	 * Get the maximal depth of the callgraph of the initcall.
	 */
	uint32_t getMaxCGDepth() const {
		return maxCGDepth;
	}

	/**
	 * Set the maximal depth of the callgraph of the initcall.
	 */
	void setMaxCGDepth(uint32_t depth) {
		maxCGDepth = depth;
	}

	/**
	 * Initersects the whole kernelCxtObj with the given one.
	 */
	virtual void intersect(const KernelContextObj &kernelCxtObj) {
		intersectFunctions(kernelCxtObj.getFunctions());
		intersectGlobalVars(kernelCxtObj.getGlobalVars());
		maxCGDepth = maxCGDepth < kernelCxtObj.getMaxCGDepth() ? maxCGDepth : kernelCxtObj.getMaxCGDepth();
	}

	CONTEXT_TYPE getContextType() const {
		return type;
	}

	std::string getContextTypeName() const {
		return ContextTypeNames[type];
	}

	inline bool operator< (const KernelContextObj& rhs) const {
		return name < rhs.name;
	}

	inline KernelContextObj& operator= (const KernelContextObj& rhs) {
		if(*this!=rhs) {
			name = rhs.name;
			cxtRoot = rhs.cxtRoot;
			functions = rhs.functions;
			globalvars = rhs.globalvars;
			maxCGDepth = rhs.maxCGDepth;
		}
		return *this;
	}

	inline bool operator== (const KernelContextObj& rhs) const {
		return (name == rhs.name && 
			cxtRoot == rhs.cxtRoot && 
			functions == rhs.functions && 
			globalvars == rhs.globalvars &&
			maxCGDepth == maxCGDepth);
	}

	inline bool operator!= (const KernelContextObj& rhs) const {
		return !(*this == rhs);
	}

	virtual void dump() const {
		llvm::outs() << "==================\n";
		llvm::outs() << "KernelContextObj " << name << ":\n";
		llvm::outs() << "Num functions= " << functions.size() << "\n";
		llvm::outs() << "Num globalvars= " << globalvars.size() << "\n";
		llvm::outs() << "MaxCGDepth= " << maxCGDepth << "\n";
		llvm::outs() << "------------------\n";
	}

protected:
	std::string name;
	StringSet cxtRoot;
	StringSet functions;
	StringSet globalvars;

	uint32_t maxCGDepth;
private:
	CONTEXT_TYPE type;	
};

#endif // KERNEL_CONTEXT_OBJ_H
