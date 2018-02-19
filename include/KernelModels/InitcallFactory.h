#ifndef INITCALL_FACTORY_H
#define INITCALL_FACTORY_H

#include "KernelModels/Initcall.h"
#include "SVF/Util/BasicTypes.h"
#include "Util/DebugUtil.h"
#include "SVF/WPA/Andersen.h"
#include "SVF/Util/PTACallGraph.h"
#include "SVF/MemoryModel/ConsG.h"
#include <llvm/Transforms/Utils/Cloning.h>	
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <algorithm>
#include <ctype.h>

typedef std::set<std::string> StringSet;
typedef std::map<std::string, StringSet> StrToStrSet;

#define toDigit(c) (c-'0')


/**
 * Groups the most similar initcalls.
 */
struct InitcallGroup {
	uint32_t ID;
	uint32_t level;
	StringSet initcalls;
	StringSet functions;
	StringSet globalvars;
	StringSet nondefvars;

	void dump() {
		outs() << "==================\n";
		outs() << "InitcallGroup" << ID << " (" << level << "):\n";
		outs() << "------------------\n";
		outs() << "Num initcalls= " << initcalls.size() << "\n";
		outs() << "Num functions= " << functions.size() << "\n";
		outs() << "Num globalvars= " << globalvars.size() << "\n";
		outs() << "Num nondefvars= " << nondefvars.size() << "\n";
		outs() << "------------------\n";
		debugUtil::printStringSet(initcalls, "Initcalls");
	}

	static const uint32_t funcLimit = 20000;
};

typedef std::list<InitcallGroup> InitcallGroupList;

/***
 * Find all the initcalls and determines the relevant functions
 * as well as the relevant global variables of the initcalls.
 */
class InitcallFactory {		
private:
	static InitcallFactory *initcallFactory;

	/** 
	 * Finds all the initcalls inside the given module. 
	 */
	void findInitcalls();

	/**
	 * Parses the initcall name and returns the corresponding level of the initcall.
	 *
	 * Level| Initcall Type
	 * -----+--------------
	 *  0	| early, 0
	 *  1	| 1, 1s 
	 *  2	| 2, 2s 
	 *  3	| 3, 3s 
	 *  4	| 4, 4s 
	 *  5	| 5, 5s, rootfs 
	 *  6	| 6, 6s 
	 *  7	| 7, 7s 
	 *  8	| other 
	 */
	uint32_t getLevelOfName(std::string initcallName) const {
		// Because of the SSA from of some functions we need to remove the ids.
		if(initcallName.find(".") != std::string::npos) {
			std::stringstream ss(initcallName);
			std::getline(ss, initcallName, '.');
		}

		char lastChar = initcallName.back();
		char preultChar = initcallName[initcallName.size()-2];

		if(initcallName.find("rootfs") != std::string::npos)
			return 5;

		if(initcallName.find("early") != std::string::npos)
			return 0;

		if(lastChar == 's') 
			return toDigit(preultChar)%10;
		
		if(std::isdigit(lastChar))
			return toDigit(lastChar)%10;

		return 8;
	}

	/**
	 * Collects the initcall names in groups with a given limit of functions and globalvars.
	 */
	InitcallGroupList groupInitcallsByLimit(const InitcallMap &initcalls);

	/**
	 * Collects the initcall names in groups with a given level.
	 */
	void groupInitcallsByLevel(const InitcallMap &initcalls);

	/**
	 * Get the next group that matches the given leven and the function limit of the group.
	 */
	InitcallGroup& getNextFreeInitcallGroup(uint32_t level, const StringSet &newFunctions) {
		uint32_t newID = initcallGroups.size();

		for(auto &iter : initcallGroups) {
			InitcallGroup &group = iter;
			StringSet interSet;

			set_intersection(group.functions.cbegin(), group.functions.cend(), newFunctions.begin(), 
					newFunctions.end(), std::inserter(interSet, interSet.begin()));

			if(group.level == level) {
				if((group.functions.size()+(newFunctions.size()-interSet.size())) 
				   <= InitcallGroup::funcLimit)
					return group;
			}
		}

		InitcallGroup newGroup;
		newGroup.ID = newID;
		newGroup.level = level;
		initcallGroups.push_back(newGroup);
		return initcallGroups.back();
	}

