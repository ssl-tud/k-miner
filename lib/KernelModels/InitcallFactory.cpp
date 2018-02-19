#include "KernelModels/InitcallFactory.h"
#include "Util/PtrCallSetAnalysis.h"
#include "Util/KernelAnalysisUtil.h"
#include "Util/CallGraphAnalysis.h"
#include "SVF/MemoryModel/PointerAnalysis.h"
#include "SVF/MemoryModel/PAGBuilder.h"
#include "SVF/MemoryModel/MemModel.h"
#include <omp.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <stdlib.h>

using namespace llvm;

static cl::opt<std::string> PreAnalysisResults("initcall-contexts", cl::init("initcall_contexts.txt"),
		cl::desc("Imports/exports the outcome of the first initcall analysis"));

#define PREANALYSISRESULTS PreAnalysisResults != ""

InitcallFactory* InitcallFactory::initcallFactory = nullptr;
uint32_t InitcallFactory::maxCGDepth = 0;
uint32_t InitcallFactory::maxNumInitcallFunctions = 0;
uint32_t InitcallFactory::maxNumInitcallGlobalVars = 0;

void InitcallFactory::findInitcalls() {
//	outs() << "Find initcalls ...\n";
	llvm::Function *initcallF = nullptr;
	auto globalIterB = module->global_begin();
	auto globalIterE = module->global_end();

	for(; globalIterB != globalIterE; ++globalIterB) {
		std::string globalName = (*globalIterB).getName();
		std::size_t pos = globalName.find(initcallSubName);

		if(globalName == "__initcall_start" || globalName == "__initcall_end")
			continue;

		if(pos != std::string::npos) {
			llvm::Constant *C = (*globalIterB).getInitializer();
			const llvm::Value *initcallValue = analysisUtil::stripAllCasts(C);
			std::string initcallName = initcallValue->getName();
			uint32_t level = getLevelOfName(globalName);
			Initcall initcall(initcallName, level);
			initcalls[initcallName] = initcall;	
		}
	}
}

InitcallGroupList InitcallFactory::groupInitcallsByLimit(const InitcallMap &initcalls) {
	InitcallGroupList initcallGroups;

	for(auto iter = initcalls.begin(); iter != initcalls.end(); ++iter) {
		const Initcall &initcall = iter->second;
		const std::string initcallName = initcall.getName();
		const StringSet &initFuncs = initcall.getFunctions();
		const StringSet &initGVs = initcall.getGlobalVars();
		const StringSet &initNonDefVars = initcall.getNonDefVars();
		bool foundGroup = false;

		for(auto iter2 = initcallGroups.begin(); iter2 != initcallGroups.end(); iter2++) {
			InitcallGroup &initGroup = *iter2;
			StringSet &gInitcalls = initGroup.initcalls;
			StringSet &gInitFuncs = initGroup.functions;
			StringSet &gInitGVs = initGroup.globalvars;
			StringSet &gInitNonDefVars = initGroup.nondefvars; 
			StringSet tmpFuncs, tmpGVs, tmpNonDefVars;

			tmpFuncs.insert(initFuncs.begin(), initFuncs.end());
			tmpFuncs.insert(gInitFuncs.begin(), gInitFuncs.end());
			tmpGVs.insert(initGVs.begin(), initGVs.end());
			tmpGVs.insert(gInitGVs.begin(), gInitGVs.end());
			tmpNonDefVars.insert(initNonDefVars.begin(), initNonDefVars.end());
			tmpNonDefVars.insert(gInitNonDefVars.begin(), gInitNonDefVars.end());

			if(tmpFuncs.size() <= InitcallGroup::funcLimit) {
				gInitcalls.insert(initcallName);
				gInitFuncs = tmpFuncs;
				gInitGVs = tmpGVs;
				gInitNonDefVars = tmpNonDefVars;
				foundGroup = true;
				break;
			}
		}

		if(!foundGroup) {
			InitcallGroup group;
			group.initcalls.insert(initcallName);
			group.functions = initFuncs;
			group.globalvars = initGVs;
			group.nondefvars = initNonDefVars;
			initcallGroups.push_back(group);
		}
	}	

	return initcallGroups;
}

void InitcallFactory::groupInitcallsByLevel(const InitcallMap &initcalls) {
	// Group all initcalls.
	for(const auto &iter : initcalls) {
		std::string initcallName = iter.first;
		const Initcall &initcall = iter.second;
		const StringSet &initFuncs = initcall.getFunctions();
		const StringSet &initGVs = initcall.getGlobalVars();
		const StringSet &initNonDefVars = initcall.getNonDefVars();
		uint32_t level = initcall.getLevel();

		InitcallGroup &group = getNextFreeInitcallGroup(level, initFuncs); 

		group.initcalls.insert(initcallName);
		group.functions.insert(initFuncs.begin(), initFuncs.end());
		group.globalvars.insert(initGVs.begin(), initGVs.end());
		group.nondefvars.insert(initNonDefVars.begin(), initNonDefVars.end());
	}	

	// Sort the groups in the list by their levels.
	initcallGroups.sort([](const InitcallGroup &g1, const InitcallGroup &g2) { return g1.level < g2.level;});

//	for(auto iter : initcallGroups)
//		iter.dump();
}

