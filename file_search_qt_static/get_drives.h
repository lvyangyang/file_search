#ifndef GET_DRIVES_H
#define GET_DRIVES_H
#include <set>
#include "windows.h"
class get_drives
{
public:
	get_drives();
	std::set<char> gets();
private:
	BOOL is_ntfs(LPWSTR szDrive);
};

#endif // GET_DRIVES_H
