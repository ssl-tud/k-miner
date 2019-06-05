#ifndef BUG_H
#define BUG_H

#include "Util/BasicTypes.h"
#include <llvm/Support/Casting.h>
#include <sstream>
#include <iostream>
#include <iomanip>

#define NUMDIGITS(n) ({n > 0 ? (int) log10 ((double) n) + 1 : 1;})

class Location;
class Bug;

typedef std::map<std::string, Location> StringToLoc;

/***
 * Contains the information where an obj (instruction, variable, function, ...) 
 * in the module is located.
 */
class Location {
public:
	Location(std::string fileName, std::string funcName, u32_t line, std::string varName=""):
		fileName(fileName), funcName(funcName), line(line), varName(varName) { }

	Location(): fileName(""), funcName(""), line(0), varName("") { }

	~Location() { }

	std::string getFileName() const {
		return fileName;
	}

	std::string getFuncName() const {
		return funcName;
	}

	u32_t getLine() const {
		return line;
	}

	std::string getVarName() const {
		return varName;
	}

	std::string toString() const {
		std::stringstream rawLoc;

	       	rawLoc << "File" << std::setw(50-fileName.size()) << " " << fileName << "\n";	
	       	rawLoc << "Function" << std::setw(46-funcName.size()) << " " << funcName << "\n";	
	       	rawLoc << "Line" << std::setw(50-NUMDIGITS(line)) << " " << line << "\n";	
		if(varName != "")
	       		rawLoc << "Name" << std::setw(50-varName.size()) << " " << varName << "\n";	

		return rawLoc.str();
	}

	std::string toOnlinerString() const {
		std::stringstream rawLoc;

		if(varName != "")
	       		rawLoc << varName;	

		rawLoc << " : ";

		rawLoc << fileName << " ";
	        rawLoc << funcName << " (ln:";
	        rawLoc << line << ")\n";

		return rawLoc.str();
	}

	inline bool operator== (const Location& rhs) const {
		return fileName == rhs.fileName && 
		       funcName == rhs.funcName && 
		       line == rhs.line &&
		       varName == rhs.varName;
	}

	inline bool operator!= (const Location& rhs) const {
		return !(*this==rhs);
	}

	/// Enable compare operator to avoid duplicated bugs insertion in map or set
	/// to be noted that two vectors can also overload operator()
	inline bool operator< (const Location& rhs) const {
		if(fileName != rhs.fileName)
			return fileName < rhs.fileName;
		else if(funcName != rhs.funcName)
			return funcName < rhs.funcName;
	       	else if (line != rhs.line)
			return line < rhs.line;
		else
			return varName < rhs.varName;
	}

	friend std::istream& operator>> (std::istream& is, Location &loc);
	friend std::ostream& operator<< (std::ostream& os, const Location &loc);
private:
	std::string varName;
	std::string fileName;
	std::string funcName;
	u32_t line;
};


struct BugCmp;

/**
 * Abstract bug object.
 */
class Bug {
public:
	typedef std::set<const Bug*, BugCmp> BugSet;

	enum BUG_TYPE {
		MEM_LEAK,
		USE_AFTER_FREE,
		USE_AFTER_RETURN,
		DOUBLE_LOCK,
		DOUBLE_FREE
	};

	Bug(BUG_TYPE ty): type(ty), time(0.0)  { }

	virtual ~Bug() { }

	virtual std::string toString() const = 0;

	virtual std::string getTypeName() const = 0;

	/**
	 * Fills the maps with the name of the functions (visited during the analysis) 
	 * and the correspoding location of the functions.
	 */
	void setVisitedFuncToLocMap(StringToLoc &map) {
		visitedFuncToLocMap = map;
	}

	/**
	 * Set the API function path to the source.
	 */
	void setAPIPath(const StringList &path) {
		apiPath = path;
	}

	/**
	 * Set the duration of the analysis that found the bug.
	 */
	void setDuration(double t) {
		time = t;
	}

	/**
	 * Get the duration of the analysis that found the bug.
	 */
	double getDuration() const {
		return time;
	}

	/**
	 * Get the API function path to the source.
	 */
	const StringList&  getAPIPath() const {
		return apiPath;
	}

	BUG_TYPE getBugType() const {
		return type;
	}

	virtual bool less_cmp(const Bug *bug) const = 0;

	/// Enable compare operator to avoid duplicated bugs insertion in map or set
	/// to be noted that two vectors can also overload operator()
	virtual bool operator< (const Bug& rhs) const {
		return type < rhs.type;
	}

	static inline bool classof(const Bug*) {
		return true;
	}

protected:
	// Maps all visited functions (names) to their locations.
	StringToLoc visitedFuncToLocMap;

