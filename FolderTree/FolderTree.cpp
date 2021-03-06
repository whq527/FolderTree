// FolderTree.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <afx.h>
#include <vector>
#include "md5.h"

using namespace std;
const CString V_PlaceHolder = "|   ";
const CString H_PlaceHolder = "|----";
#define NEEDWRITETIME 0x1
#define NEEDFILTER 0x2
#define NEEDEXCLUDE 0x4
#define NEEDMD5 0x8
#define NEEDONLYRESULT 0x10
#define NEEDSEPARATOR 0x20

struct st_SubFolder
{
	CString Name = "";
	CString Path = "";
};

bool CheckFilter(CString _filename, CString _filter)
{
	if (_filter == "")
		return true;

	CString extension = _filename.Right(_filename.GetLength() - _filename.ReverseFind('.'));
	if (_filter.Find(extension) != -1)
		return true;
	else
		return false;
}

bool CheckExclude(CString _filename, CString _exclude)
{
	if (_exclude == "")
		return true;

	CString extension = _filename.Right(_filename.GetLength() - _filename.ReverseFind('.'));
	if (_exclude.Find(extension) != -1)
		return false;
	else
		return true;
}

//_path路径 _nTier层级 _outstr输出 _filter过滤 _exclude排除 _flsnum文件数 _fldnum文件夹数
bool TreeFolder(CString _path, int _nTier, CString &_outstr, int _arg, CString _filter, CString _exclude, int &_flsnum, int &_fldnum)
{
	bool bHasFile = false;//判断是否有文件, 回报上层
	CString outstr = "";//记录当前层的数据
	vector<st_SubFolder> subfolderlist;//缓存子文件夹列表

	_path += "//*.*";
	CFileFind filefind;
	BOOL bHasThings = filefind.FindFile(_path);  // 查文件和文件夹
	while (bHasThings)
	{
		bHasThings = filefind.FindNextFile();
		if (!filefind.IsDots())//排除.
		{
			if (filefind.IsDirectory()) // 目录暂时缓存, 当前的子目录放在当前层的最后
			{
				st_SubFolder subfolder;
				subfolder.Name = filefind.GetFileName();
				subfolder.Path = filefind.GetFilePath();
				subfolderlist.push_back(subfolder);
			}
			else //处理文件
			{
				if (CheckExclude(filefind.GetFileName(), _exclude))//排除
				{
					if (CheckFilter(filefind.GetFileName(), _filter))//过滤
					{
						//画线
						for (int i = 1; i <= _nTier; i++)
						{
							outstr += V_PlaceHolder;
						}
						outstr += H_PlaceHolder;

						CString StrEx = ""; //额外信息
						char separator = ' ';
						if ((_arg & NEEDSEPARATOR) == NEEDSEPARATOR)//替换分隔符
							separator = '|';

						if ((_arg & NEEDWRITETIME) == NEEDWRITETIME)
						{
							CTime tempTime;
							filefind.GetLastWriteTime(tempTime);
							StrEx += separator;
							StrEx += tempTime.Format(_T("%F"));//获取文件修改日期
							StrEx += separator;
							StrEx += tempTime.Format(_T("%T"));//获取文件修改时间
						}

						if ((_arg & NEEDMD5) == NEEDMD5)
						{
							StrEx += separator;
							ifstream in = ifstream(filefind.GetFilePath().GetBuffer(), ios::binary);//获取MD5
							StrEx += MD5(in).toString().c_str();
						}
						outstr.AppendFormat("%s %s\n", filefind.GetFileName(), StrEx); //附加文件和附加数据
						_flsnum++;//统计文件数
						bHasFile = true;
					}
				}
			}
		}
	}
	if (subfolderlist.size() > 0)//子文件夹递归
	{
		for (auto var : subfolderlist)
		{
			CString substr;//获取子层的数据
			bool bSubFolderHasFiles = TreeFolder(var.Path, _nTier + 1, substr, _arg, _filter, _exclude, _flsnum, _fldnum);

			if ((_arg & NEEDONLYRESULT) == NEEDONLYRESULT && !bSubFolderHasFiles)//只显示有结果的子目录
				continue;

			bHasFile = bSubFolderHasFiles;//如果最下层有数据覆盖本层的判断

			//画线
			for (int i = 1; i <= _nTier; i++)
			{
				outstr += V_PlaceHolder;
			}
			outstr += H_PlaceHolder;
			outstr.AppendFormat("[%s]\n", var.Name); //附加子目录

			_fldnum++; //统计文件夹数
			outstr += substr; //补子层的数据
		}
	}

	filefind.Close();
	_outstr = outstr;//返回结果
	return bHasFile;
}

