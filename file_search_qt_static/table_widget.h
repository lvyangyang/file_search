#ifndef TABLE_WIDGET_H
#define TABLE_WIDGET_H
#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QLCDNumber>
#include "sqlite3.h"
#include <windows.h>
#include <map>
#include <stack>
#include <sstream>
#include <thread>
#include "get_drives.h"
#include "db_search.h"
#pragma execution_character_set("utf-8")

struct db_info
{
	sqlite3 * dbs;
	std::thread populate_ths;
};
class table_widget:public QWidget
{
	Q_OBJECT
public:
	table_widget(QWidget *parent = 0);
	~table_widget();
private:
	stringstream ssm;
	QTableView * table_view;
	QStandardItemModel * item_model;
	QPushButton *add_item;
	QLineEdit *wordText;
	QString dirText;
	QLabel *lab_result_count;
	QLCDNumber *result_count;
	int count;
	bool busy;
public slots:
	void add_o_item();
	void open_dir(const QModelIndex &index);



public:
	DWORDLONG  get_frn_dir(LPCTSTR pszPath);
	void set_dir(std::string s);
};

#endif // TABLE_WIDGET_H
