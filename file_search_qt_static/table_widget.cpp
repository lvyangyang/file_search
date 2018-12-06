#include "table_widget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDateTime>
#include <thread>
#include <ctype.h>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QTextCodec>
CString UTF82WCS(const char* szU8)
{
	//预转换，得到所需空间的大小;
	int wcsLen = ::MultiByteToWideChar(CP_UTF8, NULL, szU8, strlen(szU8), NULL, 0);

	//分配空间要给'\0'留个空间，MultiByteToWideChar不会给'\0'空间
	wchar_t* wszString = new wchar_t[wcsLen + 1];

	//转换
	::MultiByteToWideChar(CP_UTF8, NULL, szU8, strlen(szU8), wszString, wcsLen);

	//最后加上'\0'
	wszString[wcsLen] = '\0';

	CString unicodeString(wszString);

	delete[] wszString;
	wszString = NULL;

	return unicodeString;
}
string get_ext_name(const std::string str)
{
	if ("" == str)
	{
		return string("");
	}
	std::string strs = str + ".";
	size_t pos = str.find(".");
	size_t old_pos;
	size_t size = strs.size();
	while (pos != strs.size()-1)
	{	old_pos = pos;
		strs = strs.substr(pos + 1, size);
		pos = strs.find(".");
	}
	return strs.substr(0, strs.size()-1);
}

table_widget::table_widget(QWidget *parent):QWidget(parent)
{
		this->resize( QSize( 1200, 600 ));
		this->setWindowTitle("SV文件搜索助手");
		QGridLayout *gLayout = new QGridLayout;
		gLayout->setColumnStretch(1,0);
		QHBoxLayout *hLayout =new QHBoxLayout;
		add_item=new QPushButton("搜索",this);
		add_item->setFixedHeight(30);
		add_item->setFixedWidth(50);
		wordText=new QLineEdit(this);
		wordText->setFixedWidth(200);
		wordText->setFixedHeight(30);
		lab_result_count=new QLabel("相关结果:",this);
		QFont font;
		font.setPointSize(16);
		lab_result_count->setFont(font);

		result_count = new QLCDNumber(6,this);

		result_count->setSegmentStyle(QLCDNumber::Flat);
		   //调色板
		   QPalette lcdpat = result_count->palette();
		   /*设置颜色，整体背景颜色 颜色蓝色,此函数的第一个参数可以设置多种。如文本、按钮按钮文字、多种*/
		   lcdpat.setColor(QPalette::Normal,QPalette::WindowText,Qt::blue);
		   //设置当前窗口的调色板
		   result_count->setPalette(lcdpat);
		   //设置背景色

		count=0;
		busy=false;
		//dirText=new QLineEdit(this);
		//dirText->setFixedHeight(30);


		hLayout->addWidget(wordText);
		hLayout->addWidget(add_item);
		hLayout->addWidget(lab_result_count);
		hLayout->addWidget(result_count);
		hLayout->addStretch(0);
		//hLayout->addWidget(dirText);

		gLayout->addLayout(hLayout,0,0);

		table_view = new QTableView;
		gLayout->addWidget(table_view, 1, 0);
		item_model = new QStandardItemModel();
		item_model->setColumnCount(5);
		item_model->setHeaderData(0,Qt::Horizontal, "文件名");
		item_model->setHeaderData(1,Qt::Horizontal, "修改时间");
		item_model->setHeaderData(2,Qt::Horizontal, "大小");
		item_model->setHeaderData(3,Qt::Horizontal, "类型");
		item_model->setHeaderData(4,Qt::Horizontal, "位置");

		table_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
		table_view->setSelectionBehavior(QAbstractItemView::SelectRows);
		table_view->setSortingEnabled(true);
		//table_view->horizontalHeader()->hide();
		table_view->setModel(item_model);
		table_view->setShowGrid(false);                               // 隐藏网格线
		table_view->setFocusPolicy(Qt::NoFocus);                      // 去除当前Cell周边虚线框
		table_view->setAlternatingRowColors(true);
		table_view->horizontalHeader()->setStretchLastSection(true);
		table_view->verticalHeader()->hide();

		table_view->setColumnWidth(0,300);
		table_view->setColumnWidth(1,150);

		setLayout(gLayout);
		connect(add_item,&QPushButton::clicked,this,&table_widget::add_o_item);
		connect(wordText, SIGNAL(returnPressed()), this, SLOT(add_o_item()));
		connect(table_view, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(open_dir(const QModelIndex &)));

}

