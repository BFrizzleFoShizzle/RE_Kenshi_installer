#ifndef BUGS_H
#define BUGS_H

#include <string>

namespace Bugs
{
	void ReportBug(std::string window, int step, std::string error, std::string logs = "");
};

#endif // BUGS_H
