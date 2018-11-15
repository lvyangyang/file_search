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
#include<thread>
#include "ChangeJrnl.h"

#define MAX_DIR_LENGTH 4096
using namespace std;

BOOL WCharToMByte(LPCWSTR lpcwszStr, std::string &str)
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


time_t FileTimeToTime_t(FILETIME ft)
{
	LONGLONG ll;
	ULARGE_INTEGER ui;
	ui.LowPart = ft.dwLowDateTime;
	ui.HighPart= ft.dwHighDateTime;
	ll = ft.dwHighDateTime<< 32 + ft.dwLowDateTime;		//这一步是不是多余的
	return ((LONGLONG)(ui.QuadPart - 116444736000000000) / 10000000);
}

time_t FileTimeToTime_t(LARGE_INTEGER ft)
{
	LONGLONG ll;
	ULARGE_INTEGER ui;
	ui.LowPart = ft.LowPart;
	ui.HighPart = ft.HighPart;
	ll = ft.HighPart << 32 + ft.LowPart;		//这一步是不是多余的
	return ((LONGLONG)(ui.QuadPart - 116444736000000000) / 10000000);
}



struct file_info
{
	string filename;
	DWORDLONG parent_frn;
	DWORD attrib;
	time_t write_time;
};
struct dir_info
{
	string filename;
	DWORDLONG parent_frn;
	DWORD attrib;
	time_t write_time;
	//vector<DWORDLONG> sub_dir_v;
};

struct file_dir_db
{
	char drive_letter;
	map<DWORDLONG, file_info> fileinfo_db;
	map<DWORDLONG, dir_info> dirinfo_db;
};

int file_count = 0;

