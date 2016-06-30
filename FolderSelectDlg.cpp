// FolderSelectDlg.cpp : implementation file
//

#include "stdafx.h"

#include "FolderSelectDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Image indexes
#define ILI_HARD_DISK			0
#define ILI_NET_DRIVE			0
#define ILI_RAM_DRIVE			0
#define ILI_FLOPPY				1
#define ILI_CD_ROM				2
#define ILI_CLOSED_FOLDER		3
#define ILI_OPEN_FOLDER			4


/////////////////////////////////////////////////////////////////////////////
// CFolderSelectDlg dialog


CFolderSelectDlg::CFolderSelectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFolderSelectDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFolderSelectDlg)
	m_Path = _T("");
	//}}AFX_DATA_INIT
}


void CFolderSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFolderSelectDlg)
	DDX_Text(pDX, IDC_PATH, m_Path);
	DDV_MaxChars(pDX, m_Path, 512);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFolderSelectDlg, CDialog)
	//{{AFX_MSG_MAP(CFolderSelectDlg)
	ON_NOTIFY(TVN_ITEMEXPANDING, IDC_DIRECTORY, OnItemexpandingDirectory)
	ON_NOTIFY(TVN_SELCHANGED, IDC_DIRECTORY, OnSelchangedDirectory)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFolderSelectDlg message handlers

int CFolderSelectDlg::InitDirTree()
{
	int nPos = 0,
		nDrivesAdded = 0;
	CString strDrive = "?:\\";

	DWORD dwDriveList = ::GetLogicalDrives();

	while(dwDriveList)
	{
		if(dwDriveList & 1)
		{
			strDrive.SetAt(0, 0x41 + nPos);
			if(AddDriveNode(strDrive))
				nDrivesAdded++;
		}
		dwDriveList >>= 1;
		nPos++;
	}
	strDrive.Empty();
	return(nDrivesAdded);
}

BOOL CFolderSelectDlg::AddDriveNode(CString & strDrive)
{
	CString string;
	HTREEITEM hItem;
	static BOOL bFirst = TRUE;
	CTreeCtrl *tree_ctrl = (CTreeCtrl*)GetDlgItem(IDC_DIRECTORY);
	UINT nType = ::GetDriveType((LPCTSTR)strDrive);

	switch(nType)
	{
    case DRIVE_REMOVABLE:
        hItem = tree_ctrl->InsertItem(strDrive, ILI_FLOPPY, ILI_FLOPPY);
		tree_ctrl->InsertItem("", ILI_CLOSED_FOLDER, ILI_CLOSED_FOLDER, hItem);
		break;

	case DRIVE_FIXED:
		hItem = tree_ctrl->InsertItem(strDrive, ILI_HARD_DISK, ILI_HARD_DISK);
		SetButtonState(hItem, strDrive);

		// If this is the first fixed disk, select and expand it
		if(bFirst && 0)
		{
			tree_ctrl->SelectItem(hItem);
			tree_ctrl->Expand(hItem, TVE_EXPAND);
			bFirst = FALSE;
		}
		break;

	case DRIVE_REMOTE:
		hItem = tree_ctrl->InsertItem(strDrive, ILI_NET_DRIVE, ILI_NET_DRIVE);
		SetButtonState(hItem, strDrive);
		break;

	case DRIVE_CDROM:
		hItem = tree_ctrl->InsertItem(strDrive, ILI_CD_ROM, ILI_CD_ROM);
		tree_ctrl->InsertItem("", ILI_CLOSED_FOLDER, ILI_CLOSED_FOLDER, hItem);
		break;

	case DRIVE_RAMDISK:
		hItem = tree_ctrl->InsertItem(strDrive, ILI_RAM_DRIVE, ILI_RAM_DRIVE);
		SetButtonState(hItem, strDrive);
		break;

	default:
		string.Empty();
		return(FALSE);
    }

	string.Empty();
	return(TRUE);
}

BOOL CFolderSelectDlg::SetButtonState(HTREEITEM hItem, CString & strPath)
{
	HANDLE hFind;
	WIN32_FIND_DATA fd;
	BOOL bResult = FALSE;
	CTreeCtrl *tree_ctrl = (CTreeCtrl*)GetDlgItem(IDC_DIRECTORY);

	CString strCmp;
	CString string = strPath;
	if(string.Right(1) != "\\")
		string += "\\";
	string += "*.*";

	if((hFind = ::FindFirstFile((LPCTSTR)string, &fd)) == INVALID_HANDLE_VALUE)
		return(bResult);

	do{
		if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			strCmp = (LPCTSTR)&fd.cFileName;
			if((strCmp != ".") && (strCmp != ".."))
			{
				tree_ctrl->InsertItem("", ILI_CLOSED_FOLDER, ILI_CLOSED_FOLDER, hItem);
				bResult = TRUE;
				break;
			}
		}
	}while(::FindNextFile(hFind, &fd));

	//::CloseHandle (hFind);
	::FindClose(hFind);
	string.Empty();
	return(bResult);
}

