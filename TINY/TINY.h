
// TINY.h: PROJECT_NAME 应用程序的主头文件
//

#pragma once
//#include <WinDef.h>
//#include <windows.h>
#include "stdafx.h"
#include "pch.h"
#ifndef __AFXWIN_H__
#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif


#include "Resource.h"		// 主符号


// CTINYApp:
// 有关此类的实现，请参阅 TINY.cpp
//

class CTINYApp : public CWinApp
{
public:
	CTINYApp();

	// 重写
public:
	virtual BOOL InitInstance();

	// 实现

	DECLARE_MESSAGE_MAP()
};

extern CTINYApp theApp;
