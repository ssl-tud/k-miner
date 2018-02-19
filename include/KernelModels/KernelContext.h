#ifndef KERNEL_CONTEXT_H
#define KERNEL_CONTEXT_H

#include "KernelModels/Initcall.h"
#include "KernelModels/Systemcall.h"
#include "KernelModels/Driver.h"
#include "SVF/Util/BasicTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"

typedef std::set<std::string> StringSet;
typedef std::map<std::string, StringSet> StrToStrSet;
typedef std::map<int, StringSet> IntToStrSet;

/***
 * Provides all kernel related infos, that are handles by different Passes.
 * It contains the current Systemcall, the potential necessary initcalls
 * and the functions that are use by the non defined variables.
 */
class KernelContext {
	static KernelContext *cxtContainer;

	KernelContext() : api(nullptr) { }

	~KernelContext() { }
public:
	static KernelContext* createKernelContext() {
		if(cxtContainer == nullptr)
			cxtContainer = new KernelContext();
		return cxtContainer;
	}

	static KernelContext* getKernelContext() {
		return cxtContainer;
	}

	static void releaseKernelContext() {
		if(cxtContainer)
			delete cxtContainer;
		cxtContainer = nullptr;
	}

	/**
	 * Set the original values of the kernel, before
	 * the module (kernel) is minimized.
	 */
	static void initKernelOriginals(uint32_t numFuncs, uint32_t numGlobalVars, uint32_t numNonDefVars) {
		kernelOrigNumFunctions = numFuncs;
		kernelOrigNumGlobalVars = numGlobalVars; 
		kernelOrigNumNonDefVars = numNonDefVars;
	}

	/**
	 * Checks if the functions is either a system call or a driver.
	 */
	bool isAPIFunction(std::string name) {
		assert(api&& "The object of the kernel context has not been defined");
		if(api->inContextRoot(name))
			return true;
		// TODO this has to get better.
		if(Systemcall *syscall = dyn_cast<Systemcall>(api))
			return syscall->getAlias() == name;

		return false;
	}

	/**
	 * Checks if the functions is either in the system call or the driver context.
	 */
	bool inAPIContext(std::string name) {
		return api->hasFunction(name);	
	}

	StringSet getAPIFunctions() {
		StringSet &apiFunc = api->getContextRoot();

		// TODO this has to get better.
		if(Systemcall *syscall = dyn_cast<Systemcall>(api))
			apiFunc.insert(syscall->getAlias());

		return apiFunc;
	}

	/**
	 * Set the current driver.
	 */
	void setAPI(KernelContextObj *api);

	/**
	 * Get the current driver.
	 */
	KernelContextObj* getAPI();

	/**
	 * Set all potential necessary initcalls.
	 */
	void setInitcalls(const InitcallMap &initcalls);

	/**
	 * Get all the potential necessary initcalls.
	 */
	InitcallMap& getInitcalls();

	/**
	 * Get the initcall names mapped to their levels.
	 */
	IntToStrSet getInitcallLevelMap();

	/**
	 * Set the functions that are used by global variables, that
	 * are potentialy not defined. 
	 */
	void setNonDefVarFuncs(const StrToStrSet &nonDefVarFuncs);

	/**
	 * Get the functions that are used by global variables, that
	 * are potentialy not defined. 
	 */
	StrToStrSet& getNonDefVarFuncs();

	/**
	 * Set the global vars that are used by global variables, that
	 * are potentialy not defined. 
	 */
	void setNonDefVarGVs(const StrToStrSet &nonDefVarGVs);

	/**
	 * Get the global vars that are used by global variables, that
	 * are potentialy not defined. 
	 */
	const StrToStrSet& getNonDefVarGVs();

	/**
	 * Checks if a function is an initcall.
	 */
	bool isInitcall(const llvm::Function *F) const;

	/**
	 * Get all functions of the api and all the initcalls.
	 */
	StringSet getAllFunctions() const;

	/**
	 * Get all global variables of the api and all the initcalls.
	 */
	StringSet getAllGlobalVars() const;

	/**
	 * Get all initcall names.
	 */
	StringSet getAllInitcallNames() const;

	/**
	 * Get all the functions used by at least one initcall.
	 */
	StringSet getAllInitcallFuncs() const;

	/**
	 * Get all the global variables used by at least one initcall.
	 */
	StringSet getAllInitcallGlobalVars() const;

	/**
	 * Get all the non defined global variables used by at least one initcall.
	 */
	StringSet getAllInitcallNonDefVars() const;

	/**
	 * Get the maximal depth of all initcalls.
	 */
	uint32_t getInitcallsMaxCGDepth() const;

	/**
	 * Get the functions used by a non defined global variable and which is also used
	 * by the given initcall.
	 */
	StringSet getNonDefVarFunctions(const Initcall &initcall, const StringSet &nonDefVars) const;

	/**
	 * Get the globalvars used by a non defined global variable and which is also used
	 * by the given initcall.
	 */
	StringSet getNonDefVarGlobalVars(const Initcall &initcall, const StringSet &nonDefVars) const;

	/**
	 * Get the original number of functions inside the kernel.
	 */
	static uint32_t getKernelOrigNumFuncs();

	/**
	 * Get the original number of global variables inside the kernel.
	 */
	static uint32_t getKernelOrigNumGVs();

	/**
	 * Get the original number of non defined variables inside the kernel.
	 */
	static uint32_t getKernelOrigNumNonDefVars();

private:
	// Can be for instance a system call or driver.
	KernelContextObj *api;

	// Map of all initcalls that might by relevant for the analysis.
	InitcallMap initcalls;

	// The original values of the kernel. (before the minimisation).
	static uint32_t kernelOrigNumFunctions;
	static uint32_t kernelOrigNumGlobalVars;
	static uint32_t kernelOrigNumNonDefVars;
	
	// Set the functions that are used by global variables, that 
	// are potentialy not defined. The global variable has
	// to be used in the systemcall and a function has to by used
	// by at least one initcall. The global variable has also to 
	// be a structure(-pointer), array or a structure containing a
	// other structure pointer.
	StrToStrSet nonDefVarFuncs;
	StrToStrSet nonDefVarGVs;
};

#endif // KERNEL_CONTEXT_H