	/**
	 * Clones the module and reduces it to only the relevant functions of the initcallGroup.
	 * Additionally an andersen pointer analysis creates a precise ptacallgraph and a constraint graph.
	 * These graphs are used to find the correct relevant functions of an initcall as well as the function
	 * that are used to initialize a non defined gglobal variable.
	 */
	void handleInitcallGroup(InitcallGroup &group);

	/**
	 * Merges the relevant function of a non defined global variable, with the one that
	 * were already found by previous groups.
	 */
	void mergeNonDefVarFunctions(const StrToStrSet &groupNonDefVarFuncs, 
				     const StrToStrSet &groupNonDefVarGVs) {
		for(auto iter : groupNonDefVarFuncs) {
			std::string nondefvar = iter.first;
			const StringSet &functions = iter.second;
			nonDefVarFuncs[nondefvar].insert(functions.begin(), functions.end());
		}

		for(auto iter : groupNonDefVarGVs) {
			std::string nondefvar = iter.first;
			const StringSet &globalvars = iter.second;
			nonDefVarGVs[nondefvar].insert(globalvars.begin(), globalvars.end());
		}
	}

	/**
	 * Replaces the existing initcalls with the new one.
	 */
	void replaceInitcalls(const InitcallMap &newInitcalls) {
		for(auto iter : newInitcalls) {
			std::string initcallName = iter.first;	
			Initcall &initcall = iter.second;
			initcalls[initcallName] = initcall;
		}
	}

	/**
	 * Removes the non defined global values of an initcall
	 * that is already in an initcall with a lower level.
	 */
//	void unifyNonDefVars();

	/**
	 * Remove all non defined global vars that were defined in lower level initcall.
	 */
	void filterNonDefVars(InitcallMap &groupInitcalls);

	/**
	 * Matches the value with the manager.
	 */
	void updateInitcallGroup(InitcallGroup &group);

	/**
	 * Determines the all relevant functions and globalvars as well as the functions being used
	 * to initialize a non defined variable.
	 */
	void preAnalysis();

	/**
	 * For each given global variable find the functions used by the global variable
	 * based on the constraint graph and PAG. Additionally the pta is used to find the
	 * remaining functions by backwarding from each of the functions found in the ConsG.
	 */
	void findFunctionsToPtr(PTACallGraph    *pta, 
				ConstraintGraph *consCG, 
				PAG 		*pag, 
				const StringSet &globalvars);

	/**
	 * Removes global variables from the set, that are neither a structure/array, 
	 * struct-pointer or a structure containing a structure-pointer.
	 */
	void filterNonStructTypes(StringSet &initcallGlobalVars);

	/**
	 * Imports all initcalls with the information that where 
	 * determined during the pre-analysis in a previuos analysis.
	 */
	bool importPreAnalysisResults();

	/**
	 * Exports the initcalls with the information determined in
	 * the pre-analysis.
	 */
	bool exportPreAnalysisResults();

	/** 
	 * Finds all relevant functions and globalvariables of a initcall.
	 * For reasons of optimization, a max depth is given. The callgraph
	 * of a initcall wont be analyzed any further.
	 */
	uint32_t analyze(const StringSet &initcallNames, PTACallGraph *pta, uint32_t depth, 
			bool broad, InitcallMap &res);

	/** 
	 * Finds all relevant functions and globalvariables of a initcall.
	 * For reasons of optimization, a max depth is given. The callgraph
	 * of a initcall wont be analyzed any further.
	 */
	InitcallMap analyze(uint32_t depth, bool broad);

	InitcallFactory(llvm::Module& module) { 
		this->module = &module;
		this->pta = new PTACallGraph(&module);

		preAnalysis();
	}

	~InitcallFactory() { 
		if(pta)
			delete pta;
		pta = nullptr;
	}

public:
	static InitcallFactory* createInitcallFactory(llvm::Module &module) {
		if(initcallFactory == nullptr)
			initcallFactory = new InitcallFactory(module);
		return initcallFactory;
	}

	static InitcallFactory* getInitcallFactory() {
		return initcallFactory;
	}

	static void releaseInitcallFactory() {
		if(initcallFactory)
			delete initcallFactory;
		initcallFactory = nullptr;
	}

	/**
	 * Get the created initcalls.
	 */
	const InitcallMap& getInitcalls() {
		return initcalls;
	}

	/**
	 * Get the number of initcalls.
	 */
	uint32_t getNumInitcalls() const {
		return initcalls.size();
	}
	
	/**
	 * Get the number of functions of all the initcalls.
	 */
	uint32_t getMaxNumInitcallFunctions() const {
		return maxNumInitcallFunctions;
	}

