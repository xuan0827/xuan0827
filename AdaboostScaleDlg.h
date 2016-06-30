#pragma once


// AdaboostScaleDlg 對話方塊

class AdaboostScaleDlg : public CDialogEx
{
	DECLARE_DYNAMIC(AdaboostScaleDlg)

public:
	AdaboostScaleDlg(CWnd* pParent = NULL);   // 標準建構函式
	virtual ~AdaboostScaleDlg();

// 對話方塊資料
	enum { IDD = IDD_DIALOG_ADABOOST_SCALE };
	int m_Up;
	int m_Bottom;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援
	//virtual INT_PTR DoModal();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	virtual INT_PTR DoModal();
	int GetUpBound();
	int GetBottomBound();
};
