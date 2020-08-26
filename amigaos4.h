#include "proto/dos.h"
#include <workbench/startup.h>

#include "zgloom_rev.h"

//http://www.os4coding.net/comment/3488#comment-3488
//#include <string>
#include <sstream>
namespace std {
	using ::atoi;

	template <typename T> string to_string(const T &n) {
		ostringstream strm;
		strm << n;
		return strm.str();
	}

	inline int stoi(const string &str, size_t *idx = NULL, int base = 10) {
		const char *start = str.c_str();
		const char *end;
		int n;

		n = ::strtol(start, (char **)&end, base);

		if (idx != NULL)
			*idx = end - start;

		return n;
	}
}

/*
// https://stackoverflow.com/questions/19122574/to-string-isnt-a-member-of-std
#include <sstream>
template <typename T>
std::string ToString(T val)
{
    std::stringstream stream;
    stream << val;
    return stream.str();
}
*/