DWORDLONG get_frn_dir(LPCTSTR pszPath, DWORD volum_serial_number,bool &error)
{
	TCHAR szTempRecursePath[MAX_DIR_LENGTH];
	lstrcpy(szTempRecursePath, pszPath);
	lstrcat(szTempRecursePath, TEXT("\\"));
	//lstrcat(szTempRecursePath, pszFile);

	HANDLE hDir = CreateFile(szTempRecursePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (INVALID_HANDLE_VALUE == hDir) {
		cout << "error" << endl;
		error = true;
	}
	BY_HANDLE_FILE_INFORMATION fi;
	GetFileInformationByHandle(hDir, &fi);
	CloseHandle(hDir);

	if (volum_serial_number != fi.dwVolumeSerialNumber)
		error = true;

	LARGE_INTEGER index;
	index.LowPart = fi.nFileIndexLow;
	index.HighPart = fi.nFileIndexHigh;
	return index.QuadPart;
}

DWORD get_volum_serial_number(LPCTSTR pszPath)
{
	TCHAR szTempRecursePath[MAX_DIR_LENGTH];
	lstrcpy(szTempRecursePath, pszPath);
	lstrcat(szTempRecursePath, TEXT("\\"));
	//lstrcat(szTempRecursePath, pszFile);

	HANDLE hDir = CreateFile(szTempRecursePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (INVALID_HANDLE_VALUE == hDir) {
		cout << "error" << endl;
	}
	BY_HANDLE_FILE_INFORMATION fi;
	GetFileInformationByHandle(hDir, &fi);
	CloseHandle(hDir);
	return fi.dwVolumeSerialNumber;
}

DWORDLONG get_frn_file(LPCTSTR pszPath, LPCTSTR pszFile, bool &error)
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

void insert_fileinfo_into_db(sqlite3 *db, map<DWORDLONG, file_info> &fileinfo_db)
{
	sqlite3_exec(db, "begin;", 0, 0, 0);
	sqlite3_stmt *stmt_file;
	const char* sql_file = "insert into file_info(frn,parentfrn,attrib,write_time,name) values(?,?,?,?,?)";
	sqlite3_prepare_v2(db, sql_file, strlen(sql_file), &stmt_file, 0);
	map<DWORDLONG, file_info>::iterator iter;
	iter = fileinfo_db.begin();
	while (iter != fileinfo_db.end())
	{
		sqlite3_reset(stmt_file);

		sqlite3_bind_int64(stmt_file, 1, iter->first);
		sqlite3_bind_int64(stmt_file, 2, iter->second.parent_frn);
		sqlite3_bind_int64(stmt_file, 3, iter->second.attrib);
		sqlite3_bind_int64(stmt_file, 4, iter->second.write_time);
		sqlite3_bind_text(stmt_file, 5, iter->second.filename.data(), -1, SQLITE_STATIC);



		sqlite3_step(stmt_file);
		iter++;
	}
	sqlite3_finalize(stmt_file);
	sqlite3_exec(db, "commit;", 0, 0, 0);
}
void insert_dirinfo_into_db(sqlite3 *db, map<DWORDLONG, dir_info> &dirinfo_db)
{
	sqlite3_exec(db, "begin;", 0, 0, 0);
	sqlite3_stmt *stmt_dir;
	const char* sql_dir = "insert into dir_info(frn,parentfrn,attrib,write_time,name) values(?,?,?,?)";
	sqlite3_prepare_v2(db, sql_dir, strlen(sql_dir), &stmt_dir, 0);

	map<DWORDLONG, dir_info>::iterator iter_dir;
	iter_dir = dirinfo_db.begin();
	while (iter_dir != dirinfo_db.end())
	{
		sqlite3_reset(stmt_dir);
		sqlite3_bind_int64(stmt_dir, 1, iter_dir->first);
		sqlite3_bind_int64(stmt_dir, 2, iter_dir->second.parent_frn);
		sqlite3_bind_int64(stmt_dir, 3, iter_dir->second.attrib);
		sqlite3_bind_int64(stmt_dir, 4, iter_dir->second.write_time);
		sqlite3_bind_text(stmt_dir, 5, iter_dir->second.filename.data(), -1, SQLITE_STATIC);

		sqlite3_step(stmt_dir);
		iter_dir++;
	}
	sqlite3_finalize(stmt_dir);
	sqlite3_exec(db, "commit;", 0, 0, 0);
}

void RecursePath(LPCTSTR pszPath, LPCTSTR pszFile,
	DWORDLONG ParentIndex, DWORD dwVolumeSerialNumber, 
	sqlite3 *db,
	map<DWORDLONG, file_info> &fileinfo_db, 
	map<DWORDLONG, dir_info> &dirinfo_db) 
{
	if (fileinfo_db.size() > 50000)
	{
		insert_fileinfo_into_db(db, fileinfo_db);
		fileinfo_db.clear();
	}
	
	file_info temp_fileinfo;
	//dir_info temp_dirinfo;
	bool error = FALSE;

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
			error = FALSE;
			DWORDLONG temp_frn = get_frn_dir(szTempRecursePath, dwVolumeSerialNumber, error);
			if (!error&&dirinfo_db.find(temp_frn) == dirinfo_db.end())
			{	
				//填充文件信息
				dirinfo_db[temp_frn].parent_frn = ParentIndex;
				dirinfo_db[temp_frn].attrib = fd.dwFileAttributes;
				dirinfo_db[temp_frn].write_time = FileTimeToTime_t(fd.ftLastWriteTime);
				string filename;
				WCharToMByte(fd.cFileName, filename);
				temp_fileinfo.attrib = fd.dwFileAttributes;
				dirinfo_db[temp_frn].filename = filename;
				RecursePath(szTempRecursePath, fd.cFileName, temp_frn,
					dwVolumeSerialNumber,db, fileinfo_db, dirinfo_db);
			}	
			
		}
		else
		{
			if (0 == (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				//_tprintf(szTempRecursePath);
				//cout << endl;
				error = FALSE;
				WCharToMByte(fd.cFileName, temp_fileinfo.filename);
				temp_fileinfo.attrib = fd.dwFileAttributes;
				temp_fileinfo.parent_frn = ParentIndex;
				DWORDLONG temp_frn = get_frn_file(pszPath,fd.cFileName, error);
				if (error)
					continue;
				file_count++;
				fileinfo_db[temp_frn] = temp_fileinfo;
				fileinfo_db[temp_frn].write_time= FileTimeToTime_t(fd.ftLastWriteTime);
			}

		}

	} while (FindNextFile(hSearch, &fd) != FALSE);

	FindClose(hSearch);
}

void recurse_record(file_dir_db &temp_db)
{
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	stringstream ssm;  
	ssm<< temp_db.drive_letter <<"_"<<"file_search.db";
	rc = sqlite3_open(ssm.str().c_str(), &db);

	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	}
	//创建数据表
	sqlite3_exec(db, "PRAGMA synchronous = OFF; ", 0, 0, 0);
	sqlite3_exec(db, "drop table if exists file_info", 0, 0, 0);
	sqlite3_exec(db, "create  table file_info (frn integer primary key,parentfrn integer,attrib integer ,write_time integer,name text);", 0, 0, &zErrMsg);
	sqlite3_exec(db, "drop table if exists dir_info", 0, 0, 0);
	sqlite3_exec(db, "create  table dir_info (frn integer primary key,parentfrn integer,attrib integer ,write_time integer,name text);", 0, 0, &zErrMsg);
	sqlite3_exec(db, "create index frn_index on dir_info(frn);", 0, 0, &zErrMsg);
	//将信息收集到内存
//	map<DWORDLONG, file_info> fileinfo_db;
//	map<DWORDLONG, dir_info> dirinfo_db;
	TCHAR szCurrentPath[MAX_DIR_LENGTH];
	wsprintf(szCurrentPath, TEXT("%c:"), temp_db.drive_letter);
	//wsprintf(szCurrentPath, TEXT("%s:"), "c:\\Users\\fenhuo\\Documents\\Visual Studio 2015\\Projects");
	//lstrcat(szCurrentPath, TEXT("\\Users\\fenhuo\\Documents\\Visual Studio 2015\\Projects\\file_search\\file_search"));
	DWORD volum_serial_number = get_volum_serial_number(szCurrentPath);
	bool error;
	RecursePath(szCurrentPath, szCurrentPath, get_frn_dir(szCurrentPath,0,error), volum_serial_number, db,temp_db.fileinfo_db, temp_db.dirinfo_db);

	//插入文件信息
	sqlite3_exec(db, "begin;", 0, 0, 0);
	sqlite3_stmt *stmt_file;
	const char* sql_file = "insert into file_info(frn,parentfrn,attrib,write_time,name) values(?,?,?,?,?)";
	sqlite3_prepare_v2(db, sql_file, strlen(sql_file), &stmt_file, 0);
	map<DWORDLONG, file_info>::iterator iter;
	iter = temp_db.fileinfo_db.begin();
	while (iter != temp_db.fileinfo_db.end())
	{
		sqlite3_reset(stmt_file);

		sqlite3_bind_int64(stmt_file, 1, iter->first);
		sqlite3_bind_int64(stmt_file, 2, iter->second.parent_frn);
		sqlite3_bind_int64(stmt_file, 3, iter->second.attrib);
		sqlite3_bind_int64(stmt_file, 4, iter->second.write_time);
		sqlite3_bind_text(stmt_file, 5, iter->second.filename.data(), -1, SQLITE_STATIC);
		
		

		sqlite3_step(stmt_file);
		iter++;
	}
	sqlite3_finalize(stmt_file);
	sqlite3_exec(db, "commit;", 0, 0, 0);

	//temp_db.fileinfo_db.~map();//清空内存中的文件信息
	//插入目录信息
	sqlite3_exec(db, "begin;", 0, 0, 0);
	sqlite3_stmt *stmt_dir;
	const char* sql_dir = "insert into dir_info(frn,parentfrn,attrib,write_time,name) values(?,?,?,?,?)";
	sqlite3_prepare_v2(db, sql_dir, strlen(sql_dir), &stmt_dir, 0);

	map<DWORDLONG, dir_info>::iterator iter_dir;
	iter_dir = temp_db.dirinfo_db.begin();
	while (iter_dir != temp_db.dirinfo_db.end())
	{
		sqlite3_reset(stmt_dir);
		sqlite3_bind_int64(stmt_dir, 1, iter_dir->first);
		sqlite3_bind_int64(stmt_dir, 2, iter_dir->second.parent_frn);
		sqlite3_bind_int64(stmt_dir, 3, iter_dir->second.attrib);
		sqlite3_bind_int64(stmt_dir, 4, iter_dir->second.write_time);
		sqlite3_bind_text(stmt_dir, 5, iter_dir->second.filename.data(), -1, SQLITE_STATIC);
		
		

		sqlite3_step(stmt_dir);
		iter_dir++;
	}
	sqlite3_finalize(stmt_dir);
	sqlite3_exec(db, "commit;", 0, 0, 0);

	cout << file_count;
	sqlite3_close(db);
}
//-----------------------------------------------------------------------------------------------------
void populate_dirinfo_mem(file_dir_db &temp_db)
{
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	stringstream ssm;
	ssm << temp_db.drive_letter << "_" << "file_search.db";
	rc = sqlite3_open(ssm.str().c_str(), &db);

	if (rc) {
		fprintf(stderr, "error populate_dirinfo_mem: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_stmt * stmt = NULL;
	const char *zTail;
	dir_info temp_dir_info;
	DWORDLONG frn;

	if (sqlite3_prepare_v2(db,
		"SELECT * FROM dir_info;", -1, &stmt, &zTail) == SQLITE_OK) {
		while(sqlite3_step(stmt) == SQLITE_ROW) {
			frn = sqlite3_column_int64(stmt, 0);
			temp_dir_info.parent_frn = sqlite3_column_int64(stmt, 1);
			temp_dir_info .attrib= sqlite3_column_int64(stmt, 2);
			string name((const char *)sqlite3_column_text(stmt, 3));
			temp_dir_info.filename= name;
			temp_db.dirinfo_db[frn] = temp_dir_info;
		}
	}
	sqlite3_finalize(stmt);
	//关闭数据库 
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

void initial_file_db(CChangeJrnl &m_cj, char drive_letter)
{
	init_changejrn( m_cj, drive_letter);
	
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	stringstream ssm;
	ssm << drive_letter << "_" << "file_search.db";
	rc = sqlite3_open(ssm.str().c_str(), &db);

	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
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
		//	populate_dirinfo_mem(temp_db);
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


	file_dir_db temp_db;
	temp_db.drive_letter = drive_letter;
	cout << ssm.str().c_str();
	remove(ssm.str().c_str());
	recurse_record(temp_db);
	//更新usn_check_point
	//重新扫描磁盘前删除原有的
	rc = sqlite3_open(ssm.str().c_str(), &db);
	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_exec(db, "create table if not exists usn_check_point(id integer,usn integer,usn_journal_id integer)", 0, 0, 0);
	sqlite3_exec(db, "delete from usn_check_point where id=0;", 0, 0, &zErrMsg);
	ssm.clear();
	ssm.str("");
	ssm << "insert into usn_check_point values(" << "0" << "," << m_cj.m_rujd.StartUsn << "," << m_cj.m_rujd.UsnJournalID << ");";
	sqlite3_exec(db, ssm.str().c_str(), 0, 0, &zErrMsg);
	sqlite3_close(db);
}
//-----------------------------------------------------------------------------------------------------
void updating_db(CChangeJrnl &m_cj, char drive_letter)
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

	sqlite3_exec(db, "PRAGMA synchronous = OFF; ", 0, 0, 0);
	

	PUSN_RECORD pRecord;
	dir_info temp_dir_info;
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
					ssm << "insert into dir_info(frn,parentfrn,attrib,write_time,name) values(" << pRecord->FileReferenceNumber << ","
						<< pRecord->ParentFileReferenceNumber << "," << pRecord->FileAttributes <<","<< FileTimeToTime_t(pRecord->TimeStamp)<<",'" << filename.data() << "')";
					sqlite3_exec(db, ssm.str().c_str(), 0, 0, &zErrMsg);

					//Sleep(1);
				}

				// Process renamed directories
				if (0 != (pRecord->Reason & USN_REASON_RENAME_NEW_NAME)) {
					//g_Globals.m_db.Change(pRecord->FileReferenceNumber, szFile,pRecord->ParentFileReferenceNumber);
					ssm.clear();
					ssm.str("");
					filename.clear();
					WCharToMByte(szFile, filename);
					ssm << "update dir_info set name='" << filename.data() << "',"<< "write_time="<< FileTimeToTime_t(pRecord->TimeStamp)<<" where frn=" << pRecord->FileReferenceNumber;
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

				}
			}
			else {
				// Process newly created file
				if (0 != (pRecord->Reason & USN_REASON_FILE_CREATE)) {
					//	g_Globals.m_db.Add(pRecord->FileReferenceNumber, szFile,pRecord->ParentFileReferenceNumber);
					ssm.clear();
					ssm.str("");
					filename.clear();
					WCharToMByte(szFile, filename);
					ssm << "insert  into file_info(frn,parentfrn,attrib,write_time,name) values(" << pRecord->FileReferenceNumber << ","
						<< pRecord->ParentFileReferenceNumber << "," << pRecord->FileAttributes << "," << FileTimeToTime_t(pRecord->TimeStamp) << ",'" << filename.data() << "')";
					sqlite3_exec(db, ssm.str().c_str(), 0, 0, &zErrMsg);
					Sleep(1);
				}

				// Process renamed file
				if (0 != (pRecord->Reason & USN_REASON_RENAME_NEW_NAME)) {
					ssm.clear();
					ssm.str("");
					filename.clear();
					WCharToMByte(szFile, filename);
					ssm << "update file_info set name='" << filename.data() << "',"<<" write_time="<< FileTimeToTime_t(pRecord->TimeStamp)<<" where frn=" << pRecord->FileReferenceNumber;
					sqlite3_exec(db, ssm.str().c_str(), 0, 0, &zErrMsg);
					Sleep(1);
				}

				// Process deleted file
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
		if (!m_cj.NotifyMoreData(1000))
		return;//重新启动
	}
	sqlite3_close(db);
}

