// AdaboostScaleDlg.cpp : 實作檔
//

#include "stdafx.h"
#include "video.h"
#include "AdaboostScaleDlg.h"
#include "afxdialogex.h"


// AdaboostScaleDlg 對話方塊

IMPLEMENT_DYNAMIC(AdaboostScaleDlg, CDialogEx)

AdaboostScaleDlg::AdaboostScaleDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(AdaboostScaleDlg::IDD, pParent)
{
	m_Up = 0;
	m_Bottom = 0;
}

AdaboostScaleDlg::~AdaboostScaleDlg()
{
}

void AdaboostScaleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
// 	DDX_Text(pDX,IDC_TEXT_UP,m_Up);
// 	DDX_Text(pDX,IDC_TEXT_BOTTOM,m_Bottom);
}


BEGIN_MESSAGE_MAP(AdaboostScaleDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &AdaboostScaleDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// AdaboostScaleDlg 訊息處理常式


afx_msg void AdaboostScaleDlg::OnBnClickedOk()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	CDialogEx::OnOK();
	char szText[10];
	CEdit *pUp = (CEdit*)(GetDlgItem(IDC_TEXT_UP));
	CEdit *pBottom = (CEdit*)(GetDlgItem(IDC_TEXT_BOTTOM));
	pUp->GetWindowText(szText,10);
	m_Up = atoi(szText);
	pBottom->GetWindowText(szText,10);
	m_Bottom = atoi(szText);
	//m_Up = atoi()
	//CDialogEx::DoDataExchange(pDX);
}

INT_PTR AdaboostScaleDlg::DoModal()
{
	// TODO: 在此加入特定的程式碼和 (或) 呼叫基底類別

	return CDialogEx::DoModal();
}

int AdaboostScaleDlg::GetUpBound(){
	return m_Up;
}

int AdaboostScaleDlg::GetBottomBound(){
	return m_Bottom;
}