void table_widget::add_o_item()
{

		if(busy)
			return;
		item_model->clear();
		item_model->setColumnCount(5);
		item_model->setHeaderData(0,Qt::Horizontal, "文件名");
		item_model->setHeaderData(1,Qt::Horizontal, "修改时间");
		item_model->setHeaderData(2,Qt::Horizontal, "大小");
		item_model->setHeaderData(3,Qt::Horizontal, "类型");
		item_model->setHeaderData(4,Qt::Horizontal, "位置");
		table_view->setColumnWidth(0,300);
		table_view->setColumnWidth(1,150);
		count=0;
		result_count->display(count);
		if(wordText->text()=="")
			return;

		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		busy=true;
		QList<QStandardItem *> lsi;
		search_result temp_result;
		get_drives getdrives;

		std::set<char> drives=getdrives.gets();
		string target_dir=dirText.toStdString();
		if(target_dir=="")
		{

			for(auto & drive_letter : drives)
			{
				TCHAR Path[1024];
				wsprintf(Path, TEXT("%c:"), drive_letter);
				db_search test(drive_letter,wordText->text().toStdString(),get_frn_dir(Path),true);
				while (test.get_one_result(temp_result))
					{
					lsi.append(new QStandardItem(temp_result.filename.data()));
					lsi.append(new QStandardItem(QDateTime::fromTime_t(temp_result.write_time).toString("yyyy-MM-dd hh:mm:ss")));
					QStandardItem *item=new QStandardItem();
					item->setData(QVariant(temp_result.file_size),Qt::DisplayRole);
					lsi.append(item);
					lsi.append(new QStandardItem(get_ext_name(temp_result.filename).data()));
					ssm.clear();
					ssm.str("");
					ssm<<drive_letter<<":\\"<<temp_result.address;
					lsi.append(new QStandardItem(ssm.str().c_str()));
					item_model->appendRow(lsi);
					lsi.clear();
					count++;
					}
				db_search test_1(drive_letter,wordText->text().toStdString(),get_frn_dir(Path),false);
				while (test_1.get_one_result(temp_result))
					{
					lsi.append(new QStandardItem(temp_result.filename.data()));
					lsi.append(new QStandardItem(QDateTime::fromTime_t(temp_result.write_time).toString("yyyy-MM-dd hh:mm:ss")));
					QStandardItem *item=new QStandardItem();
					item->setData(QVariant(0),Qt::DisplayRole);
					lsi.append(item);
					lsi.append(new QStandardItem("文件夹"));
					ssm.clear();
					ssm.str("");
					ssm<<drive_letter<<":\\"<<temp_result.address;
					lsi.append(new QStandardItem(ssm.str().c_str()));
					item_model->appendRow(lsi);
					lsi.clear();
					count++;
					}

			}

		}else{
			this->setWindowTitle(dirText);
			char targer_drive=toupper(target_dir.data()[0]);
			if(drives.find(targer_drive)!=drives.end())
			{
				//TCHAR Path[1024];
				//wsprintf(Path, TEXT("%S"), target_dir.data());
				CString Path=UTF82WCS(target_dir.data());
				db_search test(targer_drive,wordText->text().toStdString(),get_frn_dir(Path),true);
				while (test.get_one_result(temp_result))
					{
					lsi.append(new QStandardItem(temp_result.filename.data()));
					lsi.append(new QStandardItem(QDateTime::fromTime_t(temp_result.write_time).toString("yyyy-MM-dd hh:mm:ss")));
					QStandardItem *item=new QStandardItem();
					item->setData(QVariant(temp_result.file_size),Qt::DisplayRole);
					lsi.append(item);
					lsi.append(new QStandardItem(get_ext_name(temp_result.filename).data()));
					ssm.clear();
					ssm.str("");
					ssm<<"\\"<<temp_result.address;
					lsi.append(new QStandardItem(ssm.str().c_str()));
					item_model->appendRow(lsi);
					lsi.clear();
					count++;
					}
				db_search test_1(targer_drive,wordText->text().toStdString(),get_frn_dir(Path),false);
				while (test_1.get_one_result(temp_result))
					{
					lsi.append(new QStandardItem(temp_result.filename.data()));
					lsi.append(new QStandardItem(QDateTime::fromTime_t(temp_result.write_time).toString("yyyy-MM-dd hh:mm:ss")));
					QStandardItem *item=new QStandardItem();
					item->setData(QVariant(0),Qt::DisplayRole);
					lsi.append(item);
					lsi.append(new QStandardItem("文件夹"));
					ssm.clear();
					ssm.str("");
					ssm<<"\\"<<temp_result.address;
					lsi.append(new QStandardItem(ssm.str().c_str()));
					item_model->appendRow(lsi);
					lsi.clear();
					count++;
					}
			}


		}

QApplication::restoreOverrideCursor();
busy=false;
result_count->display(count);

}


