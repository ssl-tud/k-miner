#include "KernelModels/Initcall.h"

void getStreamOfStringSet(const StringSet &set, std::ostream &os) {
	os << set.size() << "\n";
	
	for(auto iter : set) {
		os << iter << "\n";
	}
}

void getStringSetOfStream(std::istream &is, StringSet &set) {
	uint32_t size = 0;

	is >> size;

	for(int i=0; i < size; i++) {
		std::string tmpStr;
		is >> tmpStr;
		set.insert(tmpStr);	
	}
}

std::istream& operator>> (std::istream& is, Initcall &initcall) {
	is >> initcall.name;
	is >> initcall.level;
	getStringSetOfStream(is, initcall.functions);
	getStringSetOfStream(is, initcall.globalvars);
	getStringSetOfStream(is, initcall.nondefvars);
	is >> initcall.maxCGDepth;

	return is;
}

std::ostream& operator<< (std::ostream& os, const Initcall &initcall) {
	os << initcall.name << "\n";
	os << initcall.level << "\n";
	getStreamOfStringSet(initcall.getFunctions(), os);
	getStreamOfStringSet(initcall.getGlobalVars(), os); 
	getStreamOfStringSet(initcall.getNonDefVars(), os); 
	os << initcall.maxCGDepth << "\n";

	return os;
}

