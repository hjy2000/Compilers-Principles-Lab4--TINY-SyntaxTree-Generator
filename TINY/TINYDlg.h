//mfc配置文件
// TINYDlg.h: 头文件
//

#pragma once
//#include <string>

// CTINYDlg 对话框
class CTINYDlg : public CDialogEx
{
	// 构造
public:
	CTINYDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TINY_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOpen();
	afx_msg void OnBnClickedSave();
	afx_msg void OnBnClickedCreate();
	afx_msg void OnBnClickedShow();
	// 用户的源程序编辑框
	CEdit program;
	// 路径
	CString m_file_path;
	// 输入程序
	CString m_input;
	// 输出
	CString m_output;
};