int CFolderSelectDlg::AddDirectories(HTREEITEM hItem, CString & strPath)
{
	HANDLE hFind;
	WIN32_FIND_DATA fd;
	HTREEITEM hNewItem;
	CTreeCtrl *tree_ctrl = (CTreeCtrl*)GetDlgItem(IDC_DIRECTORY);

	int nCount = 0;
	CString strCmp, strNewPath;
	CString string = strPath;
	if(string.Right(1) != "\\")
		string += "\\";
	string += "*.*";

	if((hFind = ::FindFirstFile((LPCTSTR)string, &fd)) == INVALID_HANDLE_VALUE)
	{
		if(tree_ctrl->GetParentItem(hItem) == NULL)
			tree_ctrl->InsertItem("", ILI_CLOSED_FOLDER, ILI_CLOSED_FOLDER, hItem);
		return(0);
	}

	do{
        if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			strCmp = (LPCTSTR)&fd.cFileName;
			if((strCmp != ".") && (strCmp != ".."))
			{
				hNewItem = tree_ctrl->InsertItem((LPCTSTR)&fd.cFileName, ILI_CLOSED_FOLDER, ILI_OPEN_FOLDER, hItem);
				strNewPath = strPath;
				if(strNewPath.Right(1) != "\\")
					strNewPath += "\\";

				strNewPath += (LPCTSTR)&fd.cFileName;
				SetButtonState(hNewItem, strNewPath);
				nCount++;
			}
		}
	}while(::FindNextFile(hFind, &fd));

	//::CloseHandle (hFind);
	::FindClose(hFind);
	string.Empty();
	return(nCount);
}

void CFolderSelectDlg::DeleteFirstChild(HTREEITEM hParent)
{
	HTREEITEM hItem;
	CTreeCtrl *tree_ctrl = (CTreeCtrl*)GetDlgItem(IDC_DIRECTORY);

	if((hItem = tree_ctrl->GetChildItem(hParent)) != NULL)
		tree_ctrl->DeleteItem(hItem);
}

void CFolderSelectDlg::DeleteAllChildren(HTREEITEM hParent)
{
	HTREEITEM hItem;
	CTreeCtrl *tree_ctrl = (CTreeCtrl*)GetDlgItem(IDC_DIRECTORY);

	if((hItem = tree_ctrl->GetChildItem(hParent)) == NULL)
		return;

	HTREEITEM hNextItem;
	do{
		hNextItem = tree_ctrl->GetNextSiblingItem (hItem);
		tree_ctrl->DeleteItem(hItem);
		hItem = hNextItem;
	}while(hItem != NULL);
}

CString CFolderSelectDlg::GetPathFromNode(HTREEITEM hItem)
{
	CTreeCtrl *tree_ctrl = (CTreeCtrl*)GetDlgItem(IDC_DIRECTORY);
	CString strResult = tree_ctrl->GetItemText (hItem);

	HTREEITEM hParent;
	CString string;
	while((hParent = tree_ctrl->GetParentItem(hItem)) != NULL)
	{
		string = tree_ctrl->GetItemText(hParent);
		if(string.Right(1) != "\\")
			string += "\\";
		strResult = string + strResult;
		hItem = hParent;
	}
	return(strResult);
}

void CFolderSelectDlg::OnItemexpandingDirectory(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CTreeCtrl *tree_ctrl = (CTreeCtrl*)GetDlgItem(IDC_DIRECTORY);
	NM_TREEVIEW *pnmtv = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM hItem = pnmtv->itemNew.hItem;
	CString string = GetPathFromNode(hItem);

	*pResult = FALSE;
	if(pnmtv->action == TVE_EXPAND)
	{
		DeleteFirstChild(hItem);
		if(AddDirectories(hItem, string) == 0)
			*pResult = TRUE;
	}
	else
	{
		// pnmtv->action == TVE_COLLAPSE
		DeleteAllChildren(hItem);
		if(tree_ctrl->GetParentItem(hItem) == NULL)
			tree_ctrl->InsertItem("", ILI_CLOSED_FOLDER, ILI_CLOSED_FOLDER, hItem);
		else
			SetButtonState(hItem, string);
	}

	*pResult = 0;
}

