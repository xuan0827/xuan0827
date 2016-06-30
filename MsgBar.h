#if !defined(AFX_MSGBAR_H__EC1D435B_309D_495F_A927_B2C23B4F1796__INCLUDED_)
#define AFX_MSGBAR_H__EC1D435B_309D_495F_A927_B2C23B4F1796__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MsgBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMsgBar window


class CMsgBar : public CSizingControlBarG
{
// Construction
public:
	CMsgBar();

// Attributes
private:
	CRichEditCtrl	m_richEdit;
	CHARFORMAT		m_charFormat;

// Operations
public:
	void ShowMessage(LPCTSTR lpszFormat, ...);
	void error_msg(LPCTSTR lpszFormat, ...);
	void Format(COLORREF color=0xff0000, char *font="Arial");
	void ClearMessage() ;
	void WriteOut(char *filename);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMsgBar)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMsgBar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMsgBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSGBAR_H__EC1D435B_309D_495F_A927_B2C23B4F1796__INCLUDED_)
