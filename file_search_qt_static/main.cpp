#include "mainwindow.h"
#include "table_widget.h"
#include <QApplication>
#include "get_drives.h"
#include "db_search.h"
 string ANSItoUTF8(const char* strAnsi)
 {
	  //获取转换为宽字节后需要的缓冲区大小，创建宽字节缓冲区，936为简体中文GB2312代码页
	int nLen = MultiByteToWideChar(CP_ACP, NULL, strAnsi, -1, NULL, NULL);
	WCHAR *wszBuffer = new WCHAR[nLen + 1];
	nLen = MultiByteToWideChar(CP_ACP, NULL, strAnsi, -1, wszBuffer, nLen);
	wszBuffer[nLen] = 0;
	  //获取转为UTF8多字节后需要的缓冲区大小，创建多字节缓冲区
	nLen = WideCharToMultiByte(CP_UTF8, NULL, wszBuffer, -1, NULL, NULL, NULL, NULL);
	CHAR *szBuffer = new CHAR[nLen + 1];
	nLen = WideCharToMultiByte(CP_UTF8, NULL, wszBuffer, -1, szBuffer, nLen, NULL, NULL);
	szBuffer[nLen] = 0;

	 string s1 = szBuffer;
	 //内存清理
	 delete[]wszBuffer;
	 delete[]szBuffer;
	 return s1;
};

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	table_widget w;
	if(argc>=2)
	{
		std::string real_dir;

		std::string test_dir=ANSItoUTF8(argv[1]);
		size_t size=test_dir.size();
		if(test_dir.at(size-2)=='\\')
			real_dir=test_dir.substr(0,size-2);
		else
			real_dir=test_dir.substr(0,size-1);
		w.set_dir(real_dir);
	}
	w.show();

	return a.exec();
}
