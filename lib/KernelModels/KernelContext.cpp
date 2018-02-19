#include "KernelModels/KernelContext.h"
#include "Util/DebugUtil.h"

KernelContext* KernelContext::cxtContainer = nullptr;

uint32_t KernelContext::kernelOrigNumFunctions = 0;
uint32_t KernelContext::kernelOrigNumGlobalVars = 0;
uint32_t KernelContext::kernelOrigNumNonDefVars = 0;

void KernelContext::setInitcalls(const InitcallMap &initcalls) {
	this->initcalls = initcalls;
}

InitcallMap& KernelContext::getInitcalls() {
	return initcalls;
}

IntToStrSet KernelContext::getInitcallLevelMap() {
	IntToStrSet initcallLevelMap;

	for(const auto &iter : initcalls) {
		std::string initcallName = iter.first;
		const Initcall &initcall = iter.second;
		initcallLevelMap[initcall.getLevel()].insert(initcallName);
	}

	return initcallLevelMap;
}

void KernelContext::setAPI(KernelContextObj *api) {
	this->api = api;
}

KernelContextObj* KernelContext::getAPI() {
	return api;
}

void KernelContext::setNonDefVarFuncs(const StrToStrSet &nonDefVarFuncs) {
	this->nonDefVarFuncs = nonDefVarFuncs;
}

StrToStrSet& KernelContext::getNonDefVarFuncs() {
	return nonDefVarFuncs;
}

void KernelContext::setNonDefVarGVs(const StrToStrSet &nonDefVarGVs) {
	this->nonDefVarGVs = nonDefVarGVs;
}

const StrToStrSet& KernelContext::getNonDefVarGVs() {
	return nonDefVarGVs;
}

bool KernelContext::isInitcall(const llvm::Function *F) const {
	if(!F || !F->hasName())
		return false;

	return initcalls.find(F->getName()) != initcalls.end();
}
	
StringSet KernelContext::getAllFunctions() const {
	StringSet functions = getAllInitcallFuncs();
	StringSet &apiFunctions = api->getFunctions();

	functions.insert(apiFunctions.begin(), apiFunctions.end());	

	return functions;
}

StringSet KernelContext::getAllGlobalVars() const {
	StringSet globalvars = getAllInitcallGlobalVars();
	StringSet &apiGVs = api->getGlobalVars();
	globalvars.insert(apiGVs.begin(), apiGVs.end());	

	return globalvars;
}

StringSet KernelContext::getAllInitcallNames() const {
	StringSet names;

	std::transform(initcalls.begin(), initcalls.end(),
		       std::inserter(names, names.end()),
		       [](std::pair<std::string, Initcall> pair){ return pair.first; });

	return names;
}

StringSet KernelContext::getAllInitcallFuncs() const {
	StringSet functions;

	for(const auto &iter : initcalls) {
		const Initcall &initcall = iter.second;
		functions.insert(initcall.getFunctions().begin(), initcall.getFunctions().end());
	}	

	return functions;
}

StringSet KernelContext::getAllInitcallGlobalVars() const {
	StringSet globalvars;

	for(const auto &iter : initcalls) {
		const Initcall &initcall = iter.second;
		globalvars.insert(initcall.getGlobalVars().begin(), initcall.getGlobalVars().end());
	}	

	return globalvars;
}

StringSet KernelContext::getAllInitcallNonDefVars() const {
	StringSet nondefvars;

	for(const auto &iter : initcalls) {
		const Initcall &initcall = iter.second;
		nondefvars.insert(initcall.getNonDefVars().begin(), initcall.getNonDefVars().end());
	}	

	return nondefvars;
}

uint32_t KernelContext::getInitcallsMaxCGDepth() const {
	uint32_t tmpDepth = 0; 
	uint32_t maxDepth = 0;

	for(auto iter : initcalls) {
		const Initcall &initcall = iter.second;
		tmpDepth = initcall.getMaxCGDepth();

		if(tmpDepth > maxDepth)
			maxDepth = tmpDepth;
	}

	return maxDepth;
}

StringSet KernelContext::getNonDefVarFunctions(const Initcall &initcall, const StringSet &nonDefVars) const {
	const StringSet &initcallFuncs = initcall.getFunctions();
	StringSet relevantNonDefVarFuncs;

	for(auto iter = nonDefVars.begin(); iter != nonDefVars.end(); ++iter) {
		const StringSet &funcs = nonDefVarFuncs.at(*iter);
		StringSet interSet;

		set_intersection(initcallFuncs.cbegin(), initcallFuncs.cend(), 
				 funcs.begin(), funcs.end(), 
				 std::inserter(interSet, interSet.begin()));

		relevantNonDefVarFuncs.insert(interSet.begin(), interSet.end());
	}

	relevantNonDefVarFuncs.insert(initcall.getName());

	return relevantNonDefVarFuncs;
}

StringSet KernelContext::getNonDefVarGlobalVars(const Initcall &initcall, const StringSet &nonDefVars) const {
	const StringSet &initcallGVs = initcall.getGlobalVars();
	StringSet relevantNonDefVarGVs;

	for(auto iter = nonDefVars.begin(); iter != nonDefVars.end(); ++iter) {
		const StringSet &globalvars = nonDefVarGVs.at(*iter);
		StringSet interSet;

		set_intersection(initcallGVs.cbegin(), initcallGVs.cend(), 
				 globalvars.begin(), globalvars.end(), 
				 std::inserter(interSet, interSet.begin()));

		relevantNonDefVarGVs.insert(interSet.begin(), interSet.end());
	}

	return relevantNonDefVarGVs;
}

uint32_t KernelContext::getKernelOrigNumFuncs() {
	return kernelOrigNumFunctions;
}

uint32_t KernelContext::getKernelOrigNumGVs() {
	return kernelOrigNumGlobalVars;
}

uint32_t KernelContext::getKernelOrigNumNonDefVars() {
	return kernelOrigNumNonDefVars;
}
