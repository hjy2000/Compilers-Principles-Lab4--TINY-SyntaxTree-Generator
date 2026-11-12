
// TINYDlg.cpp: 实现文件
//

//#include "stdafx.h"
#include "TINY.h"
#include "TINYDlg.h"
#include "afxdialogex.h"
#include "analyse.h"
#include <iostream>
#include <fstream>
#include <cstring>
//#include "stdafx.h"
//#include "pch.h"

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTINYDlg 对话框



CTINYDlg::CTINYDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TINY_DIALOG, pParent)
	, m_file_path(_T(""))
	, m_input(_T(""))
	, m_output(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

// 临时文件放在系统临时目录，避免程序安装在只读目录时无法写入
static string str;  /* 语法树/错误信息输出文件 */
static string temp; /* 待分析的源程序临时文件 */

/* 把宽字符串转换为指定代码页的多字节串。
   这里直接调用 Win32 API，避免依赖 ATL 转换宏的头文件包含顺序 */
static string narrow(LPCTSTR s, UINT codePage)
{
#ifdef UNICODE
	int n = WideCharToMultiByte(codePage, 0, s, -1, NULL, 0, NULL, NULL);
	if (n <= 0) return string();
	char* buf = new char[n];
	WideCharToMultiByte(codePage, 0, s, -1, buf, n, NULL, NULL);
	string r(buf);
	delete[] buf;
	return r;
#else
	(void)codePage;
	return string(s);
#endif
}

/* 取得临时目录下的完整文件路径（ANSI 版本，供 fopen 使用）。
   临时目录若含中文（例如用户名），取其 8.3 短路径，
   保证 fopen 在默认代码页下也能访问 */
static string tempFilePath(LPCTSTR fileName)
{
	TCHAR buf[MAX_PATH] = { 0 };
	GetTempPath(MAX_PATH, buf);

	TCHAR shortDir[MAX_PATH] = { 0 };
	CString path;
	if (GetShortPathName(buf, shortDir, MAX_PATH) > 0)
		path = shortDir;
	else
		path = buf;
	if (!path.IsEmpty() && path[path.GetLength() - 1] != _T('\\'))
		path += _T('\\');
	path += fileName;
	return narrow(path, CP_ACP);
}

/* 把编辑框内容以 UTF-8 写入文件，使中文注释不会乱码 */
static int writeEditToFile(const CString& text, const string& path)
{
	ofstream file(path.c_str(), ios::binary);
	if (!file) return -1;
	string utf8 = narrow(text, CP_UTF8);
	file.write(utf8.data(), (streamsize)utf8.size());
	file.close();
	return 0;
}

void CTINYDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRAM, program);
	DDX_Control(pDX, IDC_GENTREE, m_gentree);
	DDX_Text(pDX, IDC_FILE, m_file_path);
	DDX_Text(pDX, IDC_PROGRAM, m_input);
	DDX_Text(pDX, IDC_SYTAX, m_output);
}

BEGIN_MESSAGE_MAP(CTINYDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_OPEN, &CTINYDlg::OnBnClickedOpen)
	ON_BN_CLICKED(IDC_SAVE, &CTINYDlg::OnBnClickedSave)
	ON_BN_CLICKED(IDC_CREATE, &CTINYDlg::OnBnClickedCreate)
	ON_BN_CLICKED(IDC_SHOW, &CTINYDlg::OnBnClickedShow)
END_MESSAGE_MAP()


// CTINYDlg 消息处理程序

