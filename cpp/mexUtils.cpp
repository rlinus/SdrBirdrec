#include "mexUtils.h"


namespace mexUtils
{

	std::streamsize redirectOstream2mexPrintf::xsputn(const char *s, std::streamsize n)
	{
		mexPrintf("%.*s", n, s);
		return n;
	}

	int redirectOstream2mexPrintf::overflow(int c)
	{
		if (c != EOF) {
			mexPrintf("%.1s", &c);
		}
		return 1;
	}

	std::string getMexFilePath(void) {
		mxArray *rhs[1], *lhs[1];
		char *path, *name;
		size_t lenpath, lenname, n;
		rhs[0] = mxCreateString("fullpath");
		mexCallMATLAB(1, lhs, 1, rhs, "mfilename");
		mxDestroyArray(rhs[0]);
		path = mxArrayToString(lhs[0]);
		mxDestroyArray(lhs[0]);
		mexCallMATLAB(1, lhs, 0, rhs, "mfilename");
		name = mxArrayToString(lhs[0]);
		mxDestroyArray(lhs[0]);

		lenpath = strlen(path);
		lenname = strlen(name);
		n = lenpath - lenname;
		if(n > 0) n -= 1;
		path[n] = '\0';

		//for (int i = 0; i<n; ++i) {
		//	if (path[i] == '\\') path[i] = '/';
		//}

		std::string str(path);
		return str;
	}
};




