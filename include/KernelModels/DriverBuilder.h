#ifndef DRIVER_BUILDER_H
#define DRIVER_BUILDER_H

#include "KernelModels/ContextBuilder.h"
#include "KernelModels/Driver.h"

/**
 * Abstract class which defines the functions required to handle contexts.
 */
class DriverBuilder: public ContextBuilder {
public:
	DriverBuilder(llvm::Module &module): ContextBuilder(module) { }

	virtual ~DriverBuilder() { }

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
	 * Find the functions inside the driver file. (API)
	 */
	void defineDriverAPI(Driver *driver);
};


#endif // DRIVER_BUILDER_H