BOOL CTINYDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	m_file_path = _T("");
	m_input = _T("");
	m_output = _T("");
	m_gentree.SetCheck(BST_CHECKED); /* 缺省生成语法树 */

	/* 初始化临时文件路径 */
	str = tempFilePath(_T("TINY_Atree.out"));
	temp = tempFilePath(_T("TINY_temp.tny"));

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CTINYDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CTINYDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CTINYDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CTINYDlg::OnBnClickedOpen()
{
	// TODO: 在此添加控件通知处理程序代码
	CString path;
	CFileDialog dlg(TRUE);
	dlg.m_ofn.lpstrTitle = L"打开TINY源程序";
	dlg.m_ofn.lpstrFilter = L"TINY Source Files(*.tny)\0*.tny\0Text Files(*.txt)\0*.txt\0All Files(*.*)\0*.*\0\0";
	dlg.m_ofn.lpstrDefExt = L"tny";
	if (dlg.DoModal() == IDOK)
	{
		path = dlg.GetPathName();
		CFile file;
		if (!file.Open(path, CFile::modeRead))
		{
			MessageBox(L"无法打开文件");
			return;
		}
		ULONGLONG len = file.GetLength();
		string content;
		if (len > 0)
		{
			char* buf = new char[(size_t)len + 1];
			UINT read = file.Read(buf, (UINT)len);
			buf[read] = '\0';
			content.assign(buf, read);
			delete[] buf;
		}
		file.Close();

		/* 源程序优先按 UTF-8 读入（与本程序保存格式一致）；
		   若不是合法 UTF-8，则按系统 ANSI 编码解释，以兼容 GBK 文件 */
		CString text;
		int need = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			content.c_str(), (int)content.size(), NULL, 0);
		if (need > 0)
		{
			wchar_t* wbuf = new wchar_t[need + 1];
			MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
				content.c_str(), (int)content.size(), wbuf, need);
			wbuf[need] = L'\0';
			text = wbuf;
			delete[] wbuf;
		}
		else
		{
			text = CString(content.c_str());
		}

		/* 统一换行为 CRLF，便于在多行编辑框中显示 */
		text.Replace(_T("\r\n"), _T("\n"));
		text.Replace(_T("\n"), _T("\r\n"));

		m_file_path = path;
		m_input = text;
		UpdateData(false);
	}
	else
	{
		//MessageBox(L"文件选择窗体创建失败");
		return;
	}
}


void CTINYDlg::OnBnClickedSave()
{
	// TODO: 在此添加控件通知处理程序代码
	CFileDialog dlg(FALSE);
	dlg.m_ofn.lpstrTitle = L"保存TINY源程序";
	dlg.m_ofn.lpstrFilter = L"TINY Source Files(*.tny)\0*.tny\0Text Files(*.txt)\0*.txt\0All Files(*.*)\0*.*\0\0";
	dlg.m_ofn.lpstrDefExt = L"tny";
	if (IDOK == dlg.DoModal())
	{
		CString path = dlg.GetPathName();
		program.GetWindowTextW(m_input);
		/* 以 UTF-8 无 BOM 保存，避免中文注释乱码 */
		CFile file;
		if (!file.Open(path, CFile::modeCreate | CFile::modeWrite))
		{
			MessageBox(L"文件保存失败");
			return;
		}
		string utf8 = narrow(m_input, CP_UTF8);
		file.Write(utf8.data(), (UINT)utf8.size());
		file.Close();
		m_file_path = path;
		UpdateData(false);
	}
}

void CTINYDlg::OnBnClickedCreate()
{
	// TODO: 在此添加控件通知处理程序代码
	program.GetWindowTextW(m_input);
	if (writeEditToFile(m_input, temp) != 0)
	{
		MessageBox(L"无法写入临时文件");
		return;
	}

	bool genTree = (m_gentree.GetCheck() == BST_CHECKED);
	int ret = analyse(temp, str, genTree);  //输入路径，输出文件名，是否生成语法树

	if (ret == 0)
	{
		if (genTree) MessageBox(L"分析成功，已生成语法树，请点击“查看语法树”");
		else MessageBox(L"分析成功（未生成语法树）");
	}
	else
	{
		m_output = _T("分析失败，请点击“查看语法树”查看错误信息");
		UpdateData(false);
		MessageBox(L"分析失败，请检查源程序");
	}
}


void CTINYDlg::OnBnClickedShow()
{
	// TODO: 在此添加控件通知处理程序代码

	ifstream file(str.c_str());
	//ifstream file(file_path.c_str());
	if (!file) return;
	char buffer[1024];
	string tree = "";
	while (file.getline(buffer, 1024))
	{
		tree += buffer;
		tree += "\r\n";
	}
	m_output = tree.c_str();

	file.close();
	UpdateData(false);
}
