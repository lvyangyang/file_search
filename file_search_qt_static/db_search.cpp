#include "db_search.h"

db_search::db_search(char drive_letter, string key_word, DWORDLONG target_frn,bool search_file):
	drive_letter(drive_letter),search_word(key_word), target_frn(target_frn),search_file(search_file)
{
	stringstream ssm;
	ssm <<"C:\\Program Files (x86)\\file_search\\"<< drive_letter << "_" << "file_search.db";
	int rc = sqlite3_open(ssm.str().c_str(), &db);

	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	}
	stmt = NULL;
	parent_dir_stmt=NULL;

	ssm.clear();
	ssm.str("");
	if(search_file)
		ssm << "select * from file_info where name like '%" << search_word.data() << "%';";
	else
		ssm << "select * from dir_info where name like '%" << search_word.data() << "%';";
	sqlite3_prepare_v2(db,ssm.str().c_str(), -1, &stmt, 0);

	const char* sql_parent_dir = "SELECT * FROM dir_info where frn= ?;";
	sqlite3_prepare_v2(db, sql_parent_dir, strlen(sql_parent_dir), &parent_dir_stmt, 0);
}

db_search::~db_search()
{
	sqlite3_finalize(stmt);
	sqlite3_finalize(parent_dir_stmt);
	sqlite3_close(db);
}


dir_info db_search:: get_parent_dirinfo( DWORDLONG frn,bool &error)
{
	dir_info temp_dir_info;
	//const char* sql_parent_dir = "SELECT * FROM dir_info where frn= ?;";
	//sqlite3_prepare_v2(db, sql_parent_dir, strlen(sql_parent_dir), &stmt, 0);

	sqlite3_reset(parent_dir_stmt);
	sqlite3_bind_int64(parent_dir_stmt, 1, frn);
	if (sqlite3_step(parent_dir_stmt) == SQLITE_ROW) {
		temp_dir_info.parent_frn = sqlite3_column_int64(parent_dir_stmt, 1);
		temp_dir_info.attrib = sqlite3_column_int64(parent_dir_stmt, 2);
		temp_dir_info.write_time = sqlite3_column_int64(parent_dir_stmt, 3);
		string name((const char *)sqlite3_column_text(parent_dir_stmt, 4));
		temp_dir_info.filename = name;
		error=false;
	}
	else
		error=true;
	return temp_dir_info;
}

bool db_search::get_addr(stringstream &ssm, DWORDLONG parent_frn)
{
	std::stack<string> dir_stack;
	vector<DWORDLONG> all_parentfrn;
	bool uder_target_dir = false;
	bool error;
	DWORDLONG temp_parent_frn = parent_frn;
	while (true)
	{
		if (temp_parent_frn == target_frn)
		{
			uder_target_dir = true;
			break;
		}
		if (dir_black_list.find(temp_parent_frn) != dir_black_list.end())
			break;
		if(dir_cache.find(temp_parent_frn)==dir_cache.end())
			dir_cache[temp_parent_frn]=get_parent_dirinfo(temp_parent_frn,error);
		dir_stack.push(dir_cache[temp_parent_frn].filename);
		if(error)
			return false;
		temp_parent_frn = get_parent_dirinfo(temp_parent_frn,error).parent_frn;
	}
	//判断是否在指定目录下
	if (!uder_target_dir)
		{
			for (auto &frn : all_parentfrn)
				dir_black_list.insert(frn);
			return false;
		}
	ssm.clear();
	ssm.str("");
	while (!dir_stack.empty())
	{
		ssm << dir_stack.top().data() << "\\";
		dir_stack.pop();
	}
	return true;
}

bool db_search::get_one_result(search_result &result)
{
	bool has_result = false;
	stringstream ssm;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		DWORDLONG parentfrn = sqlite3_column_int64(stmt, 1);

		if (get_addr(ssm, parentfrn))
		{	std::string name;
			has_result = true;
			result.parent_frn = parentfrn;
			result.frn= sqlite3_column_int64(stmt, 0);
			result.attrib = sqlite3_column_int64(stmt,2);
			result.write_time = sqlite3_column_int64(stmt, 3);
			if(search_file)
			{
				result.file_size = sqlite3_column_int64(stmt, 4);
				 name=string((const char *)sqlite3_column_text(stmt, 5));
			}else
				 name=string((const char *)sqlite3_column_text(stmt, 4));

			result.filename = name;
			result.address = ssm.str();
			break;
		}

	}
	return has_result;
}
