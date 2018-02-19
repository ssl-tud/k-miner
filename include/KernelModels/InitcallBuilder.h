#ifndef INITCALL_BUILDER_H
#define INITCALL_BUILDER_H

/**
 * Abstract class which defines the functions required to handle contexts.
 */
class InitcallBuilder: public ContextBuilder<Initcall> {
public:
	InitcallBuilder(llvm::Module &module): ContextBuilder(module) { }

	virtual ~InitcallBuilder() { }

	/**
	 * Determine the functions and variable (context) that have to be analyzed. 
	 */
	virtual void build(std::string cxtRootName);

	/**
	 * Remove irrelevant functions and variables from the context.
	 */
	virtual void optimizeContext();
private:
	/** 
	 * Finds all the initcalls inside the given module. 
	 */
	void defineInitcallAPI();

	/**
	 * Imports all initcalls with the information that where 
	 * determined during the pre-analysis in a previuos analysis.
	 */
	bool importInitcallContexts();

	/**
	 * Exports the initcalls with the information determined in
	 * the pre-analysis.
	 */
	bool exportInitcallContexts();
};


#endif // INITCALL_BUILDER_H
