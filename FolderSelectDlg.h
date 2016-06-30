#if !defined(AFX_FOLDERSELECTDLG_H__8CF75BBC_7615_4E2B_BA69_002B3AC56187__INCLUDED_)
#define AFX_FOLDERSELECTDLG_H__8CF75BBC_7615_4E2B_BA69_002B3AC56187__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "Resource.h"
// FolderSelectDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFolderSelectDlg dialog

class CFolderSelectDlg : public CDialog
{
// Construction
public:
	CFolderSelectDlg(CWnd* pParent = NULL);   // standard constructor
	void		GetSubFolderFiles(CString SubFolder, int &Files, int &Folders);
	void		GetSubFolderFileNameList(CString SubFolder, CString *FileList, int &Index);
	CString		GetFolderName()	{return(m_Path);};

private:
	CImageList m_imglDrives;
	CString		GetPathFromNode(HTREEITEM hItem);
	void		DeleteAllChildren(HTREEITEM hParent);
	void		DeleteFirstChild(HTREEITEM hParent);
	int			AddDirectories(HTREEITEM hItem, CString& strPath);
	BOOL		SetButtonState(HTREEITEM hItem, CString& strPath);
	BOOL		AddDriveNode(CString& strDrive);
	int			InitDirTree();
	int			FindSubDirectories(CString path, CString **DirectoryList);

public:

// Dialog Data
	//{{AFX_DATA(CFolderSelectDlg)
	enum { IDD = IDD_DIALOG_FOLDERSELECT };
	CString	m_Path;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFolderSelectDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFolderSelectDlg)
	afx_msg void OnItemexpandingDirectory(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangedDirectory(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FOLDERSELECTDLG_H__8CF75BBC_7615_4E2B_BA69_002B3AC56187__INCLUDED_)