BOOL is_ntfs(LPWSTR szDrive) {
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

vector<char> get_drives() {
	TCHAR szLogicDriveStrings[1024];
	TCHAR *szDrive;
	std::vector<char> driver_letters;
	std::string driver_string;

	ZeroMemory(szLogicDriveStrings, 1024);

	GetLogicalDriveStrings(1024 - 1, szLogicDriveStrings);
	szDrive = szLogicDriveStrings;

	do
	{
		if (is_ntfs(szDrive))
		{
			WCharToMByte(szDrive, driver_string);
			driver_letters.push_back(driver_string.data()[0]);
		}
		szDrive += (lstrlen(szDrive) + 1);
	} while (*szDrive != '\x00');
	return driver_letters;
}

int main(int argc, char* argv[])
{
	vector<char> driver_letters=get_drives();
/*
	CChangeJrnl m_cj;
	char drive_letter = driver_letters.at(1);
	initial_file_db(m_cj, drive_letter);
	updating_db(m_cj, drive_letter);
*/


	vector<thread> threads;
	for (int i = 0; i < driver_letters.size(); i++)
	{
		threads.push_back(thread([&]() {
			CChangeJrnl m_cj;
			char drive_letter = driver_letters.at(i);
			while (true)
			{
				initial_file_db(m_cj, drive_letter);
				updating_db(m_cj, drive_letter);
			}
			
		}));
	}
	for (auto& th : threads)th.join();
	return 0;
}