	// Path from the systemcall to the local variable.
	StringList apiPath;

	// The time it took for the analysis to find this bug.
	double time;
private:
	BUG_TYPE type;
};

/**
 * Contains the information of an use-after-return bug.
 */
class UARBug : public Bug {
public:
	typedef std::set<const UARBug*> BugSet;
	typedef std::pair<NodeID, NodeID> CxtNodeIdVar;
	typedef std::list<CxtNodeIdVar> CxtNodeIdList;

	UARBug(): Bug(BUG_TYPE::USE_AFTER_RETURN), counter(0) { }

	virtual ~UARBug() { }

	/**
	 * Get the name of the dangling pointer.
	 */
	std::string getDanglingPtrName() const {
		return danglingPtrLoc.getVarName();
	}

	/**
	 * Get the name of the local variable.
	 */
	std::string getLocalVarName() const {
		return localVarLoc.getVarName();
	}

	/**
	 * Set the location of the dangling pointer.
	 */
	void setDanglingPtrLocation(std::string fileName, std::string funcName, unsigned int line) {
		danglingPtrLoc = Location(fileName, funcName, line);
	}

	/**
	 * Set the location of the dangling pointer.
	 */
	void setDanglingPtrLocation(Location loc) {
		danglingPtrLoc = loc; 
	}

	/**
	 * Get the location of the dangling ponter.
	 */
	const Location& getDanglingPtrLocation() const {
		return danglingPtrLoc;
	}

	/**
	 * Set the location of the local variable.
	 */
	void setLocalVarLocation(std::string fileName, std::string funcName, unsigned int line) {
		localVarLoc = Location(fileName, funcName, line);
	}

	/**
	 * Set the location of the local variable.
	 */
	void setLocalVarLocation(Location loc) {
		localVarLoc = loc;
	}

	/**
	 * Get the location of the local variable.
	 */
	const Location& getLocalVarLocation() const {
		return localVarLoc;
	}

	/**
	 * Set the function path of the local variable to the
	 * the first function out of scope.
	 */
	void setLocalVarFuncPathStr(std::string funcPath) {
		localVarFuncPathStr = funcPath;
	}

	/**
	 * Get the function path of the local variable to the
	 * the first function out of scope.
	 */
	std::string getLocalVarFuncPathStr() const {
		return localVarFuncPathStr;
	}

	/**
	 * Set the function path of the dangling pointer to the
	 * the first function out of scope.
	 */
	void setDanglingPtrFuncPathStr(std::string funcPath) {
		danglingPtrFuncPathStr = funcPath;
	}

	/**
	 * Get the function path of the dangling pointer to the
	 * the first function out of scope.
	 */
	std::string getDanglingPtrFuncPathStr() const {
		return danglingPtrFuncPathStr;
	}

	virtual std::string getTypeName() const {
		return "Use-After-Return";
	}

	virtual std::string toString() const {
		std::stringstream rawBug;

		rawBug << "======================================================\n";
		rawBug << "______________________________________________________\n";
		rawBug << "DANGLING POINTER:\n\n" << danglingPtrLoc.toString() << "\n";
		rawBug << "______________________________________________________\n";
		rawBug << "LOCAL VARIABLE:\n\n" << localVarLoc.toString() << "\n";
		rawBug << "______________________________________________________\n";
		rawBug << "API TO LOCALVAR:\n\n";

		for(auto iter = apiPath.rbegin(); iter != apiPath.rend(); ++iter)
			rawBug << "-> " << *iter << "\n";
		rawBug << "\n";

		rawBug << "______________________________________________________\n";
		rawBug << "LOCALVAR TO OUTOFSCOPE:\n\n";

		if(localVarFuncPathStr.size() > 1)
			rawBug << localVarFuncPathStr << "\n";

		rawBug << "______________________________________________________\n";
		rawBug << "DANGLINGPTR TO OUTOFSCOPE:\n\n";

		if(danglingPtrFuncPathStr.size() > 1)
			rawBug << danglingPtrFuncPathStr << "\n";


		rawBug << "______________________________________________________\n";
		rawBug << "FUNCTION-LOCATION MAP:\n\n";

		for(auto iter = visitedFuncToLocMap.begin(); iter != visitedFuncToLocMap.end(); ++iter) {
			rawBug << " - " << iter->first << std::setw(25-iter->first.size()) << " " 
			       << iter->second.getFileName() 
			       << " (ln: " << iter->second.getLine() << ")\n" << std::setw(0);
		}

		rawBug << "______________________________________________________\n";

		if(time < 0)
			rawBug << "Duration (sec)" << std::setw(30) << " " << "timeout\n";
		else
			rawBug << "Duration (sec)" << std::setw(40-NUMDIGITS((int)time)) << " " << (int)time << "\n";

		rawBug << "======================================================\n";

		return rawBug.str();
	}

