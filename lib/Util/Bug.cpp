#include "Util/Bug.h"

using namespace std;
using namespace llvm;

std::istream& operator>> (std::istream& is, Location &loc) {
	is >> loc.fileName;
	is >> loc.funcName;
	is >> loc.line;
	is >> loc.varName;

	return is;
}

std::ostream& operator<< (std::ostream& os, const Location &loc) {
	os << loc.fileName << " ";
	os << loc.funcName << " ";
	os << loc.line << " ";
	os << loc.varName<< " ";

	return os;
}

std::istream& operator>> (std::istream& is, UARBug &bug) {
	int tmpSize = 0;
	is >> bug.localVarLoc;
	is >> bug.danglingPtrLoc;
	is >> tmpSize;

	StringToLoc tmpMap;

	for(int i=0; i < tmpSize; i++) {
		std::string funcName;
		Location loc;
		is >> funcName;
		is >> loc;
		tmpMap[funcName] = loc;
	}

	bug.setVisitedFuncToLocMap(tmpMap);

	is >> bug.counter;
	is >> bug.time;

	return is;
}

std::ostream& operator<< (std::ostream& os, const UARBug &bug) {
	os << bug.localVarLoc;
	os << bug.danglingPtrLoc;
	os << bug.visitedFuncToLocMap.size() << " ";

	for(auto iter = bug.visitedFuncToLocMap.begin(); iter != bug.visitedFuncToLocMap.end(); ++iter) {
		std::string funcName = iter->first;
		Location loc = iter->second;
		os << funcName << " ";
		os << loc;
	}

	os << bug.counter << " ";
	os << bug.time << " ";

	return os;
}
