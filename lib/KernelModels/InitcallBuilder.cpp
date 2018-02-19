#include "KernelModels/InitcallBuilder.cpp"

void InitcallBuilder::build(std::string cxtRootName) {
	outs() << "____________________________________________________\n";
	outs() << "INITCALLHANDLING:\n\n";

	if(importInitcallContexts())
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

	exportInitcallContexts();
}

void InitcallBuilder::optimizeContext() {
	std::unique_ptr<llvm::Module> optModule = CloneModule(module);
	anderOpt(*optModule.get(), context);
	flowOpt(*optModule.get(), context);
	optModule.reset();
}

void InitcallBuilder::defineInitcallAPI() {
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
			llvm::Value *initcallValue = analysisUtil::stripAllCasts(C);
			std::string initcallName = initcallValue->getName();
			uint32_t level = getLevelOfName(globalName);
			Initcall initcall(initcallName, level);
			initcalls[initcallName] = initcall;	
		}
	}
}

bool InitcallBuilder::importInitcallContexts() {
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

bool InitcallBuilder::exportInitcallContexts() {
	std::string fileName = PreAnalysisResults;
	std::ofstream out(fileName.c_str(), std::ofstream::out);

	if(out) {
		out << initcalls.size() << "\n";

		for(auto iter = initcalls.begin(); iter != initcalls.end(); ++iter) {
			Initcall &initcall = iter->second;
			out << initcall;
//			out << (*iter) << "\n";
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

	outs() << "Context of " << initcalls.size() << " initcalls exported\n";

	return true;
}