	/// Enable compare operator to avoid duplicated bugs insertion in map or set
	/// to be noted that two vectors can also overload operator()
	bool operator< (const UARBug& rhs) const {
		if(localVarLoc != rhs.localVarLoc)
			return localVarLoc < rhs.localVarLoc;
		else
			return danglingPtrLoc < rhs.danglingPtrLoc;
	}

	bool operator!= (const UARBug& rhs) const {
		return localVarLoc != rhs.localVarLoc || danglingPtrLoc != rhs.danglingPtrLoc;
	}

	bool operator== (const UARBug & rhs) const {
		return !(*this!=rhs);
	}

	virtual bool less_cmp(const Bug *bug) const {
		const UARBug &rhs = *llvm::dyn_cast<UARBug>(bug);
		return *this < rhs;
	}

	static inline bool classof(const UARBug *) {
		return true;
	}

	static inline bool classof(const Bug *bug) {
		return bug->getBugType() == BUG_TYPE::USE_AFTER_RETURN;
	}

	friend std::istream& operator>> (std::istream& is, UARBug &bug);
	friend std::ostream& operator<< (std::ostream& os, const UARBug &bug);

private:

	Location localVarLoc;

	Location danglingPtrLoc;

	// The path from the dangling ptr as a printable string.
	std::string danglingPtrFuncPathStr;

	// The path from the local variable as a printable string.
	std::string localVarFuncPathStr;

	// How often this bug was found.
	u32_t counter;
};

class MemLeakBug : public Bug {
public:
	typedef std::set<const MemLeakBug*> BugSet;

	enum MEM_LEAK_TYPE {
		NEVER_FREE,
		PARTIAL_MEM_LEAK,
	};

	MemLeakBug(MEM_LEAK_TYPE lTy) : Bug(BUG_TYPE::MEM_LEAK), type(lTy) { }

	virtual ~MemLeakBug() { }

	/**
	 * Set the location.
	 */
	void setSourceLocation(std::string fileName, std::string funcName, unsigned int line, std::string name) {
		sourceLoc = Location(fileName, funcName, line, name);
	}

	/**
	 * Get the location of the local variable that cause the bug.
	 */
	const Location& getSourceLocation() const {
		return sourceLoc;
	}

	virtual std::string toString() const {
		std::stringstream rawBug;
		rawBug << "======================================================\n";
		rawBug << "______________________________________________________\n";
		rawBug << "Never Free at:\n\n" << sourceLoc.toString() << "\n"; 
		rawBug << "______________________________________________________\n";
		rawBug << "API TO SOURCE:\n\n";

		for(auto iter = apiPath.rbegin(); iter != apiPath.rend(); ++iter)
			rawBug << "-> " << *iter << "\n";
		rawBug << "\n";

		rawBug << "______________________________________________________\n";
		rawBug << "Duration (sec)" << std::setw(40-NUMDIGITS((int)time)) << " " << (int)time << "\n";
		rawBug << "======================================================\n";


		return rawBug.str();
	}

	std::string toOnlinerString() const {
		std::stringstream rawBug;
		rawBug << getSourceLocation().toOnlinerString() << "\n";
		return rawBug.str();
	}

	virtual std::string getTypeName() const {
		return "Memory-Leak";
	}

	MEM_LEAK_TYPE getLeakType() const {
		return type;
	}

	/// Enable compare operator to avoid duplicated bugs insertion in map or set
	/// to be noted that two vectors can also overload operator()
	bool operator< (const MemLeakBug& rhs) const {
		if(sourceLoc != rhs.sourceLoc)
			return sourceLoc < rhs.sourceLoc;

		return false;
	}

	bool operator!= (const MemLeakBug& rhs) const {
		return !(*this==rhs);
	}

	bool operator== (const MemLeakBug& rhs) const {
		return sourceLoc == rhs.sourceLoc;
	}

	virtual bool less_cmp(const Bug *bug) const {
		const MemLeakBug &rhs = *llvm::dyn_cast<MemLeakBug>(bug);
		return *this < rhs;
	}

	static inline bool classof(const MemLeakBug *) {
		return true;
	}

	static inline bool classof(const Bug *bug) {
		return bug->getBugType() == BUG_TYPE::MEM_LEAK;
	}

protected:
	MEM_LEAK_TYPE type;
	Location sourceLoc;
};