void InitcallFactory::updateInitcallGroup(InitcallGroup &group) {
	group.functions.clear();
	group.globalvars.clear();
	group.nondefvars.clear();

	for(std::string initcallName : group.initcalls) {
		const Initcall &initcall = initcalls[initcallName];
		const StringSet &initcallFuncs = initcall.getFunctions();
		const StringSet &initcallGlobalVars = initcall.getGlobalVars();
		const StringSet &initcallNonDefVars = initcall.getNonDefVars();

		group.functions.insert(initcallFuncs.begin(), initcallFuncs.end()); 
		group.globalvars.insert(initcallGlobalVars.begin(), initcallGlobalVars.end()); 
		group.nondefvars.insert(initcallNonDefVars.begin(), initcallNonDefVars.end()); 
	}
}

void InitcallFactory::handleInitcallGroup(InitcallGroup &group) {
	StringSet &relevantFuncs = group.functions;
	StringSet &relevantGVs = group.globalvars;
	StringSet &relevantNonDefVars = group.nondefvars;
	InitcallMap groupInitcallMap;
	uint32_t groupMaxCGDepth = 0;

	std::unique_ptr<llvm::Module> InitcallModule = CloneModule(module);
	llvm::Module &m = *InitcallModule.get();

	analysisUtil::minimizeModule(m, relevantFuncs, relevantGVs);

	double start = omp_get_wtime();
	AndersenWaveDiff *anderdiff = AndersenWaveDiff::createAndersenWaveDiff(m); 

	PTACallGraph *fpta = anderdiff->getPTACallGraph();
	ConstraintGraph *fconsCG = anderdiff->getConstraintGraph();
	PAG *pag = anderdiff->getPAG();

	// This time the analysis will be more precise and only contains functions and globalvars
	// that were actually used.
	groupMaxCGDepth = analyze(group.initcalls, 
				    fpta, 
				    std::numeric_limits<uint32_t>::max(), 
				    false, 
				    groupInitcallMap);

	// Remove global variables that were already defined in a previous initcall.
	filterNonDefVars(groupInitcallMap);

	replaceInitcalls(groupInitcallMap);

	updateInitcallGroup(group);

	// Maps the functions that are used by the global structures (non defined variables). 
	findFunctionsToPtr(fpta, fconsCG, pag, group.nondefvars);

	if(groupMaxCGDepth > maxCGDepth)
		maxCGDepth = groupMaxCGDepth;

	AndersenWaveDiff::releaseAndersenWaveDiff();
	// releaseAndersenWaveDiff wont destroy the pag
	PAG::releasePAG();
	InitcallModule.reset();
}

void InitcallFactory::preAnalysis() {
	outs() << "____________________________________________________\n";
	outs() << "INITCALLHANDLING:\n\n";

	if(PREANALYSISRESULTS && importPreAnalysisResults())
		return;

	findInitcalls();

	// This will be a prev-analysis to find all kinds of relevant functions and globalvars. This
	// contains also functions that were only defined as a function pointer.
	InitcallMap tmpInitcalls = analyze(std::numeric_limits<uint32_t>::max(), true);

	groupInitcallsByLevel(tmpInitcalls);

	for(auto &group : initcallGroups)
		handleInitcallGroup(group);	

	maxNumInitcallFunctions = getNumRelevantFunctions();
	maxNumInitcallGlobalVars = getNumRelevantGlobalVars();

	if(PREANALYSISRESULTS)
		exportPreAnalysisResults();
}

void InitcallFactory::filterNonDefVars(InitcallMap &groupInitcalls) {
	for(auto &iter : groupInitcalls) {
		Initcall &initcall = iter.second;
		StringSet &nonDefVars = initcall.getNonDefVars();
		uint32_t level = initcall.getLevel();

		for(auto &iter2 : initcallGroups) {
			InitcallGroup &group = iter2; //initcallGroups[i];
			StringSet &gInitNonDefVars = group.nondefvars;
			StringSet tmpSet;

			if(group.level >= level)
				continue;
			
			// Removes all non defined global vars that were defined in lower level initcall.
			std::set_difference(nonDefVars.begin(), nonDefVars.end(), 
					    gInitNonDefVars.begin(), gInitNonDefVars.end(), 
					    std::inserter(tmpSet, tmpSet.begin()));

			nonDefVars = tmpSet;
		}
	}
}

