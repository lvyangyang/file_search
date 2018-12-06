#ifndef DB_SEARCH_H
#define DB_SEARCH_H

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
#include<stack>
#include<set>
using namespace std;
struct dir_info
{
	string filename;
	DWORDLONG parent_frn;
	DWORD attrib;
	time_t write_time;
	//vector<DWORDLONG> sub_dir_v;
};

struct search_result
{
	string filename;
	time_t write_time;
	DWORD attrib;
	string address;
	DWORDLONG frn;
	DWORDLONG parent_frn;
	DWORDLONG file_size;
};

class db_search
{
public:
	db_search(char drive_letter,string key_word, DWORDLONG target_frn, bool search_file);
	~db_search();

private:
	sqlite3 *db;
	char drive_letter;
	string search_word;
	DWORDLONG target_frn;
	sqlite3_stmt * stmt;
	sqlite3_stmt * parent_dir_stmt;
	map<DWORDLONG,dir_info> dir_cache;//目录缓存
	set<DWORDLONG> dir_black_list;
private:
	bool get_addr(stringstream &ssm, DWORDLONG parent_frn);
	dir_info get_parent_dirinfo(DWORDLONG frn,bool &error);
public:
	bool get_one_result(search_result &result);
public:
	bool has_results;
	bool search_file;
};

#endif // DB_SEARCH_H