void table_widget::open_dir(const QModelIndex &index)
{

	 QModelIndex index_path = item_model->index(index.row(),4);
	 QString file_path=item_model->data(index_path,Qt::DisplayRole).value<QString>();

	 QModelIndex index_name = item_model->index(index.row(),0);
	 QString file_name=item_model->data(index_name,Qt::DisplayRole).value<QString>();

	 string str_path=file_path.toStdString();
	 string str_name=file_name.toStdString();

	 string target_dir=dirText.toStdString();
	 if(target_dir=="")
	 {
		 ssm.clear();
		 ssm.str("");
		 ssm<<str_path<<str_name;
	 }else
			{
		 ssm.clear();
		 ssm.str("");
		 ssm<<target_dir<<str_path<<str_name;

	 }
	// QDesktopServices::openUrl(QUrl(ssm.str().c_str(), QUrl::TolerantMode));

	 QTextCodec *codec = QTextCodec::codecForName("GB18030");
	 QString q_path = QString::fromStdString(ssm.str());
	 QString argsStr = "/select, "+q_path.replace("/","\\"); //替换文件目录分隔形式
	 ShellExecuteA(NULL,"open","explorer",codec->fromUnicode(argsStr).constData(),NULL,SW_SHOWDEFAULT);
}

DWORDLONG table_widget:: get_frn_dir(LPCTSTR pszPath)
{
	TCHAR szTempRecursePath[1024];
	lstrcpy(szTempRecursePath, pszPath);
	lstrcat(szTempRecursePath, TEXT("\\"));
	//lstrcat(szTempRecursePath, pszFile);

	HANDLE hDir = CreateFile(szTempRecursePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (INVALID_HANDLE_VALUE == hDir) {
		return 0;
	}
	BY_HANDLE_FILE_INFORMATION fi;
	GetFileInformationByHandle(hDir, &fi);
	CloseHandle(hDir);
	LARGE_INTEGER index;
	index.LowPart = fi.nFileIndexLow;
	index.HighPart = fi.nFileIndexHigh;
	return index.QuadPart;
}

void table_widget::set_dir(std::string dir)
{
	dirText=QString::fromStdString(dir);
	this->setWindowTitle(dirText);
}

table_widget::~table_widget()
{
}

