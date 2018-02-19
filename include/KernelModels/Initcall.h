#ifndef INITCALL_H
#define INITCALL_H

#include "Util/BasicTypes.h"
#include "Util/DebugUtil.h"
#include "KernelModels/KernelContextObj.h"
#include <sstream>
#include <iostream>
#include <iomanip>

using namespace llvm;

class Initcall;

typedef std::map<std::string, Initcall> InitcallMap;
typedef std::set<std::string> InitcallNameSet;
typedef std::list<Initcall> InitcallList;

/***
 * Contains the used functions, global variables and non
 * defined variables of a initcall.
 */
class Initcall: public KernelContextObj {
public:
	Initcall(): KernelContextObj(CONTEXT_TYPE::INITCALL) { }

	Initcall(std::string name, uint32_t level): 
		KernelContextObj(CONTEXT_TYPE::INITCALL, name), level(level) { }

	Initcall(std::string name, 
		uint32_t level,
		const StringSet &functions, 
		const StringSet &globalvars, 
		const StringSet &nondefvars,
		uint32_t maxCGDepth): 
		KernelContextObj(CONTEXT_TYPE::INITCALL, name),  
		level(level), 
		nondefvars(nondefvars) { 
			addToContextRoot(name);	
			setFunctions(functions);
			setGlobalVars(globalvars);
			setMaxCGDepth(maxCGDepth);
		}

	Initcall(const Initcall &initcall) : 
		Initcall(initcall.name,
		initcall.level,
		initcall.functions, 
		initcall.globalvars, 
		initcall.nondefvars,
		initcall.maxCGDepth) { }

	virtual ~Initcall() { }

	/**
	 * Get the level of the initcall.
	 */
	uint32_t getLevel() const {
		return level;
	}

	/**
	 * Get the used non defined global variables.
	 */
	StringSet& getNonDefVars() {
		return nondefvars;
	}

	/**
	 * Get the used non defined global variables.
	 */
	const StringSet& getNonDefVars() const {
		return nondefvars;
	}

	/**
	 * Set the used non defined global variables.
	 */
	void setNonDefVars(const StringSet &set) {
		nondefvars = set;
	}

	/**
	 * Intersects the given non defined global variables 
	 * with the stored one.
	 */
	void intersectNonDefVars(const StringSet &set) {
		StringSet interSet;
		set_intersection(nondefvars.cbegin(), nondefvars.cend(), 
				 set.begin(), set.end(), 
				 std::inserter(interSet, interSet.begin()));

		nondefvars = interSet;
	}

	/**
	 * Initersects the whole initcall with the given one.
	 */
	virtual void intersect(const Initcall &initcall) {
		intersectFunctions(initcall.getFunctions());
		intersectGlobalVars(initcall.getGlobalVars());
		intersectNonDefVars(initcall.getNonDefVars());
		maxCGDepth = maxCGDepth < initcall.getMaxCGDepth() ? maxCGDepth : initcall.getMaxCGDepth();
	}

	inline Initcall& operator= (const Initcall& rhs) {
		if(*this!=rhs) {
			name = rhs.name;
			level = rhs.level;
			functions = rhs.functions;
			globalvars = rhs.globalvars;
			nondefvars = rhs.nondefvars;
			maxCGDepth = rhs.maxCGDepth;
		}
		return *this;
	}

	inline bool operator== (const Initcall& rhs) const {
		return (name == rhs.name && 
			functions == rhs.functions && 
			globalvars == rhs.globalvars &&
			nondefvars == rhs.nondefvars &&
			maxCGDepth == rhs.maxCGDepth);
	}

	inline bool operator!= (const Initcall& rhs) const {
		return !(*this == rhs);
	}

	static inline bool classof(const Initcall *initcall) {
		return true;
	}

	static inline bool classof(const KernelContextObj *cxtObj) {
		return cxtObj->getContextType() == CONTEXT_TYPE::INITCALL;
	}

	virtual void dump() const {
		outs() << "==================\n";
		outs() << "Initcall " << name << ":\n";
		outs() << "Level " << level << ":\n";
		outs() << "Num functions= " << functions.size() << "\n";
		debugUtil::printStringSet(functions, "Functions");
		outs() << "Num globalvars= " << globalvars.size() << "\n";
		outs() << "Num nondefvars= " << nondefvars.size() << "\n";
		debugUtil::printStringSet(nondefvars, "NonDefVars");
		outs() << "MaxCGDepth= " << maxCGDepth << "\n";
		outs() << "------------------\n";
	}

	friend std::istream& operator>> (std::istream& is, Initcall &bug);
	friend std::ostream& operator<< (std::ostream& os, const Initcall &bug);
	
protected:

	// Level of the initcall. (0-7)
	uint32_t level;

	// Relevant non defined global variables of the initcall.
	// (These global vairbales are also in the other set.)
	StringSet nondefvars;
};

#endif // INITCALL_H
