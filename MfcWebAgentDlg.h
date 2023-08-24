// MfcWebAgentDlg.h : header file
//

#if !defined(AFX_MFCWEBAGENTDLG_H__00AABA48_186C_4344_AB8A_AE21A528EFEB__INCLUDED_)
#define AFX_MFCWEBAGENTDLG_H__00AABA48_186C_4344_AB8A_AE21A528EFEB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CMfcWebAgentDlg dialog

class CMfcWebAgentDlg : public CDialog
{
// Construction
public:
	void UpdateWindowSurface();
	void InitSettings();
	CMfcWebAgentDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMfcWebAgentDlg)
	enum { IDD = IDD_MFCWEBAGENT_DIALOG };
	CButton	m_MailerBtn;
	CComboBox	m_ClickModeList;
	CComboBox	m_CodePageList;
	CButton	m_StartBtn;
	CButton	m_StopBtn;
	CStatic	m_TimerStatus;
	CComboBox	m_StatusList;
	CComboBox	m_ParamList;
	CString	m_URL;
	CString	m_Login;
	CString	m_Password;
	CString	m_Name;
	CString	m_ScanInterval;
	CString	m_BeginTag;
	CString	m_EndTag;
	CString	m_ShowKeyTags;
	CString	m_HistorySize;
	CString	m_SearchKeyTags;
	CString	m_LastDateScan;
	CString	m_ClickCmdTags;
	CString	m_ImageUrlTags;
	CString	m_PauseAfterScan;
	CString	m_ScanStatus;
	CString	m_HistoryCnt;
	CString	m_BrowserName;
	CString	m_LastLoadedCnt;
	CString	m_DontChangeCnt;
	CString	m_LastChangeDate;
	BOOL	m_ShowDontChanged;
	CString	m_ShowLimitDetect;
	CString	m_FilterKeyTags;
	BOOL	m_UseFilter;
	CString	m_FilterWords;
	BOOL	m_UseMailingTimer;
	CString	m_MailingOnTime;
	CString	m_MailingOffTime;
	BOOL	m_MailingDay1;
	BOOL	m_MailingDay2;
	BOOL	m_MailingDay3;
	BOOL	m_MailingDay4;
	BOOL	m_MailingDay5;
	BOOL	m_MailingDay6;
	BOOL	m_MailingDay7;
	CString	m_MailingEmail;
	CString	m_MailingServer;
	CString	m_MailingLogin;
	CString	m_MailingPassword;
	CString	m_MailKeyTags;
	CString	m_MailingAuth;
	CString	m_MailingDestMail;
	CString	m_MailingInterval;
	CString	m_MailingMessCnt;
	CString	m_EndPageKey;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMfcWebAgentDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CMfcWebAgentDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnCancel();
	afx_msg void OnDestroy();
	afx_msg void OnNotifierLeftClicked();
	afx_msg void OnNotifierRightClicked();
	afx_msg void OnNotifierWindowHided();
	afx_msg void OnMsgIcon(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnInsert();
	afx_msg void OnChangeCurParam();
	afx_msg void OnDelete();
	afx_msg void OnSave();
	afx_msg void OnStop();
	afx_msg void OnStart();
	afx_msg void OnChekNow();
	afx_msg void OnCheckCurrent();
	afx_msg void OnExit();
	afx_msg void OnSavePage();
	afx_msg void OnHideWindow();
	afx_msg void OnResetChangeCount();
	afx_msg void OnOpenUrl();
	afx_msg void OnMailingOn();
	afx_msg void OnSendMail();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MFCWEBAGENTDLG_H__00AABA48_186C_4344_AB8A_AE21A528EFEB__INCLUDED_)