int main(int argc, CHAR* argv[], CHAR* envp[])
{
	// 得到当前exe的目录
	CString modulePath;
	GetModuleFileName(NULL, modulePath.GetBuffer(MAX_PATH), MAX_PATH);
	modulePath.ReleaseBuffer();
	int nLength = modulePath.GetLength();
	int iPos = modulePath.ReverseFind('/');

	if (argc < 2)
	{   //当前 exe的名字 + 一个示例
		int csize = 35;
		cout << setw(csize) << left << "eg: " << " FolderTree.exe \"F:\\folder\"" << endl;
		cout << setw(csize) << left << "eg change separator: " << " FolderTree.exe -s \"F:\\folder\"" << endl;
		cout << setw(csize) << left << "eg show write time: " << " FolderTree.exe -w \"F:\\folder\"" << endl;
		cout << setw(csize) << left << "eg show md5: " << " FolderTree.exe -m \"F:\\folder\"" << endl;
		cout << setw(csize) << left << "eg only show folder has files: " << " FolderTree.exe -o \"F:\\folder\"" << endl;
		cout << setw(csize) << left << "eg filter file extension: " << " FolderTree.exe -f \"F:\\folder\" \".exe;.zip\"" << endl;
		cout << setw(csize) << left << "eg exclude file extension: " << " FolderTree.exe -e \"F:\\folder\" \".exe\"" << endl;
		cout << setw(csize) << left << "eg: " << " FolderTree.exe -wf \"F:\\folder\" \".exe;.zip\"" << endl;
		system("pause");
		return 0;
	}
	// 输出结果
	modulePath = modulePath.Left(iPos + 1);
	CString outFileName = modulePath + "_Tree.txt";
	ofstream outFile(outFileName, ios::out);
	if (!outFile)  // 创建文件失败
	{
		cerr << "Can't Open write file:\n";
		system("pause");
		exit(1);
	}

	int flsnum = 0, fldnum = 0;
	int iArg = 0x0, argnum = 3;
	CString buf = "";
	if (argc == 2)
	{
		cout << argv[1] << endl; // 输出指定目录
		outFile << argv[1] << endl;
		TreeFolder(argv[1], 0, buf, 0x0, "", "", flsnum, fldnum); // 递归函数的调用
	}
	else if (argc > 2)
	{
		if (strchr(argv[1], 's') != NULL && argv[1][0] == '-')//需要额外数据替换空格分隔符,使用:
			iArg = iArg | NEEDSEPARATOR;
		if (strchr(argv[1], 'w') != NULL && argv[1][0] == '-')//需要最新修改时间
			iArg = iArg | NEEDWRITETIME;
		if (strchr(argv[1], 'm') != NULL && argv[1][0] == '-')//需要md5
			iArg = iArg | NEEDMD5;
		if (strchr(argv[1], 'o') != NULL && argv[1][0] == '-')//需要只存在有结果的目录
			iArg = iArg | NEEDONLYRESULT;
		if (strchr(argv[1], 'f') != NULL && argv[1][0] == '-')//过滤需要的文件后缀, 加一个参数
		{
			iArg = iArg | NEEDFILTER;
			argnum++;
		}
		if (strchr(argv[1], 'e') != NULL && argv[1][0] == '-')//排除需要的文件后缀, 加一个参数
		{
			iArg = iArg | NEEDEXCLUDE;
			argnum++;
		}
		if (argnum != argc)
		{
			cerr << "Param num wrong:\n";
			system("pause");
			exit(1);
		}

		if ((iArg & (NEEDFILTER | NEEDEXCLUDE)) == (NEEDFILTER | NEEDEXCLUDE))
		{
			cout << "Exclude file extension " << argv[argc - 1] << endl; // 输出排除参数
			outFile << "Exclude file extension " << argv[argc - 1] << endl;
			cout << "Filter file extension " << argv[argc - 2] << endl; // 输出过滤参数
			outFile << "Filter file extension " << argv[argc - 2] << endl;
		}
		else if ((iArg & NEEDFILTER) == NEEDFILTER)
		{
			cout << "Filter file extension " << argv[argc - 1] << endl; // 输出过滤参数
			outFile << "Filter file extension " << argv[argc - 1] << endl;
		}
		else if ((iArg & NEEDEXCLUDE) == NEEDEXCLUDE)
		{
			cout << "Exclude file extension " << argv[argc - 1] << endl; // 输出过滤参数
			outFile << "Exclude file extension " << argv[argc - 1] << endl;
		}

		cout << argv[2] << endl; // 输出指定目录
		outFile << argv[2] << endl;

		if ((iArg & (NEEDFILTER | NEEDEXCLUDE)) == (NEEDFILTER | NEEDEXCLUDE))
		{
			TreeFolder(argv[2], 0, buf, iArg, argv[argc - 2], argv[argc - 1], flsnum, fldnum);
		}
		else if ((iArg & NEEDFILTER) == NEEDFILTER)
		{
			TreeFolder(argv[2], 0, buf, iArg, argv[argc - 1], "", flsnum, fldnum);
		}
		else if ((iArg & NEEDEXCLUDE) == NEEDEXCLUDE)
		{
			TreeFolder(argv[2], 0, buf, iArg, "", argv[argc - 1], flsnum, fldnum);
		}
		else
		{
			TreeFolder(argv[2], 0, buf, iArg, "", "", flsnum, fldnum);
		}
	}

	//输出
	if (!buf.IsEmpty())
	{
		cout << (LPCTSTR)buf;
		outFile << (LPCTSTR)buf;
	}

	cout << endl << "All folder num " << fldnum << " All files num " << flsnum << endl; // 输出文件统计
	outFile << endl << "All folder num " << fldnum << " All files num " << flsnum << endl;
	outFile.close();

	system("pause");
	return 0;
}