void InitcallFactory::findFunctionsToPtr(PTACallGraph 	 *pta, 
					 ConstraintGraph *consCG, 
					 PAG 		 *pag, 
					 const StringSet &globalvars) {

	const uint32_t max = globalvars.size();
	StrToStrSet globalvarToFuncs;
	StrToStrSet globalvarToGVs;

	#pragma omp parallel
	{
	PtrCallSetAnalysis *ptfAnalysis;

	LocalCallGraphAnalysis *LCGA = new LocalCallGraphAnalysis(pta);
	LCGA->setBackwardFilter(getInitcallNames());

	#pragma omp critical (new_ptfAnalysis)
	ptfAnalysis = new PtrCallSetAnalysis(consCG, pag);

	#pragma omp for
	for(int i=0; i < max; ++i) {
		auto iter = globalvars.begin();
		advance(iter, i);

		std::string globalVarName = *iter;
		ptfAnalysis->analyze(globalVarName);
		StringSet &relevantFuncs = ptfAnalysis->getRelevantFunctions();		
		StringSet &relevantGVs = ptfAnalysis->getRelevantGlobalVars();		
		StringSet tmpSet = relevantFuncs;

		// Add the functions that call the relevant functions.
		for(auto iter : tmpSet) {
			LCGA->analyze(iter, false, false, std::numeric_limits<uint32_t>::max());
			const StrListSet &paths = LCGA->getBackwardFuncPaths();

			for(const auto &iter2 : paths) {
				const StringList &path = iter2;
				relevantFuncs.insert(path.begin(), path.end());
			}
		}

		#pragma omp critical (nonDefVarFuncs)
		globalvarToFuncs[globalVarName] = relevantFuncs;
		#pragma omp critical (nonDefVarGVs)
		globalvarToGVs[globalVarName] = relevantGVs;
	}

	#pragma omp barrier

	#pragma omp critical (free_ptfAnalysis)
	delete ptfAnalysis;
	delete LCGA;
	}

	mergeNonDefVarFunctions(globalvarToFuncs, globalvarToGVs);
}

InitcallMap InitcallFactory::analyze(uint32_t depth, bool broad) {
	InitcallMap tmpInitcallMap;	
	StringSet initcallNames = getInitcallNames();

	uint32_t actualdepth = analyze(initcallNames, pta, depth, broad, tmpInitcallMap);

//	outs() << "actualdepth= " << actualdepth << "\n";

	return tmpInitcallMap;
}

uint32_t InitcallFactory::analyze(const StringSet &initcallNames, 
				  PTACallGraph *pta, 
				  uint32_t depth, 
				  bool broad,
				  InitcallMap &res) {

	const uint32_t numInitcalls = initcallNames.size();
	uint32_t maxDepth = 0;
	uint32_t tmpDepth = 0;

	#pragma omp parallel firstprivate(numInitcalls, tmpDepth) 
	{
	InitcallMap *localInitcalls = new InitcallMap();
	LocalCallGraphAnalysis *LCGA  = new LocalCallGraphAnalysis(pta);

	// Starting from the some given functions(e.g. syscalls) we calc. 
	// all the relevant functions.
	#pragma omp for
	for(int i=0; i < numInitcalls; ++i) {
		auto initcallIter = initcallNames.begin();
		advance(initcallIter, i);

		std::string initcallName = *initcallIter;
		const Initcall &origInitcall = initcalls[initcallName];

		LCGA->analyze(initcallName, true, broad, depth);

		tmpDepth = LCGA->getActualDepth();

		if(tmpDepth > maxDepth) {
			#pragma omp critical (maxDepth)
			{
			if(tmpDepth > maxDepth) 
				maxDepth = tmpDepth;
			}
		}

		const StringSet &initcallFuncs = LCGA->getForwardFuncSlice();
		const StringSet &initcallGlobalVars = LCGA->getRelevantGlobalVars();
		StringSet initcallNonDefVars = initcallGlobalVars;

		// Remove the global variables that doesn't have to be initialized. 
		// e.g simple integer pointer
		analysisUtil::filterNonStructTypes(*module, initcallNonDefVars);

		Initcall initcall(initcallName, origInitcall.getLevel());

		// Save the new relevant functions.
		initcall.setFunctions(initcallFuncs);

		// Save the new relevant global variables.
		initcall.setGlobalVars(initcallGlobalVars);

		// Save the new relevant global structures (-pointer) or arrays.
		initcall.setNonDefVars(initcallNonDefVars);

		initcall.setMaxCGDepth(tmpDepth);

		(*localInitcalls)[initcallName] = initcall;
	}

	#pragma omp barrier

	#pragma omp critical (local_initcalls)
	res.insert(localInitcalls->begin(), localInitcalls->end());

	delete localInitcalls;
	delete LCGA;
	}

	return maxDepth;
}