	/**
	 * Get the number of global variables of all the initcalls.
	 */
	uint32_t getMaxNumInitcallGlobalVars() const {
		return maxNumInitcallGlobalVars;
	}

	/**
	 * Get the number of functions of all the initcalls.
	 */
	uint32_t getNumRelevantFunctions() const {
		StringSet tmpSet;

		for(auto initcall : initcalls) {
			const StringSet &functions = initcall.second.getFunctions();
			tmpSet.insert(functions.begin(), functions.end());
		}

		return tmpSet.size();
	}

	/**
	 * Get the number of global variable of all the initcalls.
	 */
	uint32_t getNumRelevantGlobalVars() const {
		StringSet tmpSet;

		for(auto initcall : initcalls) {
			const StringSet &globalvars = initcall.second.getGlobalVars();
			tmpSet.insert(globalvars.begin(), globalvars.end());
		}

		return tmpSet.size();
	}

	/**
	 * Checks if the function name is a initcall.
	 */
	bool isInitcall(std::string funcName) {
		return initcalls.find(funcName) != initcalls.end();
	}

	/**
	 * Get the functions that are relevant for the given initcall.
	 */
	const StringSet& getInitcallFunctions(std::string initcallName) const {
		const Initcall &initcall = initcalls.at(initcallName);
		return initcall.getFunctions();
	}
		
	/**
	 * Get the global variables that are relevant for the given initcall.
	 */
	const StringSet& getInitcallGlobalVars(std::string initcallName) const {
		const Initcall &initcall = initcalls.at(initcallName);
		return initcall.getGlobalVars();
	}

	/**
	 * Get the non defined variables that are relevant for the given initcall.
	 */
	const StringSet& getInitcallNonDefVars(std::string initcallName) const {
		const Initcall &initcall = initcalls.at(initcallName);
		return initcall.getNonDefVars();
	}

	/**
	 * Determines all initcall names.
	 */
	StringSet getInitcallNames() const {
		StringSet initcallNames;

		std::transform(initcalls.begin(), initcalls.end(), 
			       std::inserter(initcallNames, initcallNames.end()),
			       [](std::pair<std::string, Initcall> pair){ return pair.first; });

		return initcallNames;
	}

	/**
	 * Get all the functions of the initcalls. (no duplicates)
	 */
	StringSet getAllInitcallFuncs(const InitcallMap &initcalls) const;

	/**
	 * Get all the global variables of the initcalls. (no duplicates)
	 */
	StringSet getAllInitcallGlobalVars(const InitcallMap &initcalls) const;

	/**
	 * Get all the non defined global variables of the initcalls. (no duplicates)
	 */
	StringSet getAllInitcallNonDefVars(const InitcallMap &initcalls) const;

	/**
	 * Get all the functions of the non defined vars. (no duplicates)
	 */
	StringSet getAllNonDefVarFuncs() const;

	/**
	 * Get the maximal depth the initcalls were analyzed.
	 */
	uint32_t getMaxCallGraphDepth() const {
		return maxCGDepth;
	}

	/**
	 * Get the functions of the non defined global variables.
	 */
	const StrToStrSet& getNonDefVarFuncs() const {
		return nonDefVarFuncs;
	}

	/**
	 * Get the globalvars of the non defined global variables.
	 */
	const StrToStrSet& getNonDefVarGVs() const {
		return nonDefVarGVs;
	}	
	
private:
	// The gerneral callgraph. (not optimized)
	PTACallGraph *pta;

	// All initcalls of the module (up to the depth).
	InitcallMap initcalls;

	// All initcalls group (either by a function limit or by their level).
	InitcallGroupList initcallGroups;

	// All corresponging/relevant functions of a potentially non defined globalvar.
	StrToStrSet nonDefVarFuncs;

	// All corresponging/relevant globalvars of a potentially non defined globalvar.
	StrToStrSet nonDefVarGVs;

	// The maximal depth the initcalls were analyzed.
	static uint32_t maxCGDepth;

	// The maximal number of functions used by all initcalls.
	static uint32_t maxNumInitcallFunctions;

	// The maximal number of global variables used by all initcalls. 
	static uint32_t maxNumInitcallGlobalVars;

	// A funtion which name contains this string as a substring is
	// interpreted as an initcall.
	const std::string initcallSubName = "__initcall_";

	llvm::Module* module;
};
#endif // INITCALL_FACTORY_H
