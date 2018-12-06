#include "get_drives.h"

static BOOL WCharToMByte(LPCWSTR lpcwszStr, std::string &str)
{
	str.clear();
	DWORD dwMinSize = 0;
	LPSTR lpszStr = NULL;
	dwMinSize = WideCharToMultiByte(CP_OEMCP, NULL, lpcwszStr, -1, NULL, 0, NULL, FALSE);
	if (0 == dwMinSize)
	{
		return FALSE;
	}
	lpszStr = new char[dwMinSize];
	WideCharToMultiByte(CP_OEMCP, NULL, lpcwszStr, -1, lpszStr, dwMinSize, NULL, FALSE);
	str = lpszStr;
	delete[] lpszStr;
	lpszStr = NULL;
	return TRUE;
}

get_drives::get_drives()
{

}
BOOL get_drives::is_ntfs(LPWSTR szDrive) {
	UINT uDriveType;   //驱动器类型
	DWORD dwVolumeSerialNumber;     //卷序列号
	DWORD dwMaximumComponentLength;  //最大组件长度
	DWORD dwFileSystemFlags;   //文件系统标识
	TCHAR szFileSystemNameBuffer[1024];  //文件系统名称缓冲
	TCHAR szDirverName[MAX_PATH]; //驱动器名称
								  //最大字符长度的路径。
	std::string name;
	uDriveType = GetDriveType(szDrive);  //获取驱动器的物理类型  该函数返回驱动器类型
	if (uDriveType != DRIVE_FIXED&&uDriveType != DRIVE_REMOVABLE)
		return FALSE;

	if (!GetVolumeInformation(szDrive, szDirverName, MAX_PATH, &dwVolumeSerialNumber, &dwMaximumComponentLength,
		&dwFileSystemFlags, szFileSystemNameBuffer, 1024)) {    //获取驱动器的相关属性
		return FALSE;
	}
	WCharToMByte(szFileSystemNameBuffer, name);
	if (name == "NTFS")
		return TRUE;
	else
		return FALSE;
}

std::set<char> get_drives:: gets() {
	TCHAR szLogicDriveStrings[1024];
	TCHAR *szDrive;
	std::set<char> driver_letters;
	std::string driver_string;

	ZeroMemory(szLogicDriveStrings, 1024);

	GetLogicalDriveStrings(1024 - 1, szLogicDriveStrings);
	szDrive = szLogicDriveStrings;

	do
	{
		if (is_ntfs(szDrive))
		{
			WCharToMByte(szDrive, driver_string);
			driver_letters.insert(driver_string.data()[0]);
		}
		szDrive += (lstrlen(szDrive) + 1);
	} while (*szDrive != '\x00');
	return driver_letters;
}


