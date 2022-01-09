
// TINYDlg.cpp: 实现文件
//

//#include "stdafx.h"
#include "TINY.h"
#include "TINYDlg.h"
#include "afxdialogex.h"
#include "analyse.h"
#include <iostream>
#include <fstream>
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

void CTINYDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRAM, program);
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

string str = "./Atree.out";
string temp = "./temp.out";

void CTINYDlg::OnBnClickedOpen()
{
	// TODO: 在此添加控件通知处理程序代码
	CString path;
	CFileDialog dlg(TRUE);
	if (dlg.DoModal() == IDOK)
	{
		path = dlg.GetPathName();
		m_file_path = path;
		UpdateData(false);
		ifstream file(path);

		if (!file) return;

		char buffer[1024];
		string str = "";
		while (!file.eof())
		{
			file.getline(buffer, 1024);
			str += buffer;
			str += "\r\n";
		}
		m_input = str.c_str();
		file.close();
		UpdateData(false);
		//MessageBox(m_input);
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
	CString text, path, fileName = L"tempfile.txt";
	CFileDialog dlg(FALSE);
	dlg.m_ofn.lpstrTitle = L"我的文件保存对话框";
	dlg.m_ofn.lpstrFilter = L"Text Files(*.cpp)\0*.cpp\0All Files(*.*)\0*.*\0\0";
	dlg.m_ofn.lpstrDefExt = L"txt";
	if (IDOK == dlg.DoModal())
	{
		path = dlg.GetPathName();
		ofstream file(path);
		UpdateData(true);
		program.GetWindowTextW(m_input);
		for (int i = 0; i < m_input.GetLength(); i++)
			if (m_input[i] != '\r')
				file.put(m_input[i]);
		file.close();
	}
}

void CTINYDlg::OnBnClickedCreate()
{
	// TODO: 在此添加控件通知处理程序代码
	ofstream file(temp);
	program.GetWindowTextW(m_input);
	for (int i = 0; i < m_input.GetLength(); i++)
		if (m_input[i] != '\r')
			file.put(m_input[i]);
	file.close();
	analyse(temp, str);  //输入路径，输出文件名
	MessageBox(L"生成成功");
}


void CTINYDlg::OnBnClickedShow()
{
	// TODO: 在此添加控件通知处理程序代码

	ifstream file(str.c_str());
	//ifstream file(file_path.c_str());
	if (!file) return;
	char buffer[1024];
	string tree = "";
	while (!file.eof())
	{
		file.getline(buffer, 1024);
		tree += buffer;
		tree += "\r\n";
	}
	m_output = tree.c_str();

	file.close();
	UpdateData(false);
}