class NeverFreeBug : public MemLeakBug {
public:
	typedef std::set<const NeverFreeBug*> BugSet;

	NeverFreeBug() : MemLeakBug(MEM_LEAK_TYPE::NEVER_FREE) { }

	virtual ~NeverFreeBug() { }

	static inline bool classof(const NeverFreeBug *) {
		return true;
	}

	static inline bool classof(const MemLeakBug *bug) {
		return bug->getLeakType() == MEM_LEAK_TYPE::NEVER_FREE;
	}
};

class PartialMemLeakBug : public MemLeakBug {
public:
	typedef std::set<const PartialMemLeakBug*> BugSet;

	PartialMemLeakBug() : MemLeakBug(MEM_LEAK_TYPE::PARTIAL_MEM_LEAK) { }

	virtual ~PartialMemLeakBug() { }

	void setCondFreePath(std::string path) {
		condFreePath = path;
	}

	virtual std::string toString() const {
		std::stringstream rawBug;

		rawBug << "======================================================\n";
		rawBug << "______________________________________________________\n";
		rawBug << "Partial Leak at: \n" << sourceLoc.toString() << "\n"; 
		rawBug << "______________________________________________________\n";
		rawBug << "conditional free path:\n" << condFreePath << "\n"; 

		return rawBug.str();
	}

	static inline bool classof(const PartialMemLeakBug *) {
		return true;
	}

	static inline bool classof(const MemLeakBug *bug) {
		return bug->getLeakType() == MEM_LEAK_TYPE::PARTIAL_MEM_LEAK;
	}

private:
	std::string condFreePath;
};


class DoubleSinkBug : public Bug {
public:
	typedef std::set<const DoubleSinkBug*> BugSet;

	DoubleSinkBug(BUG_TYPE ty): Bug(ty) { }

	virtual ~DoubleSinkBug() { }

	/**
	 * Set the location of the sink.
	 */
	void setSourceLocation(std::string fileName, 
			       std::string funcName, 
			       unsigned int line, 
			       std::string varName) {
		sourceLoc = Location(fileName, funcName, line, varName);
	}

	/**
	 * Set the location of the first sink function.
	 */
	void setSink1Location(std::string fileName, std::string funcName, unsigned int line) {
		sink1Loc = Location(fileName, funcName, line);
	}

	/**
	 * Set the location of the second sink function.
	 */
	void setSink2Location(std::string fileName, std::string funcName, unsigned int line) {
		sink2Loc = Location(fileName, funcName, line);
	}

	/**
	 * Get the location of the source.
	 */
	const Location& getSourceLocation() const {
		return sourceLoc;
	}

	/**
	 * Get the location of the first sink functions.
	 */
	const Location& getSink1Location() const {
		return sink1Loc;
	}

	/**
	 * Get the location of the second sink functions.
	 */
	const Location& getSink2Location() const {
		return sink2Loc;
	}

	/**
	 * Set the function path of the first sink function.
	 */
	void setSink1PathStr(std::string path) {
		sink1PathStr = path;
	}

	/**
	 * Get the function path of the first sink function.
	 */
	std::string getSink1PathStr() const {
		return sink1PathStr;
	}

	/**
	 * Set the function path of the second sink function.
	 */
	void setSink2PathStr(std::string path) {
		sink2PathStr = path;
	}

	/**
	 * Get the function path of the second sink function.
	 */
	std::string getSink2PathStr() const {
		return sink2PathStr;
	}

	virtual std::string getTypeName() const = 0;
	

	virtual std::string toString() const {
		std::stringstream rawBug;

		rawBug << "======================================================\n";
		rawBug << "______________________________________________________\n";
		rawBug << "SOURCE:\n\n" << sourceLoc.toString() << "\n";
		rawBug << "______________________________________________________\n";
		rawBug << "FIRST SINK:\n\n" << sink1Loc.toString() << "\n";
		rawBug << "______________________________________________________\n";
		rawBug << "SECOND SINK:\n\n" << sink2Loc.toString() << "\n";
		rawBug << "______________________________________________________\n";
		rawBug << "API TO SOURCE:\n\n";

		for(auto iter = apiPath.rbegin(); iter != apiPath.rend(); ++iter)
			rawBug << "-> " << *iter << "\n";
		rawBug << "\n";

		rawBug << "______________________________________________________\n";
		rawBug << "PATH TO FIRST SINK:\n\n";

		if(sink1PathStr.size() > 1)
			rawBug << sink1PathStr << "\n";

		rawBug << "______________________________________________________\n";
		rawBug << "PATH TO SECOND SINK:\n\n";

		if(sink2PathStr.size() > 1)
			rawBug << sink2PathStr << "\n";

		rawBug << "______________________________________________________\n";
		rawBug << "FUNCTION-LOCATION MAP:\n\n";

		for(auto iter = visitedFuncToLocMap.begin(); iter != visitedFuncToLocMap.end(); ++iter) {
			rawBug << " - " << iter->first << std::setw(25-iter->first.size()) << " " 
			       << iter->second.getFileName() 
			       << " (ln: " << iter->second.getLine() << ")\n" << std::setw(0);
		}

		rawBug << "______________________________________________________\n";
		rawBug << "Duration (sec)" << std::setw(40-NUMDIGITS((int)time)) << " " << (int)time << "\n";
		rawBug << "======================================================\n";

		return rawBug.str();
	}