StringSet InitcallFactory::getAllInitcallFuncs(const InitcallMap &initcalls) const {
	StringSet functions;

	for(auto iter : initcalls) {
		const Initcall &initcall = iter.second;
		functions.insert(initcall.getFunctions().begin(), initcall.getFunctions().end());
	}	

	return functions;
}

StringSet InitcallFactory::getAllInitcallGlobalVars(const InitcallMap &initcalls) const {
	StringSet globalvars;

	for(auto iter : initcalls) {
		const Initcall &initcall = iter.second;
		globalvars.insert(initcall.getGlobalVars().begin(), initcall.getGlobalVars().end());
	}	

	return globalvars;
}

StringSet InitcallFactory::getAllInitcallNonDefVars(const InitcallMap &initcalls) const {
	StringSet nondefvars;

	for(auto iter : initcalls) {
		const Initcall &initcall = iter.second;
		nondefvars.insert(initcall.getNonDefVars().begin(), initcall.getNonDefVars().end());
	}	

	return nondefvars;
}

StringSet InitcallFactory::getAllNonDefVarFuncs() const {
	StringSet functions;

	for(auto iter : nonDefVarFuncs) {
		const StringSet &tmpFuncs = iter.second;
		functions.insert(tmpFuncs.begin(), tmpFuncs.end());
	}	

	return functions;
}

bool InitcallFactory::importPreAnalysisResults() {
	std::string fileName = PreAnalysisResults;
	std::ifstream in(fileName.c_str(), std::ifstream::in);
	u32_t numInitcalls = 0;
	u32_t numNonDefVars = 0;
	u32_t numNonDefVarFuncs = 0;
	u32_t numNonDefVarGVs = 0;

	if(in) {
		// Check if file is empty.
		if(in.peek() == std::ifstream::traits_type::eof())
			return false;

		in >> numInitcalls;

		for(int i=0; i < numInitcalls; ++i) {
			Initcall initcall;
			in >> initcall;
			initcalls[initcall.getName()] = initcall;
		}

		in >> maxCGDepth;
		in >> maxNumInitcallFunctions;
		in >> maxNumInitcallGlobalVars;
		in >> numNonDefVars;

		for(int i=0; i < numNonDefVars; ++i) {
			std::string nondefvar= "";
			in >> nondefvar;
			in >> numNonDefVarFuncs;
			StringSet &funcSet = nonDefVarFuncs[nondefvar];

			for(int j=0; j < numNonDefVarFuncs; ++j) {
				std::string function = "";
				in >> function;
				funcSet.insert(function);
			}

			in >> numNonDefVarGVs;
			StringSet &gvSet = nonDefVarGVs[nondefvar];

			for(int j=0; j < numNonDefVarGVs; ++j) {
				std::string globalvar = "";
				in >> globalvar;
				gvSet.insert(globalvar);
			}
		}
			
		in.close();
	}

	outs() << "Context of " << initcalls.size() << " initcalls imported\n";

	return initcalls.size() > 0;
}

bool InitcallFactory::exportPreAnalysisResults() {
	std::string fileName = PreAnalysisResults;
//	outs() << "Export Pre-Analysis Results " << fileName << " ...\n";
	std::ofstream out(fileName.c_str(), std::ofstream::out);

	if(out) {
		out << initcalls.size() << "\n";

		for(auto iter = initcalls.begin(); iter != initcalls.end(); ++iter) {
			Initcall &initcall = iter->second;
			out << initcall;
		}

		out << maxCGDepth << "\n";
		out << maxNumInitcallFunctions << "\n";
		out << maxNumInitcallGlobalVars << "\n";
		out << nonDefVarFuncs.size() << "\n";

		assert(nonDefVarFuncs.size() == nonDefVarGVs.size() && "nonDefVar map differ");

		for(int i=0; i < nonDefVarFuncs.size(); i++) {
			auto funcIter = nonDefVarFuncs.begin();
			auto gvIter = nonDefVarGVs.begin();
			advance(funcIter, i);
			advance(gvIter, i);

			std::string nondefvar = funcIter->first;
			StringSet &funcSet = funcIter->second;
			StringSet &gvSet = gvIter->second;

			out << nondefvar << "\n";
			out << funcSet.size() << "\n";

			for(auto iter2 = funcSet.begin(); iter2 != funcSet.end(); ++iter2) {
				out << (*iter2) << "\n";	
			}
			
			out << gvSet.size() << "\n";

			for(auto iter2 = gvSet.begin(); iter2 != gvSet.end(); ++iter2) {
				out << (*iter2) << "\n";	
			}
		}

		out.close();
	} else
		return false;

//	outs() << "\t" << initcalls.size() << " initcalls exported\n";

	return true;
}
