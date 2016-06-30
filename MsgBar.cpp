// MsgBar.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "MsgBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define _ERROR_COLOR	RGB(255, 0, 0)
#define _COLOE_MEGENTA	RGB(255, 0, 255)
#define _COLOR_BLACK	RGB(0, 0, 0)
#define _COLOR_NORMAL	RGB(0, 0, 255)

/////////////////////////////////////////////////////////////////////////////
// CMsgBar

CMsgBar::CMsgBar()
{
}

CMsgBar::~CMsgBar()
{
}


BEGIN_MESSAGE_MAP(CMsgBar, CSizingControlBarG)
	//{{AFX_MSG_MAP(CMsgBar)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMsgBar message handlers

int CMsgBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CSizingControlBarG::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// TODO: Add your specialized creation code here
	m_richEdit.Create(ES_AUTOVSCROLL | ES_MULTILINE | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
			CRect(0,0,100,100), this, IDC_RICHEDIT);	

	SetSCBStyle(SCBS_SHOWEDGES | SCBS_SIZECHILD);
	m_richEdit.ModifyStyleEx(0, WS_EX_CLIENTEDGE); // 加邊框
	m_richEdit.SetReadOnly(); // 設定read only

	Format(0xff0000, "Courier New");
	m_richEdit.SetDefaultCharFormat(m_charFormat);

// 	char str[300];
// 	sprintf(str, "System messages will be shown here...C.C. Lee\n");
// 	m_richEdit.SetWindowText(str);
	return 0;
}

void CMsgBar::ShowMessage(LPCTSTR lpszFormat, ...)
{
	if(this == NULL)	return;
	if(::IsWindow(m_hWnd) == FALSE)	return;

	CString strLineMsg;
	va_list argList;
	va_start(argList, lpszFormat);
	strLineMsg.FormatV(lpszFormat, argList);
	va_end(argList); // 以上這幾行是用來處理類似printf 的參數
//	CString buf;
//	buf.Format( "%s", strLineMsg );

	m_richEdit.SetSel(-1 , -1);
	m_richEdit.ReplaceSel( strLineMsg);
	m_richEdit.LineScroll(1); // 讓scroll bar 在最下面
}

void CMsgBar::error_msg(LPCTSTR lpszFormat, ...)
{
	if(this == NULL)	return;
	if(::IsWindow(m_hWnd) == FALSE)	return;

	CString strLineMsg;
	va_list argList;
	va_start(argList, lpszFormat);
	strLineMsg.FormatV(lpszFormat, argList);
	va_end(argList);

	CString  linebuf;
	long len, linecount;

	len = m_richEdit.GetTextLength();
	linecount = m_richEdit.GetLineCount();
	m_richEdit.GetLine(linecount-1, linebuf.GetBuffer(500), 490);
	linebuf.ReleaseBuffer( );

//	buf.Format( "%s", strLineMsg );


	
	COLORREF  old_color = m_charFormat.crTextColor;

	m_charFormat.crTextColor = _ERROR_COLOR;

	m_richEdit.SetSelectionCharFormat(m_charFormat);
	m_richEdit.SetSel(len , len);
	m_richEdit.ReplaceSel( strLineMsg);
	m_richEdit.LineScroll(1);

	m_charFormat.crTextColor = old_color;


}

void CMsgBar::Format(COLORREF color, char *font)
{
	int fontsize = 9;
	m_richEdit.GetDefaultCharFormat(m_charFormat);

    m_charFormat.dwMask |=  CFM_COLOR | CFM_FACE | CFM_SIZE;
	m_charFormat.dwEffects = 0;
 
	m_charFormat.yHeight = fontsize * 20;
    m_charFormat.crTextColor = color;
 
    m_charFormat.bCharSet = 0;
    m_charFormat.bPitchAndFamily = 0;
    strcpy(m_charFormat.szFaceName, font);
	
	
}


BOOL CMsgBar::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
    if(pMsg->message==WM_KEYDOWN)
	{
	   if( pMsg->wParam=='C') {
		   if( GetKeyState(VK_CONTROL) >> 8) {
				m_richEdit.Copy( );	
				MessageBeep(MB_OK);
				return TRUE;
		   }
	   }
	}	
  
	return CSizingControlBarG::PreTranslateMessage(pMsg);
}

void CMsgBar::ClearMessage() 
{
//	m_nLine = 1;
	m_richEdit.SetWindowText("");
}

static DWORD CALLBACK 
MyStreamOutCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
   CFile* pFile = (CFile*) dwCookie;

   pFile->Write(pbBuff, cb);
   *pcb = cb;

   return 0;
}



void CMsgBar::WriteOut(char *filename)
{
	CFile cFile(TEXT(filename), 
	   CFile::modeCreate|CFile::modeWrite);
	EDITSTREAM es;

	es.dwCookie = (DWORD) &cFile;
	es.pfnCallback = MyStreamOutCallback; 
	m_richEdit.StreamOut(SF_TEXT, es);
	cFile.Close();
}