	std::string toOnlinerString() const {
		std::stringstream rawBug;
		rawBug << getSourceLocation().toOnlinerString() << "\n";
		return rawBug.str();
	}


	/// Enable compare operator to avoid duplicated bugs insertion in map or set
	/// to be noted that two vectors can also overload operator()
	bool operator< (const DoubleSinkBug& rhs) const {
//		if(sourceLoc != rhs.sourceLoc)
//			return sourceLoc < rhs.sourceLoc;
		if(sink1Loc != rhs.sink1Loc)
			return sink1Loc < rhs.sink1Loc;
		else
			return sink2Loc < rhs.sink2Loc;
	}

	bool operator!= (const DoubleSinkBug& rhs) const {
		return !(*this==rhs);
	}

	bool operator== (const DoubleSinkBug& rhs) const {
		return sourceLoc == rhs.sourceLoc && sink1Loc == rhs.sink1Loc && sink2Loc == rhs.sink2Loc;
	}

	virtual bool less_cmp(const Bug *bug) const {
		const DoubleSinkBug &rhs = *llvm::dyn_cast<DoubleSinkBug>(bug);
		return *this < rhs;
	}

	static inline bool classof(const DoubleSinkBug*) {
		return true;
	}

	static inline bool classof(const Bug *bug) {
		return bug->getBugType() == BUG_TYPE::DOUBLE_FREE || 
		       bug->getBugType() == BUG_TYPE::USE_AFTER_FREE || 
		       bug->getBugType() == BUG_TYPE::DOUBLE_LOCK;
	}

private:
	Location sourceLoc;	

	Location sink1Loc;

	Location sink2Loc;

	// The path from the dangling ptr as a printable string.
	std::string sink1PathStr;

	// The path from the local variable as a printable string.
	std::string sink2PathStr;
};

class UseAfterFreeBug : public DoubleSinkBug {
public:
	typedef std::set<const UseAfterFreeBug*> BugSet;

	UseAfterFreeBug() : DoubleSinkBug(BUG_TYPE::USE_AFTER_FREE) { }

	virtual ~UseAfterFreeBug() { }

	virtual std::string getTypeName() const {
		return "Use-After-Free";
	}

	static inline bool classof(const UseAfterFreeBug *) {
		return true;
	}

	static inline bool classof(const DoubleSinkBug *bug) {
		return bug->getBugType() == BUG_TYPE::USE_AFTER_FREE;
	}

private:
};

class DoubleFreeBug : public DoubleSinkBug {
public:
	typedef std::set<const DoubleFreeBug*> BugSet;

	DoubleFreeBug() : DoubleSinkBug(BUG_TYPE::DOUBLE_FREE) { }

	virtual ~DoubleFreeBug() { }

	virtual std::string getTypeName() const {
		return "Double-Free";
	}

	static inline bool classof(const DoubleFreeBug *) {
		return true;
	}

	static inline bool classof(const DoubleSinkBug *bug) {
		return bug->getBugType() == BUG_TYPE::DOUBLE_FREE;
	}

private:
};

class DoubleLockBug : public DoubleSinkBug {
public:
	typedef std::set<const DoubleLockBug*> BugSet;

	DoubleLockBug() : DoubleSinkBug(BUG_TYPE::DOUBLE_LOCK) { }

	virtual ~DoubleLockBug() { }

	virtual std::string getTypeName() const {
		return "Double-Lock";
	}

	static inline bool classof(const DoubleSinkBug *) {
		return true;
	}

	static inline bool classof(const DoubleLockBug *bug) {
		return bug->getBugType() == BUG_TYPE::DOUBLE_LOCK;
	}

private:
};

struct BugCmp {
	bool operator()(const Bug *lhs, const Bug *rhs) const { 
		return lhs->less_cmp(rhs);
	}
};
#endif // BUG_H