void CFolderSelectDlg::OnSelchangedDirectory(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW *pnmtv = (NM_TREEVIEW*)pNMHDR;
	m_Path = GetPathFromNode(pnmtv->itemNew.hItem);
//	OnSelectionChanged(m_Path);
	UpdateData(FALSE);
	*pResult = 0;
}

BOOL CFolderSelectDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CTreeCtrl *tree_ctrl = (CTreeCtrl*)GetDlgItem(IDC_DIRECTORY);
	
	m_imglDrives.Create(IDR_DRIVEIMAGES, 16, 1, RGB(255, 0, 255));
	tree_ctrl->SetImageList(&m_imglDrives, TVSIL_NORMAL);
	InitDirTree();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CFolderSelectDlg::PreCreateWindow(CREATESTRUCT& cs) 
{
	if(!CDialog::PreCreateWindow(cs))
		return(FALSE);

	cs.style |= TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS;
	return(TRUE);
}

void CFolderSelectDlg::GetSubFolderFileNameList(CString SubFolder, CString *FileList, int &Index)
{
	int i, CurrentSubFolders;

	BOOL	ret;
	CFileFind File_Finder;

	CString *DirectoryList, Folder_Name, File_Path, File_SubName;

	CurrentSubFolders = FindSubDirectories(SubFolder, &DirectoryList);

	// 預掃瞄建立資料夾內所有檔案數量
	for(i=0;i<CurrentSubFolders;i++)
		GetSubFolderFileNameList(DirectoryList[i], FileList, Index);

	// 根目錄
	Folder_Name = SubFolder + _T("\\*.*");
	ret = File_Finder.FindFile(Folder_Name);
	while(ret)
	{
		ret = File_Finder.FindNextFile();
		File_Path = File_Finder.GetFilePath();
		File_SubName = File_Path.Right(3);
		File_SubName.MakeLower();

		if(File_SubName == "bmp" || File_SubName == "jpg")
			FileList[Index++] = File_Path;
	}

	File_Finder.Close();

	if(DirectoryList)
		delete [] DirectoryList;
	DirectoryList = NULL;
}

void CFolderSelectDlg::GetSubFolderFiles(CString SubFolder, int &Files, int &Folders)
{
	int i, TempFiles, TempFolders,
		CurrentFiles = 0,
		CurrentSubFolders = 0,
		SubFiles = 0,
		SubFolders = 0;

	BOOL	ret;
	CFileFind File_Finder;

	CString *DirectoryList, Folder_Name, File_Path, File_SubName;

	CurrentSubFolders = FindSubDirectories(SubFolder, &DirectoryList);

	// 預掃瞄建立資料夾內所有檔案數量
	for(i=0;i<CurrentSubFolders;i++)
	{
		GetSubFolderFiles(DirectoryList[i], TempFiles, TempFolders);

		SubFiles += TempFiles;
		SubFolders += TempFolders;
	}

	// 根目錄
	Folder_Name = SubFolder + _T("\\*.*");
	ret = File_Finder.FindFile(Folder_Name);
	while(ret)
	{
		ret = File_Finder.FindNextFile();
		File_Path = File_Finder.GetFilePath();
		File_SubName = File_Path.Right(3);
		File_SubName.MakeLower();

		if(File_SubName == "bmp" || File_SubName == "jpg")
			CurrentFiles++;
	}

	File_Finder.Close();

	Files = CurrentFiles + SubFiles;
	Folders = CurrentSubFolders + SubFolders;

	if(DirectoryList)
		delete [] DirectoryList;
	DirectoryList = NULL;
}

int CFolderSelectDlg::FindSubDirectories(CString path, CString **DirectoryList)
{
	CString string, *temp, *s;
	HANDLE hFind;
	WIN32_FIND_DATA fd;
	int i, nCount;

	string = path;
	temp = new CString[1000];

	if(string.Right(1) != "\\")
		string += "\\";
	string += "*.*";

	if((hFind = ::FindFirstFile((LPCTSTR)string, &fd)) == INVALID_HANDLE_VALUE)
	{
		AfxMessageBox("Errors for Database Selections！");
		return(-1);
	}

	nCount = 0;
	do{
        if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
            CString strCmp = (LPCTSTR)&fd.cFileName;
            if((strCmp != ".") && (strCmp != ".."))
			{
                string = path;
				if(string.Right(1) != "\\")
					string += "\\";
				string += (LPCTSTR)&fd.cFileName;
				temp[nCount] = string;
                nCount++;
            }
        }
    }while(::FindNextFile(hFind, &fd));

	::FindClose(hFind);
	s = new CString[nCount + 1];
	for(i=0;i<nCount;i++)
		s[i] = temp[i];

	*DirectoryList = s;
	delete [] temp;
	return(nCount);
}
