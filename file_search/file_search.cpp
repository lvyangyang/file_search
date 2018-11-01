// sqlite_test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>
#include "sqlite3.h"
#include <iostream>
#include <sstream>
#include <string>        
#include <io.h>
#include <atlstr.h>
//#include <winioctl.h>
#include<windows.h>
#include<map>
#include<vector>
#include "ChangeJrnl.h"

#define MAX_DIR_LENGTH 4096
using namespace std;

BOOL WCharToMByte(LPCWSTR lpcwszStr, std::string &str)
{
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


struct file_info
{
	string filename;
	DWORDLONG parent_frn;
	DWORD attrib;
};

int file_count = 0;

DWORDLONG get_frn(LPCTSTR pszPath, LPCTSTR pszFile, bool &error)
{
	TCHAR szTempRecursePath[MAX_DIR_LENGTH];
	lstrcpy(szTempRecursePath, pszPath);
	lstrcat(szTempRecursePath, TEXT("\\"));
	lstrcat(szTempRecursePath, pszFile);

	HANDLE hDir = CreateFile(szTempRecursePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (INVALID_HANDLE_VALUE == hDir) {
		cout << "error" << endl;
		error = true;
	}
	BY_HANDLE_FILE_INFORMATION fi;
	GetFileInformationByHandle(hDir, &fi);
	CloseHandle(hDir);

	LARGE_INTEGER index;
	index.LowPart = fi.nFileIndexLow;
	index.HighPart = fi.nFileIndexHigh;
	return index.QuadPart;
}

void RecursePath(LPCTSTR pszPath, LPCTSTR pszFile,
	DWORDLONG ParentIndex, DWORD dwVolumeSerialNumber, 
	map<DWORDLONG, file_info> &fileinfo_db, 
	map<DWORDLONG, file_info> &dirinfo_db) 
{

	// Enumerate the directores in the current path and store the file reference
	// numbers in the database. Call this function recursively to enumerate into
	// the subdirectores.
	file_info temp_fileinfo;
	CString szPath2(pszPath);
	szPath2 += TEXT("\\");
	HANDLE hDir = CreateFile(szPath2, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

	// NOTE: If we cannot get a handle to this, there is no need to recurs into
	// it (even though it might be possible). Example - we couldn't open it, but
	// FindFirst(...) succeeds for some reason. We don't care about any extra
	// information we can get by FindFirst inside this directory since we won't
	// have the FileIndex of this directory. Therefore, any objects under this
	// directory will not be able to have their names fully resolved (ie: when
	// we walk up the parent id's for an object below this one, we won't be able
	// to find this id)
	if (INVALID_HANDLE_VALUE == hDir) {
		return;
	}

	BY_HANDLE_FILE_INFORMATION fi;
	GetFileInformationByHandle(hDir, &fi);
	CloseHandle(hDir);

	if (0 == dwVolumeSerialNumber) {
		dwVolumeSerialNumber = fi.dwVolumeSerialNumber;
	}

	if (fi.nNumberOfLinks != 1) {
		//ASSERT(FALSE);
	}

	if (dwVolumeSerialNumber != fi.dwVolumeSerialNumber) {
		// We've wondered onto another volume - This should only
		// happen if we recursed into a reparse point, but we check
		// for that below!
		//ASSERT(FALSE);
		return;
	}

	LARGE_INTEGER index;
	index.LowPart = fi.nFileIndexLow;
	index.HighPart = fi.nFileIndexHigh;
	if (dirinfo_db.find(index.QuadPart) != dirinfo_db.end())
		return;
	//dir_set[index.QuadPart] = true;
	//收集文件信息
	temp_fileinfo.attrib = fi.dwFileAttributes;
	//temp_fileinfo.frn = index.QuadPart;
	temp_fileinfo.parent_frn = ParentIndex;
	//WCharToMByte(szPath2, temp_fileinfo.path);
	WCharToMByte(pszFile, temp_fileinfo.filename);
	dirinfo_db[index.QuadPart] = temp_fileinfo;
	/*
	if (!Add(index.QuadPart, pszFile, ParentIndex)) {
	// Duplicate - The FRN for this directory was already
	// in our database. This shouldn't happen, but if it
	// does, there's no need to recurse into sub-directories
	return;
	}
	*/
	TCHAR szTempPath[MAX_DIR_LENGTH];
	lstrcpy(szTempPath, pszPath);
	lstrcat(szTempPath, TEXT("\\*.*"));

	WIN32_FIND_DATA fd;
	HANDLE hSearch = FindFirstFile(szTempPath, &fd);
	if (INVALID_HANDLE_VALUE == hSearch) {
		// Something went wrong trying to enumerate the files/directories
		// in the current directory
		return;
	}

	do {

		TCHAR szTempRecursePath[MAX_DIR_LENGTH];
		lstrcpy(szTempRecursePath, pszPath);
		lstrcat(szTempRecursePath, TEXT("\\"));
		lstrcat(szTempRecursePath, fd.cFileName);
		if ((0 != (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			&& (0 != lstrcmp(fd.cFileName, TEXT(".")))
			&& (0 != lstrcmp(fd.cFileName, TEXT("..")))
			&& (0 == (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))) {
			//	_tprintf( szTempRecursePath);
			//	cout << endl;
			RecursePath(szTempRecursePath, fd.cFileName, index.QuadPart,
				dwVolumeSerialNumber, fileinfo_db, dirinfo_db);
		}
		else
		{
			if (0 == (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				//_tprintf(szTempRecursePath);
				//cout << endl;
				bool error = FALSE;
				WCharToMByte(fd.cFileName, temp_fileinfo.filename);
				temp_fileinfo.attrib = fd.dwFileAttributes;
				DWORDLONG temp_frn = get_frn(pszPath, fd.cFileName, error);
				if (error)
					continue;
				file_count++;
				fileinfo_db[temp_frn] = temp_fileinfo;
			}


		}

	} while (FindNextFile(hSearch, &fd) != FALSE);

	FindClose(hSearch);
}

void recurse_record(const char drive_letter)
{
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	stringstream ssm;  
	ssm<< drive_letter <<"_"<<"file_search.db";
	rc = sqlite3_open(ssm.str().c_str(), &db);

	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else {
		fprintf(stderr, "Opened database successfully\n");
	}
	//创建数据表
	sqlite3_exec(db, "PRAGMA synchronous = OFF; ", 0, 0, 0);
	sqlite3_exec(db, "drop table if exists file_info", 0, 0, 0);
	sqlite3_exec(db, "create table file_info(frn integer,parentfrn integer,attrib integer ,name text)", 0, 0, 0);
	sqlite3_exec(db, "drop table if exists dir_info", 0, 0, 0);
	sqlite3_exec(db, "create table dir_info(frn integer,parentfrn integer,attrib integer ,name text)", 0, 0, 0);

	//将信息收集到内存
	map<DWORDLONG, file_info> fileinfo_db;
	map<DWORDLONG, file_info> dirinfo_db;
	TCHAR szCurrentPath[MAX_DIR_LENGTH];
	wsprintf(szCurrentPath, TEXT("%c:"), drive_letter);
	//wsprintf(szCurrentPath, TEXT("%s:"), "c:\\Users\\fenhuo\\Documents\\Visual Studio 2015\\Projects");
	lstrcat(szCurrentPath, TEXT("\\Users\\fenhuo\\Documents\\Visual Studio 2015\\Projects"));
	RecursePath(szCurrentPath, szCurrentPath, 0, 0, fileinfo_db, dirinfo_db);

	//插入文件信息
	sqlite3_exec(db, "begin;", 0, 0, 0);
	sqlite3_stmt *stmt_file;
	const char* sql_file = "insert into file_info values(?,?,?,?)";
	sqlite3_prepare_v2(db, sql_file, strlen(sql_file), &stmt_file, 0);
	map<DWORDLONG, file_info>::iterator iter;
	iter = fileinfo_db.begin();
	while (iter != fileinfo_db.end())
	{
		sqlite3_reset(stmt_file);

		sqlite3_bind_int64(stmt_file, 1, iter->first);
		sqlite3_bind_int64(stmt_file, 2, iter->second.parent_frn);
		sqlite3_bind_int64(stmt_file, 4, iter->second.attrib);
		sqlite3_bind_text(stmt_file, 3, iter->second.filename.data(), -1, SQLITE_STATIC);

		sqlite3_step(stmt_file);
		iter++;
	}
	sqlite3_finalize(stmt_file);
	sqlite3_exec(db, "commit;", 0, 0, 0);

	//插入目录信息
	sqlite3_exec(db, "begin;", 0, 0, 0);
	sqlite3_stmt *stmt_dir;
	const char* sql_dir = "insert into dir_info values(?,?,?,?)";
	sqlite3_prepare_v2(db, sql_dir, strlen(sql_dir), &stmt_dir, 0);

	iter = dirinfo_db.begin();
	while (iter != dirinfo_db.end())
	{
		sqlite3_reset(stmt_dir);
		sqlite3_bind_int64(stmt_dir, 1, iter->first);
		sqlite3_bind_int64(stmt_dir, 2, iter->second.parent_frn);
		sqlite3_bind_int64(stmt_dir, 4, iter->second.attrib);
		sqlite3_bind_text(stmt_dir, 3, iter->second.filename.data(), -1, SQLITE_STATIC);

		sqlite3_step(stmt_dir);
		iter++;
	}
	sqlite3_finalize(stmt_dir);
	sqlite3_exec(db, "commit;", 0, 0, 0);

	cout << file_count;
	sqlite3_close(db);
}

void InitialzeForMonitoring(CChangeJrnl &m_cj) {

	// This function ensures that the journal on the volume is active, and it
	// will also populate the directory database.
	BOOL fOk = TRUE;
	USN_JOURNAL_DATA ujd;

	// Try to query for current journal information
	while (fOk && !m_cj.Query(&ujd)) {
		switch (GetLastError()) {
		case ERROR_JOURNAL_DELETE_IN_PROGRESS:
			// The system is deleting a journal. We need to wait for it to finish
			// before trying to query it again.
			m_cj.Delete(0, USN_DELETE_FLAG_NOTIFY);
			break;

		case ERROR_JOURNAL_NOT_ACTIVE:
			// The journal is not active on the volume. We need to create it and
			// then query for its information again
			m_cj.Create(0x800000, 0x100000);
			break;

		default:
			// Some other error happened while querying the journal information.
			// There is nothing we can do from here
			fOk = FALSE;
			break;
		}
	}
	if (!fOk) {
		// We were not able to query the volume for journal information
		return;
	}

	// Start processing records at the start of the journal
	m_cj.SeekToUsn(ujd.FirstUsn, 0xFFFFFFFF, FALSE, ujd.UsnJournalID);
}

void init_changejrn(CChangeJrnl &m_cj ,char drive_letter)
{
	TCHAR path=drive_letter;
	m_cj.Init(path, 10000);
	InitialzeForMonitoring(m_cj);
	
}

void initial_file_db(CChangeJrnl &m_cj, const char drive_letter)
{
	init_changejrn( m_cj,  drive_letter);
	
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	stringstream ssm;
	ssm << drive_letter << "_" << "file_search.db";
	rc = sqlite3_open(ssm.str().c_str(), &db);

	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else {
		fprintf(stderr, "Opened database successfully\n");
	}
	//创建数据表
	bool has_check_point=false;
	int id;
	USN current_usn;
	DWORDLONG current_journal_id;
	sqlite3_exec(db, "PRAGMA synchronous = OFF; ", 0, 0, 0);
	sqlite3_exec(db, "create table if not exists usn_check_point(id integer,usn integer,usn_journal_id integer)", 0, 0, 0);
	sqlite3_stmt * stmt = NULL;
	const char *zTail;
		if (sqlite3_prepare_v2(db,
			"SELECT * FROM usn_check_point;", -1, &stmt, &zTail) == SQLITE_OK) {
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				 id= sqlite3_column_int(stmt, 0);
				 current_usn = sqlite3_column_int64(stmt, 1);
				 current_journal_id = sqlite3_column_int64(stmt, 2);
				has_check_point = true;
			}
		}
	sqlite3_finalize(stmt);
	//关闭数据库 
	sqlite3_close(db);
	if (has_check_point)
	{
		if (current_journal_id == m_cj.m_rujd.UsnJournalID)
		{
			m_cj.SeekToUsn(current_usn, 0xFFFFFFFF, FALSE, current_journal_id);
			return;
		}

	}
	//journal_id过期或第一次进入程序
	PUSN_RECORD pRecord;
	while (true)
	{
		while (pRecord = m_cj.EnumNext())
		{
		};
		BOOL fOk = (S_OK == GetLastError());
		if (fOk)
		{
			break;
		}
		else
		{
			InitialzeForMonitoring(m_cj);
		}

	}
	recurse_record(drive_letter);
	//更新usn_check_point
	//char *zErrMsg = 0;
	rc = sqlite3_open(ssm.str().c_str(), &db);
	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else {
		fprintf(stderr, "Opened database successfully\n");
	}
	sqlite3_exec(db, "delete from usn_check_point where id=0;", 0, 0, &zErrMsg);
	ssm.clear();
	ssm.str("");
	ssm << "insert into usn_check_point values(" << "0" << "," << m_cj.m_rujd.StartUsn << "," << m_cj.m_rujd.UsnJournalID << ");";
	sqlite3_exec(db, ssm.str().c_str(), 0, 0, &zErrMsg);
	sqlite3_close(db);
}

void updating_db(CChangeJrnl &m_cj, const char drive_letter)
{
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	stringstream ssm;
	string filename;
	ssm << drive_letter << "_" << "file_search.db";
	rc = sqlite3_open(ssm.str().c_str(), &db);

	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return;
	}
	else {
		fprintf(stderr, "Opened database successfully\n");
	}
	sqlite3_exec(db, "PRAGMA synchronous = OFF; ", 0, 0, 0);
	

	PUSN_RECORD pRecord;
	while (true)
	{
		//准备更新数据库
		sqlite3_exec(db, "begin;", 0, 0, 0);
		// Use EnumNext to loop through available records
		while (pRecord = m_cj.EnumNext()) {
			// Create a zero terminated copy of the filename
			WCHAR szFile[MAX_PATH];
			LPWSTR pszFileName = (LPWSTR)
				((PBYTE)pRecord + pRecord->FileNameOffset);
			int cFileName = pRecord->FileNameLength / sizeof(WCHAR);
			wcsncpy_s(szFile, pszFileName, cFileName);
			szFile[cFileName] = 0;

			// If this is a close record for a directory, we may need to adjust 
			// our directory database
			if (0 != (pRecord->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				&& 0 != (pRecord->Reason & USN_REASON_CLOSE)) {

				// Process newly created directories
				if (0 != (pRecord->Reason & USN_REASON_FILE_CREATE)) {
					//	g_Globals.m_db.Add(pRecord->FileReferenceNumber, szFile,pRecord->ParentFileReferenceNumber);
					ssm.clear();
					ssm.str("");
					filename.clear();
					WCharToMByte(szFile, filename);
					ssm << "insert into dir_info values(" << pRecord->FileReferenceNumber << ","
						<< pRecord->ParentFileReferenceNumber << "," << pRecord->FileAttributes << ",'" << filename.data() << "')";
					sqlite3_exec(db, ssm.str().c_str(), 0, 0, &zErrMsg);
					Sleep(1);
				}

				// Process renamed directories
				if (0 != (pRecord->Reason & USN_REASON_RENAME_NEW_NAME)) {
					//g_Globals.m_db.Change(pRecord->FileReferenceNumber, szFile,pRecord->ParentFileReferenceNumber);
					ssm.clear();
					ssm.str("");
					filename.clear();
					WCharToMByte(szFile, filename);
					ssm << "update dir_info set name='" << filename.data() << "' where frn=" << pRecord->FileReferenceNumber;
					sqlite3_exec(db, ssm.str().c_str(), 0, 0, &zErrMsg);
					Sleep(1);
				}

				// Process deleted directories
				if (0 != (pRecord->Reason & USN_REASON_FILE_DELETE)) {
					//g_Globals.m_db.Delete(pRecord->FileReferenceNumber);
					ssm.clear();
					ssm.str("");
					ssm << "delete from dir_info where frn=" << pRecord->FileReferenceNumber;
					sqlite3_exec(db, ssm.str().c_str(), 0, 0, &zErrMsg);
					Sleep(1);
				}
			}
			else {
				// Process newly created directories
				if (0 != (pRecord->Reason & USN_REASON_FILE_CREATE)) {
					//	g_Globals.m_db.Add(pRecord->FileReferenceNumber, szFile,pRecord->ParentFileReferenceNumber);
					ssm.clear();
					ssm.str("");
					filename.clear();
					WCharToMByte(szFile, filename);
					ssm << "insert into file_info values(" << pRecord->FileReferenceNumber << ","
						<< pRecord->ParentFileReferenceNumber << "," << pRecord->FileAttributes << ",'" << filename.data() << "')";
					sqlite3_exec(db, ssm.str().c_str(), 0, 0, &zErrMsg);
					Sleep(1);
				}

				// Process renamed directories
				if (0 != (pRecord->Reason & USN_REASON_RENAME_NEW_NAME)) {
					ssm.clear();
					ssm.str("");
					filename.clear();
					WCharToMByte(szFile, filename);
					ssm << "update file_info set name='" << filename.data() << "' where frn=" << pRecord->FileReferenceNumber;
					sqlite3_exec(db, ssm.str().c_str(), 0, 0, &zErrMsg);
					Sleep(1);
				}

				// Process deleted directories
				if (0 != (pRecord->Reason & USN_REASON_FILE_DELETE)) {
					ssm.clear();
					ssm.str("");
					ssm << "delete from file_info where frn=" << pRecord->FileReferenceNumber;
					sqlite3_exec(db, ssm.str().c_str(), 0, 0, &zErrMsg);
					Sleep(1);
				}
			}
		}
		ssm.clear();
		ssm.str("");
		ssm << "update usn_check_point set usn=" << m_cj.m_rujd.StartUsn<<",usn_journal_id="<<m_cj.m_rujd.UsnJournalID<< " where id=0";
		sqlite3_exec(db, ssm.str().c_str(), 0, 0, &zErrMsg);
		sqlite3_exec(db, "commit;", 0, 0, 0);
		if(!m_cj.NotifyMoreData(1000))
		return;
	}
	
	
	sqlite3_close(db);

}

int main(int argc, char* argv[])
{
	
	//recurse_record('c');
	CChangeJrnl m_cj;
	//init_changejrn( m_cj,  'c');
	initial_file_db(m_cj, 'c');
	updating_db(m_cj,'c');

	return 0;
}


