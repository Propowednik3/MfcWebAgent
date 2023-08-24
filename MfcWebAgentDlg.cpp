// MfcWebAgentDlg.cpp : implementation file

#include "stdafx.h"
#include "MfcWebAgent.h"
#include "MfcWebAgentDlg.h"
#include "TaskbarNotifier.h"
#include "Shlwapi.h"
#include <gdiplus.h>
#include "AzWindows.h"
#include <stdlib.h>
#include <stdio.h>
#include <curl.h>
#include <atlbase.h>
#include <afxole.h>
#include <afxmt.h>
#include "iconv.h"

#pragma comment (lib, "gdiplus.lib")
#pragma comment (lib, "shlwapi.lib")
//#pragma  comment (lib, "libcurl_imp.lib")
#pragma  comment (lib, "libcurl.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define		MYMSG_NOTIFYICON		(WM_APP + 100)
#define		NA_TIMER_SKIP			10
#define		MAIL_SIZE_MAX			5000000


struct upload_status 
{
	int attach_index;
	int attach_pos;
	int attach2_index;
	int attach2_pos;
	int lines_count;	
	int lines_read;	
	char *cMailBody[64];
	int BodyLen[64];	
	int BodyPos[64];	
};

typedef struct 
{
	char				Word[128];
} FilterDataType;

typedef struct 
{
	char				*Body;
	unsigned int		BodySize;
	char				*SearchKey;
	char				*ShowKey;
	char				*FilterKey;
	char				*MailKey;
	char				*ClickCmd;
	char				*ImageUrl;
	char				*Text;
	char				Status;
	unsigned int		DetectCount;
	int					Service;
} GroupDataType;

typedef struct 
{
	char				Status;
	char				*URL;
	char				*Login;
	char				*Password;
	char				*Name;
	unsigned int		ScanInterval;
	char				CodePage;
	char				*EndPageKeys;
	FilterDataType		*EndPageKeysList;
	unsigned int		EndPageKeysListLen;	
	char				*BeginTag;
	char				*EndTag;
	char				*SearchKeyTags;
	char				*ShowKeyTags;
	char				*ClickCmdTags;
	char				*FilterKeyTags;
	char				*FilterWords;
	FilterDataType		*FilterList;
	unsigned int		FilterListLen;
	char				*MailKeyTags;
	unsigned int		ClickMode;
	char				*ImageUrlTags;
	unsigned int		HistorySize;
	GroupDataType		*HistoryData;
	unsigned int		HistoryCount;
	char				*LastDateScan;
	char				*LastChangeDate;
	unsigned int		LastCountScan;
	unsigned int		Counter;
	unsigned int		CounterNA;
	GroupDataType		*GroupData;
	unsigned int		GroupLen;
	char				ChangedHistory;
	unsigned int		DontChangeCnt;
	char				ShowDontChanged;
	unsigned int		ShowDontChangeLimit;
	char				UseFilter;	
} Search_Param;


LONG OnMsgIcon(WPARAM wParam, LPARAM lParam);
VOID AnimateIcon(HINSTANCE hInstance, HWND hWnd, DWORD dwMsgType,UINT nIndexOfIcon, char * cCaption);
VOID TimerAnimationIcon(HINSTANCE hInst, HWND hWnd);
LONG OnTrayNotification(HINSTANCE hInstance, HWND hWnd, WPARAM wParam, LPARAM lParam);
int LoadSettings(char *Buff);
int Str2Int(char *cString);
void UpperTextLimit(char *cText, int iLen);
int GetParamSetting(unsigned int uiNum, char cParamKey, char *cBuffIn, unsigned int uiBuffInSize, char *cBuffOut, unsigned int uiBuffOutSize);
int TestTarget(int iCurNum);
int ConvertData(unsigned int uiParamNum, GroupDataType **GroupData, unsigned int *uiGroupLen, char* cHtmlData, int uiHtmlLen);
int GenerateKeys(Search_Param *sParam);
int ExtractHtmlText(char *cHtmlCode, int iHtmlSize, char **cOutBuffer);
int RenderKeyData(GroupDataType *GroupData, char *cMaskStr, char **cOutBuffer);
int GetHtmlParam(char *cHtmlData, unsigned int uiHtmlSize, char *cParam, unsigned int uiParamSize, char **cOutValue, char cDebug);
int ClearHtmlSpecs(char *cBuff, int *iOldLen);
int ClearSpecsSymb(unsigned char *cBuff, int *iOldLen);
int LoadHistory(Search_Param *spSett);
int SaveHistory(Search_Param *spSett);
void UpdateShowStatus();
void ShowFirstMessage();
void ShowErrorMessage(char* cMessLogo, char* cMessCaption, char* cMessBody);
void CopyToBuffer(char *cText);
int ReleaseMinHistory(GroupDataType *gdtHistory, unsigned int uiHistoryCount);
void ConvertFilterToList(char *cWords, FilterDataType **ftListOut, unsigned int *uiLestLen);
void SendNews();
int SendHtmlMail(char *mToAddress, char *mFromAddr, char *mMainText, char *mBodyText, unsigned int uiBodyTextLen, char *mServer, char *mLogin, char *mPassword, char *mAuth);


enum ICON_TYPE 
{
	ICON_TYPE_WAIT, 
	ICON_TYPE_LOCK,
	ICON_TYPE_STOP,	
	ICON_TYPE_ANIMATE1,
	ICON_TYPE_ANIMATE2,
	ICON_TYPE_UPDATE,
	ICON_TYPE_SEEM,
	ICON_TYPE_WIFI,
	ICON_TYPE_MAX
};

enum CODE_PAGE 
{
	CODE_PAGE_UNKNOWN, 
	CODE_PAGE_1251,
	CODE_PAGE_UTF8	
};

#define		NUM_ICON_FOR_ANIMATION	ICON_TYPE_MAX


using namespace Gdiplus;

CTaskbarNotifier m_wndTaskbarNotifier1;
unsigned int uiTimeCount;
static int		IconBmpArray[NUM_ICON_FOR_ANIMATION] = {IDB_BMP_STILL, IDB_BMP_LOCK, IDB_BMP_STOP, IDB_BMP_STILL, IDB_BMP_LOVE, IDB_BMP_UPDATE, IDB_BMP_SEEM, IDB_BMP_WIFI};
static int		IconBmpIdle;
static char		IconCaption[64];
unsigned int	uiSearchSettCount;
Search_Param	*spSearchSettings;
ULONG_PTR    gdiplusToken;
HGLOBAL m_hBuffer;	
CCriticalSection BlockSection;
//CURL *curl;	
	
char cMessageReaded;
char cAnimationIconStatus;
char cAnimationFirstIcon;
char cAnimationIconCnt;
char cStatus;
unsigned int uiPauseAfterScan;
char cScanStatus;
unsigned int uiShowTime;
unsigned int uiPauseTimer;
unsigned int uiTestIcon;
char AppConfigPath[MAX_PATH];
char AppHistoryPath[MAX_PATH];
char AppCookiePath[MAX_PATH];
unsigned int uiShowStatus;
char cBrowserString[1024];

unsigned int	uiUseMailingTimer;
unsigned int	uiMailingDays;
unsigned int	uiMailingInterval;
unsigned int	uiMailingIntervalCounter;
unsigned int	uiMailingTimeOn;
unsigned int	uiMailingTimeOff;
unsigned int	uiMailingMessCnt;
char			cMailingEmail[64];
char			cMailingServer[64];
char			cMailingLogin[64];
char			cMailingPassword[64];
char			cMailingAuth[64];
char			cMailingDestMail[64];


unsigned int uiAnimIconFirst;
unsigned int uiAnimIconLast;
unsigned int uiCurAnimIcon;

Gdiplus::Image *m_picture;
unsigned int uiShowSett;
unsigned int uiShowPos;
char cStart;
char cIconStatus;

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMfcWebAgentDlg dialog

CMfcWebAgentDlg::CMfcWebAgentDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMfcWebAgentDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMfcWebAgentDlg)
	m_URL = _T("");
	m_Login = _T("");
	m_Password = _T("");
	m_Name = _T("");
	m_ScanInterval = _T("");
	m_BeginTag = _T("");
	m_EndTag = _T("");
	m_ShowKeyTags = _T("");
	m_HistorySize = _T("");
	m_SearchKeyTags = _T("");
	m_LastDateScan = _T("");
	m_ClickCmdTags = _T("");
	m_ImageUrlTags = _T("");
	m_PauseAfterScan = _T("");
	m_ScanStatus = _T("");
	m_HistoryCnt = _T("");
	m_BrowserName = _T("");
	m_LastLoadedCnt = _T("");
	m_DontChangeCnt = _T("");
	m_LastChangeDate = _T("");
	m_ShowDontChanged = FALSE;
	m_ShowLimitDetect = _T("");
	m_FilterKeyTags = _T("");
	m_UseFilter = FALSE;
	m_FilterWords = _T("");
	m_UseMailingTimer = FALSE;
	m_MailingOnTime = _T("");
	m_MailingOffTime = _T("");
	m_MailingDay1 = FALSE;
	m_MailingDay2 = FALSE;
	m_MailingDay3 = FALSE;
	m_MailingDay4 = FALSE;
	m_MailingDay5 = FALSE;
	m_MailingDay6 = FALSE;
	m_MailingDay7 = FALSE;
	m_MailingEmail = _T("");
	m_MailingServer = _T("");
	m_MailingLogin = _T("");
	m_MailingPassword = _T("");
	m_MailKeyTags = _T("");
	m_MailingAuth = _T("");
	m_MailingDestMail = _T("");
	m_MailingInterval = _T("");
	m_MailingMessCnt = _T("");
	m_EndPageKey = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMfcWebAgentDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMfcWebAgentDlg)
	DDX_Control(pDX, IDC_BUTTON13, m_MailerBtn);
	DDX_Control(pDX, IDC_COMBO4, m_ClickModeList);
	DDX_Control(pDX, IDC_COMBO3, m_CodePageList);
	DDX_Control(pDX, IDC_BUTTON5, m_StartBtn);
	DDX_Control(pDX, IDC_BUTTON4, m_StopBtn);
	DDX_Control(pDX, IDC_STR_TIMER, m_TimerStatus);
	DDX_Control(pDX, IDC_COMBO2, m_StatusList);
	DDX_Control(pDX, IDC_COMBO1, m_ParamList);
	DDX_Text(pDX, IDC_EDIT1, m_URL);
	DDX_Text(pDX, IDC_EDIT2, m_Login);
	DDX_Text(pDX, IDC_EDIT3, m_Password);
	DDX_Text(pDX, IDC_EDIT4, m_Name);
	DDX_Text(pDX, IDC_EDIT5, m_ScanInterval);
	DDX_Text(pDX, IDC_EDIT6, m_BeginTag);
	DDX_Text(pDX, IDC_EDIT7, m_EndTag);
	DDX_Text(pDX, IDC_EDIT9, m_ShowKeyTags);
	DDX_Text(pDX, IDC_EDIT10, m_HistorySize);
	DDX_Text(pDX, IDC_EDIT8, m_SearchKeyTags);
	DDX_Text(pDX, IDC_STR_LASTDATE, m_LastDateScan);
	DDX_Text(pDX, IDC_EDIT11, m_ClickCmdTags);
	DDX_Text(pDX, IDC_EDIT12, m_ImageUrlTags);
	DDX_Text(pDX, IDC_EDIT13, m_PauseAfterScan);
	DDX_Text(pDX, IDC_SCANSTATUS, m_ScanStatus);
	DDX_Text(pDX, IDC_HISTORY_CNT, m_HistoryCnt);
	DDX_Text(pDX, IDC_EDIT14, m_BrowserName);
	DDX_Text(pDX, IDC_LAST_LOADED, m_LastLoadedCnt);
	DDX_Text(pDX, IDC_DONTCHANGE_CNT, m_DontChangeCnt);
	DDX_Text(pDX, IDC_LASTCHANGE_DATE, m_LastChangeDate);
	DDX_Check(pDX, IDC_CHECK1, m_ShowDontChanged);
	DDX_Text(pDX, IDC_EDIT15, m_ShowLimitDetect);
	DDX_Text(pDX, IDC_EDIT16, m_FilterKeyTags);
	DDX_Check(pDX, IDC_CHECK2, m_UseFilter);
	DDX_Text(pDX, IDC_EDIT17, m_FilterWords);
	DDX_Check(pDX, IDC_CHECK10, m_UseMailingTimer);
	DDX_Text(pDX, IDC_EDIT22, m_MailingOnTime);
	DDX_Text(pDX, IDC_EDIT23, m_MailingOffTime);
	DDX_Check(pDX, IDC_CHECK3, m_MailingDay1);
	DDX_Check(pDX, IDC_CHECK4, m_MailingDay2);
	DDX_Check(pDX, IDC_CHECK5, m_MailingDay3);
	DDX_Check(pDX, IDC_CHECK6, m_MailingDay4);
	DDX_Check(pDX, IDC_CHECK7, m_MailingDay5);
	DDX_Check(pDX, IDC_CHECK8, m_MailingDay6);
	DDX_Check(pDX, IDC_CHECK9, m_MailingDay7);
	DDX_Text(pDX, IDC_EDIT18, m_MailingEmail);
	DDX_Text(pDX, IDC_EDIT19, m_MailingServer);
	DDX_Text(pDX, IDC_EDIT20, m_MailingLogin);
	DDX_Text(pDX, IDC_EDIT21, m_MailingPassword);
	DDX_Text(pDX, IDC_EDIT25, m_MailKeyTags);
	DDX_Text(pDX, IDC_EDIT24, m_MailingAuth);
	DDX_Text(pDX, IDC_EDIT26, m_MailingDestMail);
	DDX_Text(pDX, IDC_EDIT27, m_MailingInterval);
	DDX_Text(pDX, IDC_EDIT28, m_MailingMessCnt);
	DDX_Text(pDX, IDC_EDIT29, m_EndPageKey);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMfcWebAgentDlg, CDialog)
	//{{AFX_MSG_MAP(CMfcWebAgentDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_NOTIFIER_LEFT_CLICKED,OnNotifierLeftClicked)
	ON_MESSAGE(WM_NOTIFIER_RIGHT_CLICKED,OnNotifierRightClicked)
	ON_MESSAGE(WM_NOTIFIER_WINDOW_HIDED,OnNotifierWindowHided)
	ON_MESSAGE(MYMSG_NOTIFYICON, OnMsgIcon)	
	ON_WM_TIMER()	
	ON_BN_CLICKED(IDC_BUTTON1, OnInsert)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnChangeCurParam)
	ON_BN_CLICKED(IDC_BUTTON2, OnDelete)
	ON_BN_CLICKED(IDC_BUTTON3, OnSave)
	ON_BN_CLICKED(IDC_BUTTON4, OnStop)
	ON_BN_CLICKED(IDC_BUTTON5, OnStart)
	ON_BN_CLICKED(IDC_BUTTON6, OnChekNow)
	ON_BN_CLICKED(IDC_BUTTON7, OnCheckCurrent)
	ON_BN_CLICKED(IDC_BUTTON8, OnExit)
	ON_BN_CLICKED(IDC_BUTTON9, OnSavePage)
	ON_BN_CLICKED(IDC_BUTTON10, OnHideWindow)
	ON_BN_CLICKED(IDC_BUTTON11, OnResetChangeCount)
	ON_BN_CLICKED(IDC_BUTTON12, OnOpenUrl)
	ON_BN_CLICKED(IDC_BUTTON13, OnMailingOn)
	ON_BN_CLICKED(IDC_BUTTON14, OnSendMail)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMfcWebAgentDlg message handlers

void CMfcWebAgentDlg::OnNotifierLeftClicked()
{
	lPrint("MfcWebAgent: OnNotifierLeftClicked");
	BlockSection.Lock();
	if ((uiShowSett < uiSearchSettCount) && 
		(uiShowPos < spSearchSettings[uiShowSett].HistoryCount) && 
		(spSearchSettings[uiShowSett].HistoryData[uiShowPos].Status == 0)) 
	{
		spSearchSettings[uiShowSett].HistoryData[uiShowPos].Status = 1;		
		SaveHistory(&spSearchSettings[uiShowSett]);
		if (strlen(spSearchSettings[uiShowSett].ClickCmdTags) &&
			strlen(spSearchSettings[uiShowSett].HistoryData[uiShowPos].ClickCmd))
		{				
			if (spSearchSettings[uiShowSett].ClickMode == 1)
				CopyToBuffer(spSearchSettings[uiShowSett].HistoryData[uiShowPos].ClickCmd);
			if (spSearchSettings[uiShowSett].ClickMode == 2) 
				ShellExecute(0, "Open", spSearchSettings[uiShowSett].HistoryData[uiShowPos].ClickCmd, NULL, NULL, SW_SHOW);
			cMessageReaded = 0;
		}
	}
	BlockSection.Unlock();
	m_wndTaskbarNotifier1.ExtraHide(5);		
}

void CMfcWebAgentDlg::OnNotifierRightClicked()
{
	lPrint("MfcWebAgent: OnNotifierRightClicked");
	BlockSection.Lock();
	if ((uiShowSett < uiSearchSettCount) && 
		(uiShowPos < spSearchSettings[uiShowSett].HistoryCount) && 
		(spSearchSettings[uiShowSett].HistoryData[uiShowPos].Status == 0)) 
	{
		spSearchSettings[uiShowSett].HistoryData[uiShowPos].Status = 1;		
		SaveHistory(&spSearchSettings[uiShowSett]);
		cMessageReaded = 1;
	}
	BlockSection.Unlock();
	m_wndTaskbarNotifier1.ExtraHide(5);		
}

void CMfcWebAgentDlg::OnNotifierWindowHided()
{
	lPrint("MfcWebAgent: OnNotifierWindowHided");
	BlockSection.Lock();
	UpdateShowStatus();
	
	if (uiShowStatus && cMessageReaded) ShowFirstMessage();
	BlockSection.Unlock();
}

void CMfcWebAgentDlg::OnMsgIcon(WPARAM wParam, LPARAM lParam)
{
	BlockSection.Lock();
	switch(lParam)
	{
	case WM_RBUTTONDOWN:
		{
			//if (cMessageReaded || (cShowStatus == 0)) ShowWindow(SW_SHOW);
			//else
		/*	{
				cMessageReaded = 0;
				m_wndTaskbarNotifier1.ExtraHide(5);
			}*/	
			//if (uiShowStatus) ShowFirstMessage();
			/*cScanStatus = 0;
			m_ScanStatus = "Stop";
			m_StopBtn.EnableWindow(0);
			m_StartBtn.EnableWindow(1);
			UpdateData(false);
			AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_MODIFY, ICON_TYPE_STOP, "Disable");*/			
			break ;
		}
	case WM_LBUTTONDOWN:
		{	
			if (uiShowStatus) ShowFirstMessage();
				
			/*if (cAnimationIconStatus == 1)
			{
				cAnimationIconStatus = 0;
				if (cScanStatus == 0) AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_MODIFY, ICON_TYPE_STOP, "Disable");
				else AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_MODIFY, ICON_TYPE_WAIT, "Wait");
				BlockSection.Unlock();	
				OnTimer(3);
				BlockSection.Lock();
			}
			else
			{
				BlockSection.Unlock();	
				OnTimer(1);
				BlockSection.Lock();
			}	*/
			break ;
		}
	case WM_LBUTTONDBLCLK:
		{
		//	ShowWindow(SW_SHOW);
		//	SetForegroundWindow();
			break ;
		}
	case WM_RBUTTONDBLCLK:
		{
			ShowWindow(SW_SHOW);
			SetForegroundWindow();
			//BlockSection.Unlock();			
			//ExitProcess(0);
			break ;
		}
	}
	BlockSection.Unlock();			
}

BOOL CMfcWebAgentDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	char AppPath[MAX_PATH];
	char AppFile[MAX_PATH];	

	
	GetModuleFileName(NULL, AppPath, MAX_PATH);
	PathRemoveFileSpec(AppPath);
	SetCurrentDirectory(AppPath);
	CreateDirectory("Cookies", NULL);
	CreateDirectory("Historias", NULL);
	
	IconBmpIdle = ICON_TYPE_WAIT;
	memset(IconCaption, 0, 64);
	strcpy(IconCaption, "Wait");
	cIconStatus = 0;
	uiShowStatus = 0;
	cScanStatus = 0;
	uiShowTime = 5000;
	uiSearchSettCount = 0;
	spSearchSettings = NULL;
	uiPauseAfterScan = 10;
	uiShowSett = 0;
	uiShowPos = 0;
	cStart = 0;
	uiTestIcon = 0;
	cMessageReaded = 0;
	memset(cBrowserString, 0, 1024);
	strcpy(cBrowserString, "Mozilla/5.0 (Windows; U; Windows NT 6.1; uk; rv:1.9.2.13) Gecko/20101203 Firefox/3.6.13 Some plugins");

	uiUseMailingTimer = 0;
	uiMailingDays = 0;
	uiMailingIntervalCounter = 0;
	uiMailingInterval = 300;	
	uiMailingTimeOn = 1800;
	uiMailingTimeOff = 900;
	uiMailingMessCnt = 20;
	memset(cMailingEmail, 0, 64);
	memset(cMailingServer, 0, 64);
	memset(cMailingLogin, 0, 64);
	memset(cMailingPassword, 0, 64);
	memset(cMailingAuth, 0, 64);
	memset(cMailingDestMail, 0, 64);
	strcpy(cMailingServer, "smtp://");

	uiAnimIconFirst = ICON_TYPE_ANIMATE1;
	uiAnimIconLast = ICON_TYPE_ANIMATE2;
	uiCurAnimIcon = uiAnimIconFirst;
		
	memset(AppConfigPath, 0, MAX_PATH);
	strcpy(AppConfigPath, AppPath);
	strcat(AppConfigPath, "\\");
	strcat(AppConfigPath, "config.ini");
	LoadSettings(AppConfigPath);

	memset(AppHistoryPath, 0, MAX_PATH);
	strcpy(AppHistoryPath, AppPath);
	strcat(AppHistoryPath, "\\Historias\\");

	memset(AppCookiePath, 0, MAX_PATH);
	strcpy(AppCookiePath, AppPath);
	strcat(AppCookiePath, "\\Cookies\\");	

	unsigned int i;
	for (i = 0; i < uiSearchSettCount; i++)
	{
		LoadHistory(&spSearchSettings[i]);
		lPrint("MfcWebAgent: History(%i):%i", i, spSearchSettings[i].HistoryCount);
	}
	
	memset(AppFile, 0, MAX_PATH);
	strcpy(AppFile, AppPath);
	strcat(AppFile, "\\");
	strcat(AppFile, "skin.bmp");
	
	lPrint("MfcWebAgent: ScanStatus:%i", cScanStatus);
	lPrint("MfcWebAgent: ShowTime:%i", uiShowTime);
	lPrint("MfcWebAgent: Targets:%i", uiSearchSettCount);
	lPrint("MfcWebAgent: PauseAfterScan:%i", uiPauseAfterScan);
	lPrint("MfcWebAgent: UserAgent:%s", cBrowserString);

	using namespace Gdiplus;

	m_picture = NULL;
	
	GdiplusStartupInput gdiplusStartupInput;
	Status st = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	if (st != 0) 
	{
		MessageBox("Error init GDI+", "Error", MB_ICONERROR);
		return 0;
	}
	
	m_wndTaskbarNotifier1.Create(this);
	//m_wndTaskbarNotifier1.SetSkin(bufffile);
	m_wndTaskbarNotifier1.SetSkin(IDB_SKIN);	
	m_wndTaskbarNotifier1.SetTextFont("Arial",70,TN_TEXT_NORMAL,TN_TEXT_UNDERLINE);
	m_wndTaskbarNotifier1.SetTextColor(RGB(0,0,0),RGB(0,200,200));
	m_wndTaskbarNotifier1.SetTextRect(CRect(10,40,m_wndTaskbarNotifier1.m_nSkinWidth-10,m_wndTaskbarNotifier1.m_nSkinHeight-25));	
	
//	m_StatusList.SetItemHeight(-1,50);
	InitSettings();
	UpdateWindowSurface();
	if (cScanStatus == 0) 
	{
		m_StopBtn.EnableWindow(0);
		m_StartBtn.EnableWindow(1);
		m_MailerBtn.EnableWindow(1);
		m_ScanStatus = "Run";
	}
	if (cScanStatus == 1) 
	{
		m_StopBtn.EnableWindow(1);
		m_StartBtn.EnableWindow(0);
		m_MailerBtn.EnableWindow(1);
		m_ScanStatus = "Stop";
	}
	if (cScanStatus == 2) 
	{
		m_StopBtn.EnableWindow(1);
		m_StartBtn.EnableWindow(1);
		m_MailerBtn.EnableWindow(0);
		m_ScanStatus = "Cast";
	}
	char cBuff[1024];
	memset(cBuff, 0, 1024);
	itoa(uiPauseAfterScan, cBuff, 10);	
	m_PauseAfterScan = cBuff;

	memset(cBuff, 0, 1024);
	itoa(uiMailingTimeOn, cBuff, 10);	
	m_MailingOnTime = cBuff;
	memset(cBuff, 0, 1024);
	itoa(uiMailingTimeOff, cBuff, 10);	
	m_MailingOffTime = cBuff;
	memset(cBuff, 0, 1024);
	itoa(uiMailingInterval, cBuff, 10);	
	m_MailingInterval = cBuff;
	memset(cBuff, 0, 1024);
	itoa(uiMailingMessCnt, cBuff, 10);	
	m_MailingMessCnt = cBuff;
	
	
		
	
	m_BrowserName = cBrowserString;
	m_MailingDestMail = cMailingDestMail;
	m_MailingEmail = cMailingEmail;
	m_MailingServer = cMailingServer;
	m_MailingLogin = cMailingLogin;
	m_MailingPassword = cMailingPassword;
	m_MailingAuth = cMailingAuth;
	m_UseMailingTimer = uiUseMailingTimer;
	m_MailingDay1 = (uiMailingDays & 1) ? 1 : 0;
	m_MailingDay2 = (uiMailingDays & 2) ? 1 : 0;
	m_MailingDay3 = (uiMailingDays & 4) ? 1 : 0;
	m_MailingDay4 = (uiMailingDays & 8) ? 1 : 0;
	m_MailingDay5 = (uiMailingDays & 16) ? 1 : 0;
	m_MailingDay6 = (uiMailingDays & 32) ? 1 : 0;
	m_MailingDay7 = (uiMailingDays & 64) ? 1 : 0;
		
	UpdateData(false);
	
	//curl = curl_easy_init();

	UpdateShowStatus();	

	if (cScanStatus == 0) AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_ADD, ICON_TYPE_STOP, "Disable");
	else AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_ADD, IconBmpIdle, IconCaption);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMfcWebAgentDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMfcWebAgentDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}	
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMfcWebAgentDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CMfcWebAgentDlg::OnCancel() 
{
	ShowWindow(SW_HIDE);
	
	//CDialog::OnCancel();
}

void AnimateIcon(HINSTANCE hInstance, HWND hWnd, DWORD dwMsgType, UINT nIndexOfIcon, char * cCaption)
{
	if (dwMsgType == NIM_ADD) cIconStatus = 0;
	if ((cIconStatus == 0) && (dwMsgType == NIM_MODIFY)) dwMsgType = NIM_ADD;
	//HICON hIconAtIndex = LoadIcon(hInstance, (LPCTSTR) MAKEINTRESOURCE(IconResourceArray[nIndexOfIcon]));
	if ((nIndexOfIcon == 5) && (cStatus == 0)) 
	{
		nIndexOfIcon = 0;
		cCaption = "Error";
	}
	CBitmap	m_BMP;
	if (m_BMP.LoadBitmap(IconBmpArray[nIndexOfIcon]) != 1) MessageBox(0, "Error LoadBitmap", "Error", MB_ICONERROR);
	ICONINFO icInfo;	
	icInfo.fIcon	= TRUE;
	icInfo.hbmMask	= (HBITMAP) m_BMP;
	icInfo.xHotspot = 0;
	icInfo.yHotspot = 0;
	icInfo.hbmColor	= (HBITMAP) m_BMP;
	HICON hIconAtIndex = CreateIconIndirect(&icInfo);	
	NOTIFYICONDATA IconData;
	
	IconData.cbSize = sizeof(NOTIFYICONDATA);
	IconData.hIcon  = hIconAtIndex;
	IconData.uID = 25;   
	IconData.hWnd   = hWnd;
	lstrcpyn(IconData.szTip, cCaption, (int) strlen(cCaption)+1);
	IconData.uCallbackMessage = MYMSG_NOTIFYICON;
	IconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	
	if (Shell_NotifyIcon(dwMsgType, &IconData) != 1) 
	{
		//MessageBox(0, "Error Shell_NotifyIcon", "Error", MB_ICONERROR);
		lPrint("MfcWebAgent: Error Shell_NotifyIcon %i %i", dwMsgType, GetLastError());
	} 
	else 
	{
		if (dwMsgType == NIM_ADD) cIconStatus = 1;
	}	

	SendMessage(hWnd, WM_SETICON, NULL, (long) hIconAtIndex);
	
	if(hIconAtIndex) DestroyIcon(hIconAtIndex);
	m_BMP.DeleteObject();
}

int SaveSettings(char *Buff)
{
	FILE *f;
	if (!Buff || (strlen(Buff) == 0) || ((f = fopen(Buff,"w")) == NULL))
	{
		lPrint("MfcWebAgent: Error save settings:'%s'\n", Buff);
		return 0;
	}
	
	char Buff1[16384];
	
	memset(Buff1, 0, 1024);
	sprintf(Buff1, "ScanStatus=%i;\n", cScanStatus & 3);
	fputs(Buff1, f);

	memset(Buff1, 0, 1024);
	sprintf(Buff1, "ShowTime=%i;\n", uiShowTime);
	fputs(Buff1, f);

	memset(Buff1, 0, 1024);
	sprintf(Buff1, "PauseAfterScan=%i;\n", uiPauseAfterScan);
	fputs(Buff1, f);

	memset(Buff1, 0, 1024);
	sprintf(Buff1, "UserAgent=%s\n", cBrowserString);
	fputs(Buff1, f);

	memset(Buff1, 0, 1024);
	sprintf(Buff1, "UseMailing=%i\n", uiUseMailingTimer);
	fputs(Buff1, f);

	memset(Buff1, 0, 1024);
	sprintf(Buff1, "MailingDays=%i\n", uiMailingDays);
	fputs(Buff1, f);

	memset(Buff1, 0, 1024);
	sprintf(Buff1, "MailingInterval=%i\n", uiMailingInterval);
	fputs(Buff1, f);

	memset(Buff1, 0, 1024);
	sprintf(Buff1, "MailingTimeOn=%i\n", uiMailingTimeOn);
	fputs(Buff1, f);

	memset(Buff1, 0, 1024);
	sprintf(Buff1, "MailingTimeOff=%i\n", uiMailingTimeOff);
	fputs(Buff1, f);

	memset(Buff1, 0, 1024);
	sprintf(Buff1, "MailingMessCnt=%i\n", uiMailingMessCnt);
	fputs(Buff1, f);

	memset(Buff1, 0, 1024);
	sprintf(Buff1, "MailingDestMail=%s\n", cMailingDestMail);
	fputs(Buff1, f);

	memset(Buff1, 0, 1024);
	sprintf(Buff1, "MailingEmail=%s\n", cMailingEmail);
	fputs(Buff1, f);

	memset(Buff1, 0, 1024);
	sprintf(Buff1, "MailingServer=%s\n", cMailingServer);
	fputs(Buff1, f);

	memset(Buff1, 0, 1024);
	sprintf(Buff1, "MailingLogin=%s\n", cMailingLogin);
	fputs(Buff1, f);

	memset(Buff1, 0, 1024);
	sprintf(Buff1, "MailingPassword=%s\n", cMailingPassword);
	fputs(Buff1, f);

	memset(Buff1, 0, 1024);
	sprintf(Buff1, "MailingAuth=%s\n", cMailingAuth);
	fputs(Buff1, f);
	
	unsigned int n;	
	for (n = 0; n < uiSearchSettCount; n++)
	{
		memset(Buff1, 0, 16384);
		sprintf(Buff1, "Target=%i;%s;%s;%s;%s;%i;%s;%s;%s;%s;%s;%s;%i;%s;%i;%i;%i;%i;%i;%i;%s;%s;%i;%s;%s;%s;\n", 
			spSearchSettings[n].Status,
			spSearchSettings[n].Name,
			spSearchSettings[n].URL,
			spSearchSettings[n].Login,
			spSearchSettings[n].Password,
			spSearchSettings[n].ScanInterval,
			spSearchSettings[n].BeginTag,
			spSearchSettings[n].EndTag,
			spSearchSettings[n].SearchKeyTags,
			spSearchSettings[n].ShowKeyTags,
			spSearchSettings[n].ClickCmdTags,
			spSearchSettings[n].ImageUrlTags,
			spSearchSettings[n].HistorySize,
			spSearchSettings[n].LastDateScan,
			spSearchSettings[n].CodePage,
			spSearchSettings[n].ClickMode,
			spSearchSettings[n].LastCountScan,
			spSearchSettings[n].DontChangeCnt,
			spSearchSettings[n].ShowDontChanged & 1,
			spSearchSettings[n].ShowDontChangeLimit,
			spSearchSettings[n].LastChangeDate,
			spSearchSettings[n].FilterKeyTags,
			spSearchSettings[n].UseFilter & 1,
			spSearchSettings[n].FilterWords,
			spSearchSettings[n].MailKeyTags,
			spSearchSettings[n].EndPageKeys
			);
		fputs(Buff1, f);
	}
	fclose(f);
	lPrint("MfcWebAgent: Saved settings: '%s'", Buff);
	return 1;
}

int SaveHistory(Search_Param *spSett)
{
	char cPath[MAX_PATH];
	memset(cPath, 0, MAX_PATH);
	strcpy(cPath, AppHistoryPath);
	strcat(cPath, spSett->Name);
	strcat(cPath, ".ini");

	FILE *f;
	if (!cPath || (strlen(cPath) == 0) || ((f = fopen(cPath,"w")) == NULL))
	{
		lPrint("MfcWebAgent: Error save history:'%s'\n", cPath);
		return 0;
	}
	
	char Buff1[16384];	
		
	unsigned int n;	
	for (n = 0; n < spSett->HistoryCount; n++)
	{
		memset(Buff1, 0, 1024);
		sprintf(Buff1, "History=%i|%s|%s|%s|%s|%i|%s|%s|\n", 
			spSett->HistoryData[n].Status,
			spSett->HistoryData[n].SearchKey,
			spSett->HistoryData[n].ShowKey,
			spSett->HistoryData[n].ClickCmd,
			spSett->HistoryData[n].ImageUrl,
			spSett->HistoryData[n].DetectCount,
			spSett->HistoryData[n].FilterKey,
			spSett->HistoryData[n].MailKey
			);
		fputs(Buff1, f);
	}
	fclose(f);
	spSett->ChangedHistory = 0;
	lPrint("MfcWebAgent: Saved history: '%s'", cPath);
	return 1;
}

int LoadSettings(char *Buff)
{	
	FILE *f;
	if ((f = fopen(Buff,"r")) == NULL)
	{
		lPrint("MfcWebAgent: Error load settings:%s\n", Buff);
		return 0;
	}
	
	char Buff1[16384];
	char Buff2[16384];
	char Buff3[16384];
	char Buff4[2048];
	unsigned int n, m, len2, len3;
	
	memset(Buff1, 0, 16384);
	while (fgets(Buff1, 16384, f) != NULL)
	{
		if ((Buff1[0] != 35) && ((Buff1[0] < 0) || (Buff1[0] > 32)))
		{
			memset(Buff2, 0, 16384);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((Buff1[n] < 0) || (Buff1[n] > 31)) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 16384);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);	
				
								
				if ((SearchStrInData(Buff2, len2, 0, "SCANSTATUS=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) cScanStatus = Str2Int(Buff4);
				
				if ((SearchStrInData(Buff2, len2, 0, "SHOWTIME=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiShowTime = Str2Int(Buff4);
								
				if ((SearchStrInData(Buff2, len2, 0, "PAUSEAFTERSCAN=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiPauseAfterScan = Str2Int(Buff4);

				if (SearchStrInData(Buff2, len2, 0, "USERAGENT=") == 1)
				{
					memset(cBrowserString, 0, 1024);
					if (strlen(Buff3) >= 1024) memcpy(cBrowserString, Buff3, 1023);
						else memcpy(cBrowserString, Buff3, strlen(Buff3));
				}
				if ((SearchStrInData(Buff2, len2, 0, "USEMAILING=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiUseMailingTimer = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "MAILINGDAYS=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiMailingDays = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "MAILINGINTERVAL=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiMailingInterval = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "MAILINGTIMEON=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiMailingTimeOn = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "MAILINGTIMEOFF=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiMailingTimeOff = Str2Int(Buff4);
				if ((SearchStrInData(Buff2, len2, 0, "MAILINGMESSCNT=") == 1)
					&& (GetParamSetting(0, 59, Buff3, len3, Buff4, 10) == 1)) uiMailingMessCnt = Str2Int(Buff4);
				
				if (SearchStrInData(Buff2, len2, 0, "MAILINGDESTMAIL=") == 1)
				{
					memset(cMailingDestMail, 0, 64);
					if (strlen(Buff3) >= 64) memcpy(cMailingDestMail, Buff3, 63);
					else memcpy(cMailingDestMail, Buff3, strlen(Buff3));
				}
				if (SearchStrInData(Buff2, len2, 0, "MAILINGEMAIL=") == 1)
				{
					memset(cMailingEmail, 0, 64);
					if (strlen(Buff3) >= 64) memcpy(cMailingEmail, Buff3, 63);
					else memcpy(cMailingEmail, Buff3, strlen(Buff3));
				}
				if (SearchStrInData(Buff2, len2, 0, "MAILINGSERVER=") == 1)
				{
					memset(cMailingServer, 0, 64);
					if (strlen(Buff3) >= 64) memcpy(cMailingServer, Buff3, 63);
					else memcpy(cMailingServer, Buff3, strlen(Buff3));
				}
				if (SearchStrInData(Buff2, len2, 0, "MAILINGLOGIN=") == 1)
				{
					memset(cMailingLogin, 0, 64);
					if (strlen(Buff3) >= 64) memcpy(cMailingLogin, Buff3, 63);
					else memcpy(cMailingLogin, Buff3, strlen(Buff3));
				}
				if (SearchStrInData(Buff2, len2, 0, "MAILINGPASSWORD=") == 1)
				{
					memset(cMailingPassword, 0, 64);
					if (strlen(Buff3) >= 64) memcpy(cMailingPassword, Buff3, 63);
					else memcpy(cMailingPassword, Buff3, strlen(Buff3));
				}
				if (SearchStrInData(Buff2, len2, 0, "MAILINGAUTH=") == 1)
				{
					memset(cMailingAuth, 0, 64);
					if (strlen(Buff3) >= 64) memcpy(cMailingAuth, Buff3, 63);
					else memcpy(cMailingAuth, Buff3, strlen(Buff3));
				}

				if (SearchStrInData(Buff2, len2, 0, "TARGET=") == 1)
				{
					uiSearchSettCount++;
					spSearchSettings = (Search_Param*)realloc(spSearchSettings, sizeof(Search_Param) * uiSearchSettCount);
					memset(&spSearchSettings[uiSearchSettCount - 1], 0, sizeof(Search_Param));
					
					if (GetParamSetting(0, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].Status = Str2Int(Buff4);
					}

					if (GetParamSetting(1, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].Name = (char*)malloc(strlen(Buff4) + 1);
						memcpy(spSearchSettings[uiSearchSettCount - 1].Name, Buff4, strlen(Buff4));
						spSearchSettings[uiSearchSettCount - 1].Name[strlen(Buff4)] = 0;
					}					
					else
					{
						spSearchSettings[uiSearchSettCount - 1].Name = (char*)malloc(1);
						spSearchSettings[uiSearchSettCount - 1].Name[0] = 0;
					}

					if (GetParamSetting(2, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].URL = (char*)malloc(strlen(Buff4) + 1);
						memcpy(spSearchSettings[uiSearchSettCount - 1].URL, Buff4, strlen(Buff4));
						spSearchSettings[uiSearchSettCount - 1].URL[strlen(Buff4)] = 0;
					}					
					else
					{
						spSearchSettings[uiSearchSettCount - 1].URL = (char*)malloc(1);
						spSearchSettings[uiSearchSettCount - 1].URL[0] = 0;
					}
					
					if (GetParamSetting(3, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].Login = (char*)malloc(strlen(Buff4) + 1);
						memcpy(spSearchSettings[uiSearchSettCount - 1].Login, Buff4, strlen(Buff4));
						spSearchSettings[uiSearchSettCount - 1].Login[strlen(Buff4)] = 0;
					}					
					else
					{
						spSearchSettings[uiSearchSettCount - 1].Login = (char*)malloc(1);
						spSearchSettings[uiSearchSettCount - 1].Login[0] = 0;
					}

					if (GetParamSetting(4, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].Password = (char*)malloc(strlen(Buff4) + 1);
						memcpy(spSearchSettings[uiSearchSettCount - 1].Password, Buff4, strlen(Buff4));
						spSearchSettings[uiSearchSettCount - 1].Password[strlen(Buff4)] = 0;
					}				
					else
					{
						spSearchSettings[uiSearchSettCount - 1].Password = (char*)malloc(1);
						spSearchSettings[uiSearchSettCount - 1].Password[0] = 0;
					}

					if (GetParamSetting(5, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].ScanInterval = Str2Int(Buff4);
					}

					if (GetParamSetting(6, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].BeginTag = (char*)malloc(strlen(Buff4) + 1);
						memcpy(spSearchSettings[uiSearchSettCount - 1].BeginTag, Buff4, strlen(Buff4));
						spSearchSettings[uiSearchSettCount - 1].BeginTag[strlen(Buff4)] = 0;
					}					
					else
					{
						spSearchSettings[uiSearchSettCount - 1].BeginTag = (char*)malloc(1);
						spSearchSettings[uiSearchSettCount - 1].BeginTag[0] = 0;
					}

					if (GetParamSetting(7, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].EndTag = (char*)malloc(strlen(Buff4) + 1);
						memcpy(spSearchSettings[uiSearchSettCount - 1].EndTag, Buff4, strlen(Buff4));
						spSearchSettings[uiSearchSettCount - 1].EndTag[strlen(Buff4)] = 0;
					}					
					else
					{
						spSearchSettings[uiSearchSettCount - 1].EndTag = (char*)malloc(1);
						spSearchSettings[uiSearchSettCount - 1].EndTag[0] = 0;
					}

					if (GetParamSetting(8, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].SearchKeyTags = (char*)malloc(strlen(Buff4) + 1);
						memcpy(spSearchSettings[uiSearchSettCount - 1].SearchKeyTags, Buff4, strlen(Buff4));
						spSearchSettings[uiSearchSettCount - 1].SearchKeyTags[strlen(Buff4)] = 0;
					}					
					else
					{
						spSearchSettings[uiSearchSettCount - 1].SearchKeyTags = (char*)malloc(1);
						spSearchSettings[uiSearchSettCount - 1].SearchKeyTags[0] = 0;
					}

					if (GetParamSetting(9, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].ShowKeyTags = (char*)malloc(strlen(Buff4) + 1);
						memcpy(spSearchSettings[uiSearchSettCount - 1].ShowKeyTags, Buff4, strlen(Buff4));
						spSearchSettings[uiSearchSettCount - 1].ShowKeyTags[strlen(Buff4)] = 0;
					}					
					else
					{
						spSearchSettings[uiSearchSettCount - 1].ShowKeyTags = (char*)malloc(1);
						spSearchSettings[uiSearchSettCount - 1].ShowKeyTags[0] = 0;
					}

					if (GetParamSetting(10, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].ClickCmdTags = (char*)malloc(strlen(Buff4) + 1);
						memcpy(spSearchSettings[uiSearchSettCount - 1].ClickCmdTags, Buff4, strlen(Buff4));
						spSearchSettings[uiSearchSettCount - 1].ClickCmdTags[strlen(Buff4)] = 0;
					}
					else
					{
						spSearchSettings[uiSearchSettCount - 1].ClickCmdTags = (char*)malloc(1);
						spSearchSettings[uiSearchSettCount - 1].ClickCmdTags[0] = 0;
					}

					if (GetParamSetting(11, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].ImageUrlTags = (char*)malloc(strlen(Buff4) + 1);
						memcpy(spSearchSettings[uiSearchSettCount - 1].ImageUrlTags, Buff4, strlen(Buff4));
						spSearchSettings[uiSearchSettCount - 1].ImageUrlTags[strlen(Buff4)] = 0;
					}
					else
					{
						spSearchSettings[uiSearchSettCount - 1].ImageUrlTags = (char*)malloc(1);
						spSearchSettings[uiSearchSettCount - 1].ImageUrlTags[0] = 0;
					}

					if (GetParamSetting(12, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].HistorySize = Str2Int(Buff4);
					}

					if (GetParamSetting(13, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].LastDateScan = (char*)malloc(128);
						memset(spSearchSettings[uiSearchSettCount - 1].LastDateScan, 0, 128);
						if (strlen(Buff4) < 128) 
							memcpy(spSearchSettings[uiSearchSettCount - 1].LastDateScan, Buff4, strlen(Buff4));
							else
							memcpy(spSearchSettings[uiSearchSettCount - 1].LastDateScan, Buff4, 127);
					}
					else
					{
						spSearchSettings[uiSearchSettCount - 1].LastDateScan = (char*)malloc(128);
						memset(spSearchSettings[uiSearchSettCount - 1].LastDateScan, 0, 128);
						strcpy(spSearchSettings[uiSearchSettCount - 1].LastDateScan, "Not scanned");
					}

					if (GetParamSetting(14, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].CodePage = Str2Int(Buff4);
					}
					if (GetParamSetting(15, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].ClickMode = Str2Int(Buff4);
					}
					if (GetParamSetting(16, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].LastCountScan = Str2Int(Buff4);
					}
					if (GetParamSetting(17, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].DontChangeCnt = Str2Int(Buff4);
					}
					if (GetParamSetting(18, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].ShowDontChanged = Str2Int(Buff4);
					}
					if (GetParamSetting(19, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].ShowDontChangeLimit = Str2Int(Buff4);
					}
					
					if (GetParamSetting(20, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].LastChangeDate = (char*)malloc(128);
						memset(spSearchSettings[uiSearchSettCount - 1].LastChangeDate, 0, 128);
						if (strlen(Buff4) < 128) 
							memcpy(spSearchSettings[uiSearchSettCount - 1].LastChangeDate, Buff4, strlen(Buff4));
						else
							memcpy(spSearchSettings[uiSearchSettCount - 1].LastChangeDate, Buff4, 127);
					}
					else
					{
						spSearchSettings[uiSearchSettCount - 1].LastChangeDate = (char*)malloc(128);
						memset(spSearchSettings[uiSearchSettCount - 1].LastChangeDate, 0, 128);
						strcpy(spSearchSettings[uiSearchSettCount - 1].LastChangeDate, "Not scanned");
					}

					if (GetParamSetting(21, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].FilterKeyTags = (char*)malloc(strlen(Buff4) + 1);
						memcpy(spSearchSettings[uiSearchSettCount - 1].FilterKeyTags, Buff4, strlen(Buff4));
						spSearchSettings[uiSearchSettCount - 1].FilterKeyTags[strlen(Buff4)] = 0;
					}					
					else
					{
						spSearchSettings[uiSearchSettCount - 1].FilterKeyTags = (char*)malloc(1);
						spSearchSettings[uiSearchSettCount - 1].FilterKeyTags[0] = 0;
					}

					if (GetParamSetting(22, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].UseFilter = Str2Int(Buff4);
					}

					if (GetParamSetting(23, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].FilterWords = (char*)malloc(strlen(Buff4) + 1);
						memcpy(spSearchSettings[uiSearchSettCount - 1].FilterWords, Buff4, strlen(Buff4));
						spSearchSettings[uiSearchSettCount - 1].FilterWords[strlen(Buff4)] = 0;
						ConvertFilterToList(spSearchSettings[uiSearchSettCount - 1].FilterWords, 
											&spSearchSettings[uiSearchSettCount - 1].FilterList, 
											&spSearchSettings[uiSearchSettCount - 1].FilterListLen);
					}					
					else
					{
						spSearchSettings[uiSearchSettCount - 1].FilterWords = (char*)malloc(1);
						spSearchSettings[uiSearchSettCount - 1].FilterWords[0] = 0;
					}
					
					if (GetParamSetting(24, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].MailKeyTags = (char*)malloc(strlen(Buff4) + 1);
						memcpy(spSearchSettings[uiSearchSettCount - 1].MailKeyTags, Buff4, strlen(Buff4));
						spSearchSettings[uiSearchSettCount - 1].MailKeyTags[strlen(Buff4)] = 0;
					}					
					else
					{
						spSearchSettings[uiSearchSettCount - 1].MailKeyTags = (char*)malloc(1);
						spSearchSettings[uiSearchSettCount - 1].MailKeyTags[0] = 0;
					}
					if (GetParamSetting(25, 59, Buff3, len3, Buff4, 2048) == 1) 
					{
						spSearchSettings[uiSearchSettCount - 1].EndPageKeys = (char*)malloc(strlen(Buff4) + 1);
						memcpy(spSearchSettings[uiSearchSettCount - 1].EndPageKeys, Buff4, strlen(Buff4));
						spSearchSettings[uiSearchSettCount - 1].EndPageKeys[strlen(Buff4)] = 0;
						ConvertFilterToList(spSearchSettings[uiSearchSettCount - 1].EndPageKeys, 
											&spSearchSettings[uiSearchSettCount - 1].EndPageKeysList, 
											&spSearchSettings[uiSearchSettCount - 1].EndPageKeysListLen);
					}					
					else
					{
						spSearchSettings[uiSearchSettCount - 1].EndPageKeys = (char*)malloc(1);
						spSearchSettings[uiSearchSettCount - 1].EndPageKeys[0] = 0;
					}
				}
			}
		}
	}
	fclose(f);
	lPrint("MfcWebAgent: Loaded settings: '%s'", Buff);
	return 1;
}

int LoadHistory(Search_Param *spSett)
{	
	char cPath[MAX_PATH];
	memset(cPath, 0, MAX_PATH);
	strcpy(cPath, AppHistoryPath);
	strcat(cPath, spSett->Name);
	strcat(cPath, ".ini");

	GroupDataType *gdtHistory = NULL;
	unsigned int uiHistoryCount = 0;
	
	FILE *f;
	if ((f = fopen(cPath,"r")) == NULL)
	{
		lPrint("MfcWebAgent: Error load history:%s\n", cPath);
		return 0;
	}
	
	char Buff1[16384];
	char Buff2[16384];
	char Buff3[16384];
	char Buff4[2048];
	unsigned int n, m, len2, len3, iRet;
	
	memset(Buff1, 0, 16384);
	while (fgets(Buff1, 16384, f) != NULL)
	{
		if ((Buff1[0] != 35) && ((Buff1[0] < 0) || (Buff1[0] > 32)))
		{
			memset(Buff2, 0, 16384);	
			m = 0;
			for (n = 0; n < strlen(Buff1); n++) if ((Buff1[n] < 0) || (Buff1[n] > 31)) {Buff2[m] = Buff1[n]; m++;}					
			//for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] < 32) Buff2[n] = 0;		
			for (n = 0; n < strlen(Buff2); n++) if (Buff2[n] == 61) break;  // "="
			if (strlen(Buff2) != n)
			{
				UpperTextLimit(Buff2, n);
				n++;
				memset(Buff3, 0, 16384);
				memcpy(Buff3,&Buff2[n],strlen(Buff2)-n);
				len2 = strlen(Buff2);				
				len3 = strlen(Buff3);					
								
				if (SearchStrInData(Buff2, len2, 0, "HISTORY=") == 1)
				{					
					if (uiHistoryCount < spSett->HistorySize)
					{
						iRet = uiHistoryCount;
						uiHistoryCount++;					
						gdtHistory = (GroupDataType*)realloc(gdtHistory, sizeof(GroupDataType) * uiHistoryCount);
					}
					else
					{
						iRet = ReleaseMinHistory(gdtHistory, uiHistoryCount);									
					}
					memset(&gdtHistory[iRet], 0, sizeof(GroupDataType));
					
					if (GetParamSetting(0, 124, Buff3, len3, Buff4, 2048) == 1) 
					{
						gdtHistory[iRet].Status = Str2Int(Buff4);
					}

					if (GetParamSetting(1, 124, Buff3, len3, Buff4, 2048) == 1) 
					{
						gdtHistory[iRet].SearchKey = (char*)malloc(strlen(Buff4) + 1);
						memcpy(gdtHistory[iRet].SearchKey, Buff4, strlen(Buff4));
						gdtHistory[iRet].SearchKey[strlen(Buff4)] = 0;
					}					
					else
					{
						gdtHistory[iRet].SearchKey = (char*)malloc(1);
						gdtHistory[iRet].SearchKey[0] = 0;
					}

					if (GetParamSetting(2, 124, Buff3, len3, Buff4, 2048) == 1) 
					{
						gdtHistory[iRet].ShowKey = (char*)malloc(strlen(Buff4) + 1);
						memcpy(gdtHistory[iRet].ShowKey, Buff4, strlen(Buff4));
						gdtHistory[iRet].ShowKey[strlen(Buff4)] = 0;
					}					
					else
					{
						gdtHistory[iRet].ShowKey = (char*)malloc(1);
						gdtHistory[iRet].ShowKey[0] = 0;
					}

					if (GetParamSetting(3, 124, Buff3, len3, Buff4, 2048) == 1) 
					{
						gdtHistory[iRet].ClickCmd = (char*)malloc(strlen(Buff4) + 1);
						memcpy(gdtHistory[iRet].ClickCmd, Buff4, strlen(Buff4));
						gdtHistory[iRet].ClickCmd[strlen(Buff4)] = 0;
					}					
					else
					{
						gdtHistory[iRet].ClickCmd = (char*)malloc(1);
						gdtHistory[iRet].ClickCmd[0] = 0;
					}

					if (GetParamSetting(4, 124, Buff3, len3, Buff4, 2048) == 1) 
					{
						gdtHistory[iRet].ImageUrl = (char*)malloc(strlen(Buff4) + 1);
						memcpy(gdtHistory[iRet].ImageUrl, Buff4, strlen(Buff4));
						gdtHistory[iRet].ImageUrl[strlen(Buff4)] = 0;
					}					
					else
					{
						gdtHistory[iRet].ImageUrl = (char*)malloc(1);
						gdtHistory[iRet].ImageUrl[0] = 0;
					}

					if (GetParamSetting(5, 124, Buff3, len3, Buff4, 2048) == 1) 
					{
						gdtHistory[iRet].DetectCount = Str2Int(Buff4);
					}

					if (GetParamSetting(6, 124, Buff3, len3, Buff4, 2048) == 1) 
					{
						gdtHistory[iRet].FilterKey = (char*)malloc(strlen(Buff4) + 1);
						memcpy(gdtHistory[iRet].FilterKey, Buff4, strlen(Buff4));
						gdtHistory[iRet].FilterKey[strlen(Buff4)] = 0;
					}					
					else
					{
						gdtHistory[iRet].FilterKey = (char*)malloc(1);
						gdtHistory[iRet].FilterKey[0] = 0;
					}

					if (GetParamSetting(7, 124, Buff3, len3, Buff4, 2048) == 1) 
					{
						gdtHistory[iRet].MailKey = (char*)malloc(strlen(Buff4) + 1);
						memcpy(gdtHistory[iRet].MailKey, Buff4, strlen(Buff4));
						gdtHistory[iRet].MailKey[strlen(Buff4)] = 0;
					}					
					else
					{
						gdtHistory[iRet].MailKey = (char*)malloc(1);
						gdtHistory[iRet].MailKey[0] = 0;
					}
				}
			}
		}
	}
	fclose(f);
	
	spSett->HistoryData = gdtHistory;
	spSett->HistoryCount = uiHistoryCount;
	lPrint("MfcWebAgent: Loaded history: '%s'", cPath);
	return 1;
}


int Str2Int(char *cString)
{
	if (cString == NULL) return 0;
	int n,i;
	int ret;
	char cStr[32];
	int iLen = strlen(cString);
	if (iLen == 0) return 0;
	if (iLen > 31) return - 2;
	memset(cStr, 0, 32);
	strcpy(cStr,cString);
	ret = 0;
	i = 1;
	for(n = 0; n < iLen; n++)
	{
		if ((cStr[n] > 47) && (cStr[n] < 58))
		{
			ret *= 10;
			cStr[n] -= 48;			
			ret += cStr[n];	
		}
		if (cStr[n] == 45) i *= -1;
	}
	return ret*i;
}

int GetParamSetting(unsigned int uiNum, char cParamKey, char *cBuffIn, unsigned int uiBuffInSize, char *cBuffOut, unsigned int uiBuffOutSize)
{
	memset(cBuffOut, 0, uiBuffOutSize);
	uiBuffOutSize--;
	unsigned int n, m;
	unsigned int Clk = 0;
	int PrevPos = 0;
	uiBuffInSize--;
	for (n = 0; n <= uiBuffInSize; n++)
	{
		if ((cBuffIn[n] == cParamKey) || (n == uiBuffInSize))
		{
			if (Clk == uiNum)
			{
				if (cBuffIn[n] != cParamKey) n++;
				m = n - PrevPos;
				if (m > uiBuffOutSize)
				{
					memcpy(cBuffOut, &cBuffIn[PrevPos], uiBuffOutSize);
					return 2;
				}
				else 
				{
					memcpy(cBuffOut, &cBuffIn[PrevPos], m);
					return 1;
				}
			}
			PrevPos = n + 1;
			Clk++;
		}
	}
	return 0;	
}

void UpperTextLimit(char *cText, int iLen)
{
	int n;
	int m = iLen;
	for (n = 0; n != m; n++) if ((cText[n] > 96) && (cText[n] < 123)) cText[n] = cText[n] - 32;
}


BOOL CMfcWebAgentDlg::DestroyWindow() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CDialog::DestroyWindow();
}

void CMfcWebAgentDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	BlockSection.Lock();
	for (unsigned int n = 0; n < uiSearchSettCount; n++)
	{
		if (spSearchSettings[n].ChangedHistory)
			SaveHistory(&spSearchSettings[n]);
	}
	AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_DELETE, 0, "");
	BlockSection.Unlock();
	m_wndTaskbarNotifier1.BlockSection.Lock();
	if (m_picture) delete m_picture;
	m_wndTaskbarNotifier1.BlockSection.Unlock();
	GdiplusShutdown(gdiplusToken);
	//curl_easy_cleanup(curl);
	
}

void CMfcWebAgentDlg::OnTimer(UINT nIDEvent) 
{
	
	if (cStart == 0) 
	{
		ShowWindow(SW_HIDE);
		cStart = 1;
	}

	BlockSection.Lock();

	if ((nIDEvent == 0) && uiPauseTimer) uiPauseTimer--;
					
	if ((nIDEvent == 0) && cScanStatus)
	{	
		WINDOWPLACEMENT winstate;		
		GetWindowPlacement(&winstate);
		if (winstate.showCmd)
		{
			int iCurSel = m_ParamList.GetCurSel();
			if (iCurSel >= 0)
			{
				int iCurNum = m_ParamList.GetItemData(iCurSel);
				if ((iCurNum >= 0) && (iCurNum < (int)uiSearchSettCount))
				{
					char Buff1[128];
					memset(Buff1, 0, 128);
					sprintf(Buff1, "Timer:%i  Pause:%i  Wait:%i Mailer:%i News:%i", 
						spSearchSettings[iCurNum].ScanInterval - spSearchSettings[iCurNum].Counter,
						uiPauseTimer, 
						NA_TIMER_SKIP - spSearchSettings[iCurNum].CounterNA,
						uiMailingInterval - uiMailingIntervalCounter,
						uiShowStatus);					
					m_TimerStatus.SetWindowText(Buff1);
				}			
			}
		}

		unsigned int n;
		int next = -1;
		int nextnum = -1;
		for (n = 0; n < uiSearchSettCount; n++) if (spSearchSettings[n].Status != 0) spSearchSettings[n].Counter++;
		if (uiPauseTimer == 0)
			for (n = 0; n < uiSearchSettCount; n++)				
				if (((int)spSearchSettings[n].Counter - (int)spSearchSettings[n].ScanInterval) > next) 
				{
					nextnum = n;
					next = spSearchSettings[n].Counter - spSearchSettings[n].ScanInterval;
				}	
		if (nextnum != -1)
		{
			n = nextnum;
			if ((spSearchSettings[n].Counter >= spSearchSettings[n].ScanInterval) && (uiPauseTimer == 0))
			{
				spSearchSettings[n].Counter = 0;
				if (spSearchSettings[n].Status == 2)
				{
					spSearchSettings[n].CounterNA++;
					if (spSearchSettings[n].CounterNA >= NA_TIMER_SKIP)
					{
						spSearchSettings[n].Status = 1;
						spSearchSettings[n].CounterNA = 0;
					}
				}
				if (spSearchSettings[n].Status == 1)
				{
					AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_MODIFY, ICON_TYPE_UPDATE, "Checking");
					int res = TestTarget(n);
					if (res <= 0) 
					{
						//spSearchSettings[n].Status = 2;
						uiPauseTimer = uiPauseAfterScan * 3;
					}
					else
					{
						uiPauseTimer = uiPauseAfterScan;
						time_t seconds = time(NULL);
						tm* timeinfo = localtime(&seconds);
						memset(spSearchSettings[n].LastDateScan, 0, 128);
						sprintf(spSearchSettings[n].LastDateScan, "%.2i.%.2i.%.4i  %.2i:%.2i:%.2i", 
							timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
						if (m_ParamList.GetCount() && (m_ParamList.GetCurSel() != -1))
						{
							unsigned int uiCurNum = m_ParamList.GetItemData(m_ParamList.GetCurSel());
							if (uiCurNum == n)
							{
								m_LastDateScan = spSearchSettings[n].LastDateScan;
								m_LastChangeDate = spSearchSettings[n].LastChangeDate;
								char cBuff[128];
								memset(cBuff, 0, 128);
								itoa(spSearchSettings[n].LastCountScan, cBuff, 10);
								m_LastLoadedCnt = cBuff;
								memset(cBuff, 0, 128);
								itoa(spSearchSettings[n].DontChangeCnt, cBuff, 10);
								m_DontChangeCnt = cBuff;
								
								UpdateData(false);
							}
						}				
	
						unsigned int iPrev = uiShowStatus;
						UpdateShowStatus();
						AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_MODIFY, IconBmpIdle, IconCaption);					
						if ((iPrev == 0) && uiShowStatus && cMessageReaded) ShowFirstMessage();
						if (res == -1) ShowErrorMessage(spSearchSettings[n].Name, "Error find end tags", spSearchSettings[n].Name);
					}	
				}
			}
		}

		if (winstate.showCmd)
		{
			int iCurSel = m_ParamList.GetCurSel();
			if (iCurSel >= 0)
			{
				int iCurNum = m_ParamList.GetItemData(iCurSel);
				if ((iCurNum >= 0) && (iCurNum < (int)uiSearchSettCount))
				{
					char Buff1[196];
					memset(Buff1, 0, 196);
					sprintf(Buff1, "Timer:%i  Pause:%i  Wait:%i Mailer:%i News:%i", 
						spSearchSettings[iCurNum].ScanInterval - spSearchSettings[iCurNum].Counter,
						uiPauseTimer, 
						NA_TIMER_SKIP - spSearchSettings[iCurNum].CounterNA,
						uiMailingInterval - uiMailingIntervalCounter,
						uiShowStatus);
					m_TimerStatus.SetWindowText(Buff1);
				}			
			}
		}
	}

	if ((nIDEvent == 1) && uiShowStatus && cScanStatus)
	{
		char cIconCapt[64];
		memset(cIconCapt, 0, 64);
		sprintf(cIconCapt, "%i new", uiShowStatus);
		AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_MODIFY, uiCurAnimIcon, cIconCapt);
		uiCurAnimIcon++;
		cAnimationIconStatus = 1;
		if (uiCurAnimIcon > uiAnimIconLast) uiCurAnimIcon = uiAnimIconFirst; 
	}
	if ((nIDEvent == 1) && (uiShowStatus == 0) && cAnimationIconStatus)
	{
		cAnimationIconStatus = 0;
		AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_MODIFY, IconBmpIdle, IconCaption);
	}

	if ((nIDEvent == 0) && (cIconStatus == 0))
	{
		uiTestIcon++;
		if (uiTestIcon == 5) 
		{
			uiTestIcon = 0;
			AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_ADD, IconBmpIdle, IconCaption);
		}
	}

	if (nIDEvent == 0)
	{
		if ((uiMailingTimeOff != uiMailingTimeOn) && uiUseMailingTimer && uiMailingDays)
		{
			time_t seconds = time(NULL);
			tm* timeinfo = localtime(&seconds);
			unsigned int uiNowTime = timeinfo->tm_hour * 100 + timeinfo->tm_min;
			unsigned int uiNowDay = timeinfo->tm_wday;
			if (uiNowDay == 0) uiNowDay = 7;
			uiNowDay--;
			uiNowDay = 1 << uiNowDay;
			
			if (uiNowDay & uiMailingDays)
			{
				if (cScanStatus & 4)
				{
					if (((uiMailingTimeOff < uiMailingTimeOn) && (uiNowTime > uiMailingTimeOff) && (uiNowTime < uiMailingTimeOn))
						|| ((uiMailingTimeOff > uiMailingTimeOn) && ((uiNowTime > uiMailingTimeOff) || (uiNowTime < uiMailingTimeOn))))
					{
						cScanStatus ^= 4;
						m_ScanStatus = "Wait";
						IconBmpIdle = ICON_TYPE_WAIT;
					}
				}
				else
				{
					if (((uiMailingTimeOff < uiMailingTimeOn) && ((uiNowTime > uiMailingTimeOn) || (uiNowTime < uiMailingTimeOff))) 
						|| ((uiMailingTimeOff > uiMailingTimeOn) && (uiNowTime > uiMailingTimeOn) && (uiNowTime < uiMailingTimeOff))) 
					{
						cScanStatus |= 4;
						m_ScanStatus = "Cast";
						IconBmpIdle = ICON_TYPE_WIFI;
					}
				}
				
			}
		}
		if ((cScanStatus == 2) || (cScanStatus & 4)) 
		{
			uiMailingIntervalCounter++;
			if (uiMailingIntervalCounter >= uiMailingInterval)
			{
				uiMailingIntervalCounter = 0;
				SendNews();
			}
		}
		//lPrint("cScanStatus %i", cScanStatus);
	}	

	BlockSection.Unlock();	

	
	return;
/*	if ((cAnimationIconStatus != 2) || (nIDEvent > 2))
	{
		if ((nIDEvent == 2) && (cAnimationIconStatus == 1))
		{
			AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_MODIFY, uiTimeCount, "New");
			uiTimeCount++;
			if ((uiTimeCount >= NUM_ICON_FOR_ANIMATION) || (uiTimeCount >= (unsigned int)(cAnimationFirstIcon + cAnimationIconCnt))) uiTimeCount = cAnimationFirstIcon;
		}
		if (((nIDEvent == 0) && (cAnimationIconStatus == 0) && (cScanStatus == 1)) 
			|| ((nIDEvent == 1) && (cScanStatus == 1)) 
			|| (nIDEvent == 3)
			|| (nIDEvent == 4))
		{
			if (nIDEvent != 3) 
			{
				AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_MODIFY, ICON_TYPE_UPDATE, "Checking");
				//GetMail(&CurrentMailInfo, nIDEvent == 4 ? 1:0);
				//if (cScanStatus == 0) AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_MODIFY, 1, "Disable");
				//	else AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_MODIFY, 5, "Wait");	
			}	
			//if (CurrentMailInfo.status == 1)
			{
				AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_MODIFY, ICON_TYPE_SEEM, "Show");
				//lPrint("MfcWebAgent: Load attach 1 %i %i %i", CurrentMailInfo.BodyAttach, CurrentMailInfo.AttachLen, m_picture);
				
				OnPaint();
				cAnimationIconStatus = 2;
			//	m_wndTaskbarNotifier1.Show(CurrentMailInfo.Date, CurrentMailInfo.From, CurrentMailInfo.Subject, CurrentMailInfo.BodyText, 500, uiShowTime, 1, 3);				
			}// else AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_MODIFY, 3, "Wait");
			if ((nIDEvent == 1) || (nIDEvent == 3)) return;
		}
	}*/
	
	CDialog::OnTimer(nIDEvent);
}

void UpdateShowStatus()
{
	unsigned int i, n;
	char cStat;
	uiShowStatus = 0;
	for (n = 0; n < uiSearchSettCount; n++)
	{
		for (i = 0; i < spSearchSettings[n].HistoryCount; i++)					
			if (spSearchSettings[n].HistoryData[i].Status == 0)
			{
				uiShowStatus++;
			}
	}
	
	cStat = 0;
	for (n = 0; n < uiSearchSettCount; n++)
	{
		if (spSearchSettings[n].Status && spSearchSettings[n].ShowDontChanged && (spSearchSettings[n].DontChangeCnt >= spSearchSettings[n].ShowDontChangeLimit))
		{
			cStat = 1;
			break;
		}		
	}
	if (uiShowStatus == 1)
	{
		if (cStat) 
		{
			IconBmpIdle = ICON_TYPE_SEEM;
			memset(IconCaption, 0, 64);
			strcpy(IconCaption, "Wait (");
			if (strlen(spSearchSettings[n].Name) < 50) 
			{
				memcpy(&IconCaption[6], spSearchSettings[n].Name, strlen(spSearchSettings[n].Name));
				strcat(IconCaption, ")");
			}	
			else
			{
				memcpy(&IconCaption[6], spSearchSettings[n].Name, 50);	
				strcat(IconCaption, "...)");
			}	
			//sprintf(IconCaption, "Wait (%s)", spSearchSettings[n].Name);
		} 
		else 
		{
			IconBmpIdle = ICON_TYPE_WAIT;
			memset(IconCaption, 0, 64);
			strcpy(IconCaption, "Wait");
		}
	}
}

int WideTo1251(WCHAR *wcsString, unsigned int uiStringLen, char **utfData, unsigned int *utfSize)
{	
	unsigned int sizeRequired = WideCharToMultiByte(1251, 0, wcsString, uiStringLen, NULL, 0, NULL, NULL);
	char * utfString = (char*)malloc(sizeRequired + 4);
	memset(utfString, 0, sizeRequired + 4);
	WideCharToMultiByte(1251, 0, wcsString, uiStringLen, utfString, sizeRequired, NULL, NULL);
	
	*utfData = utfString;
	*utfSize = sizeRequired;
	return 1;
}

int W1251ToWide(LPCSTR csString, unsigned int uiStringLen, WCHAR **WideData, unsigned int *WideSize)
{
	unsigned int sizeRequired = MultiByteToWideChar(1251, 0, csString, uiStringLen, NULL, 0);
	unsigned int OutSize = sizeRequired * sizeof(WCHAR);
	WCHAR * WideString = (WCHAR*)malloc(OutSize + sizeof(WCHAR));
	memset(WideString, 0, OutSize + sizeof(WCHAR));
	MultiByteToWideChar(1251, 0, csString, uiStringLen, WideString, sizeRequired);
	*WideData = WideString;
	*WideSize = OutSize;
	return 1;
}

int Utf8ToWide(LPCSTR csString, unsigned int uiStringLen, WCHAR **WideData, unsigned int *WideSize)
{
	unsigned int sizeRequired = MultiByteToWideChar(CP_UTF8, 0, csString, uiStringLen, NULL, 0);
	unsigned int OutSize = sizeRequired * sizeof(WCHAR);
	WCHAR * WideString = (WCHAR*)malloc(OutSize + sizeof(WCHAR));
	memset(WideString, 0, OutSize + sizeof(WCHAR));
	MultiByteToWideChar(CP_UTF8, 0, csString, uiStringLen, WideString, sizeRequired);
	*WideData = WideString;
	*WideSize = OutSize;
	return 1;
}

int WideToAscii(WCHAR *wcsString, char **utfData, unsigned int *utfSize)
{	
	unsigned int sizeRequired = WideCharToMultiByte(CP_ACP, 0, wcsString, -1, NULL, 0, NULL, NULL);
	char * utfString = (char*)malloc(sizeRequired + 4);
	memset(utfString, 0, sizeRequired + 4);
	WideCharToMultiByte(CP_ACP, 0, wcsString, -1, utfString, sizeRequired, NULL, NULL);
	
	*utfData = utfString;
	*utfSize = sizeRequired;
	return 1;
}

int AsciiToWide(LPCSTR csString, unsigned int uiStringLen, WCHAR **WideData, unsigned int *WideSize)
{
	unsigned int sizeRequired = MultiByteToWideChar(CP_ACP, 0, csString, uiStringLen, NULL, 0);
	unsigned int OutSize = sizeRequired * sizeof(WCHAR);
	WCHAR * WideString = (WCHAR*)malloc(OutSize + sizeof(WCHAR));
	memset(WideString, 0, OutSize + sizeof(WCHAR));
	MultiByteToWideChar(CP_ACP, 0, csString, uiStringLen, WideString, sizeRequired);
	*WideData = WideString;
	*WideSize = OutSize;
	return 1;
}

int WideToUtf8(WCHAR *wcsString, char **utfData, unsigned int *utfSize)
{	
	unsigned int sizeRequired = WideCharToMultiByte(CP_UTF8, 0, wcsString, -1, NULL, 0, NULL, NULL);
	char * utfString = (char*)malloc(sizeRequired + 4);
	memset(utfString, 0, sizeRequired + 4);
	WideCharToMultiByte(CP_UTF8, 0, wcsString, -1, utfString, sizeRequired, NULL, NULL);
	
	*utfData = utfString;
	*utfSize = sizeRequired;
	return 1;
}


struct MiscData 
{
	char *DataBody;
	unsigned int DataLen;
	unsigned int BufferSize;
};

int curl_writer(char *data, size_t size, size_t nmemb, void *pData)
{
	int result = size * nmemb;
	MiscData *UserData = (MiscData*)pData;
	//lPrint("MfcWebAgent: %s", data);
	if ((UserData->DataLen + result) > UserData->BufferSize)
	{
		if (UserData->BufferSize < 500000000)
		{
			UserData->BufferSize += 5000000;
			UserData->DataBody = (char*)realloc(UserData->DataBody, UserData->BufferSize);
			//lPrint("MfcWebAgent: Downloaded %i", UserData->DataLen + result);
		} else result = 0;
	}
	if (result)	
	{
		memcpy(&UserData->DataBody[UserData->DataLen], data, result);
		UserData->DataLen += result;
	}
	
	return result;
}

int DownloadAddress(char **pBuffer, unsigned int *uiLen, char *cPath, char *cLogin, char *cPassword, char *cCoockieName, char *cUserAgent)
{
	lPrint("MfcWebAgent: DownloadAddress : '%s'", cPath);
	CURLcode res = CURLE_OK;
	MiscData UserData;
	int ret = 0;

	char cFullPath[1024];
	int iPlen = strlen(cPath);
	memset(cFullPath, 0, 1024);
	if ((SearchStrInDataCaseIgn(cPath, iPlen, 0, "HTTP") <= 0) &&
		(SearchStrInDataCaseIgn(cPath, iPlen, 0, "FTP") <= 0))
			strcat(cFullPath, "http");
	if (SearchStrInDataCaseIgn(cPath, iPlen, 0, ":") <= 0)
			strcat(cFullPath, ":");
	if (SearchStrInDataCaseIgn(cPath, iPlen, 0, "//") <= 0)
		strcat(cFullPath, "//");

	strcat(cFullPath, cPath);

	if (strlen(cFullPath) != (unsigned int)iPlen) lPrint("MfcWebAgent: Update link to '%s'", cFullPath);

	UserData.DataBody = NULL;
	UserData.DataLen = 0;
	UserData.BufferSize = 0;
	
	*uiLen = 0;
	*pBuffer = NULL;
	
	CURL *curl = curl_easy_init();
	if(curl) 
	{
		if (cLogin && strlen(cLogin)) curl_easy_setopt(curl, CURLOPT_USERNAME, cLogin);
		if (cPassword && strlen(cPassword)) curl_easy_setopt(curl, CURLOPT_PASSWORD, cPassword);		
		curl_easy_setopt(curl, CURLOPT_URL, cFullPath);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		//curl_easy_setopt(curl, CURLOPT_REFERER, "http://google.com");
		//curl_easy_setopt(curl, CURLOPT_HEADER, true);
		curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cCoockieName);
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cCoockieName);

		/*curl_easy_setopt(curl, CURLOPT_CAPATH, cCaCert);
		curl_easy_setopt(curl, CURLOPT_SSLCERT, cClntCert);
		curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
		curl_easy_setopt(curl, CURLOPT_SSLKEY, cKeyCert);
		curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, "PEM");
		curl_easy_setopt(curl, CURLOPT_SSLKEYPASSWD, cKeyPass);*/
		//curl_easy_setopt(curl, CURLOPT_RETURNTRANSFER, true);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1); 
		curl_easy_setopt(curl, CURLOPT_COOKIE,"");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_writer);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &UserData);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, cUserAgent);

		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);

		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
		
		res = curl_easy_perform(curl);
		if(res != CURLE_OK)	
		{
			lPrint("MfcWebAgent: DownloadAddress: '%s'", cFullPath);
			lPrint("MfcWebAgent: curl_easy_perform() failed: %s",	curl_easy_strerror(res));
			//MessageBox(0, curl_easy_strerror(res), "curl_easy_perform() failed:", MB_ICONERROR);
		}
		else
		{
			*uiLen = UserData.DataLen;
			*pBuffer = UserData.DataBody;
			ret = 1;
			lPrint("MfcWebAgent: curl_easy_perform() ok: %s",	curl_easy_strerror(res));
		}	
		
		curl_easy_cleanup(curl);
	} 
	else 
	{
		lPrint("MfcWebAgent: curl_easy_init failed: %s",	curl_easy_strerror(res));
		//MessageBox(0, "curl_easy_init error", curl_easy_strerror(res), MB_ICONERROR);
	}	
	lPrint("MfcWebAgent: DownloadAddress Out");
	return ret;
}

void CMfcWebAgentDlg::OnInsert() 
{
	lPrint("MfcWebAgent: OnInsert In");
	BlockSection.Lock();

	uiSearchSettCount++;
	spSearchSettings = (Search_Param*)realloc(spSearchSettings, sizeof(Search_Param) * uiSearchSettCount);
	memset(&spSearchSettings[uiSearchSettCount - 1], 0, sizeof(Search_Param));
	
	UpdateData(true);
	
	int i = m_StatusList.GetCurSel();
	if (i == -1) i = 0;
	spSearchSettings[uiSearchSettCount - 1].Status = i;
	
	spSearchSettings[uiSearchSettCount - 1].Name = (char*)malloc(strlen(m_Name) + 1);
	memcpy(spSearchSettings[uiSearchSettCount - 1].Name, m_Name, strlen(m_Name));
	spSearchSettings[uiSearchSettCount - 1].Name[strlen(m_Name)] = 0;

	spSearchSettings[uiSearchSettCount - 1].URL = (char*)malloc(strlen(m_URL) + 1);
	memcpy(spSearchSettings[uiSearchSettCount - 1].URL, m_URL, strlen(m_URL));
	spSearchSettings[uiSearchSettCount - 1].URL[strlen(m_URL)] = 0;

	spSearchSettings[uiSearchSettCount - 1].Login = (char*)malloc(strlen(m_Login) + 1);
	memcpy(spSearchSettings[uiSearchSettCount - 1].Login, m_Login, strlen(m_Login));
	spSearchSettings[uiSearchSettCount - 1].Login[strlen(m_Login)] = 0;
	
	spSearchSettings[uiSearchSettCount - 1].Password = (char*)malloc(strlen(m_Password) + 1);
	memcpy(spSearchSettings[uiSearchSettCount - 1].Password, m_Password, strlen(m_Password));
	spSearchSettings[uiSearchSettCount - 1].Password[strlen(m_Password)] = 0;
	
	spSearchSettings[uiSearchSettCount - 1].ScanInterval = Str2Int((LPTSTR)(LPCTSTR)m_ScanInterval);
		
	spSearchSettings[uiSearchSettCount - 1].EndPageKeys = (char*)malloc(strlen(m_EndPageKey) + 1);
	memcpy(spSearchSettings[uiSearchSettCount - 1].EndPageKeys, m_EndPageKey, strlen(m_EndPageKey));
	spSearchSettings[uiSearchSettCount - 1].EndPageKeys[strlen(m_EndPageKey)] = 0;
	ConvertFilterToList(spSearchSettings[uiSearchSettCount - 1].EndPageKeys, 
						&spSearchSettings[uiSearchSettCount - 1].EndPageKeysList, 
						&spSearchSettings[uiSearchSettCount - 1].EndPageKeysListLen);

	spSearchSettings[uiSearchSettCount - 1].BeginTag = (char*)malloc(strlen(m_BeginTag) + 1);
	memcpy(spSearchSettings[uiSearchSettCount - 1].BeginTag, m_BeginTag, strlen(m_BeginTag));
	spSearchSettings[uiSearchSettCount - 1].BeginTag[strlen(m_BeginTag)] = 0;
	
	spSearchSettings[uiSearchSettCount - 1].EndTag = (char*)malloc(strlen(m_EndTag) + 1);
	memcpy(spSearchSettings[uiSearchSettCount - 1].EndTag, m_EndTag, strlen(m_EndTag));
	spSearchSettings[uiSearchSettCount - 1].EndTag[strlen(m_EndTag)] = 0;
	
	spSearchSettings[uiSearchSettCount - 1].SearchKeyTags = (char*)malloc(strlen(m_SearchKeyTags) + 1);
	memcpy(spSearchSettings[uiSearchSettCount - 1].SearchKeyTags, m_SearchKeyTags, strlen(m_SearchKeyTags));
	spSearchSettings[uiSearchSettCount - 1].SearchKeyTags[strlen(m_SearchKeyTags)] = 0;
	
	spSearchSettings[uiSearchSettCount - 1].ShowKeyTags = (char*)malloc(strlen(m_ShowKeyTags) + 1);
	memcpy(spSearchSettings[uiSearchSettCount - 1].ShowKeyTags, m_ShowKeyTags, strlen(m_ShowKeyTags));
	spSearchSettings[uiSearchSettCount - 1].ShowKeyTags[strlen(m_ShowKeyTags)] = 0;
	
	spSearchSettings[uiSearchSettCount - 1].ClickCmdTags = (char*)malloc(strlen(m_ClickCmdTags) + 1);
	memcpy(spSearchSettings[uiSearchSettCount - 1].ClickCmdTags, m_ClickCmdTags, strlen(m_ClickCmdTags));
	spSearchSettings[uiSearchSettCount - 1].ClickCmdTags[strlen(m_ClickCmdTags)] = 0;
	
	spSearchSettings[uiSearchSettCount - 1].FilterKeyTags = (char*)malloc(strlen(m_FilterKeyTags) + 1);
	memcpy(spSearchSettings[uiSearchSettCount - 1].FilterKeyTags, m_FilterKeyTags, strlen(m_FilterKeyTags));
	spSearchSettings[uiSearchSettCount - 1].FilterKeyTags[strlen(m_FilterKeyTags)] = 0;

	spSearchSettings[uiSearchSettCount - 1].MailKeyTags = (char*)malloc(strlen(m_MailKeyTags) + 1);
	memcpy(spSearchSettings[uiSearchSettCount - 1].MailKeyTags, m_MailKeyTags, strlen(m_MailKeyTags));
	spSearchSettings[uiSearchSettCount - 1].MailKeyTags[strlen(m_MailKeyTags)] = 0;
	
	spSearchSettings[uiSearchSettCount - 1].FilterWords = (char*)malloc(strlen(m_FilterWords) + 1);
	memcpy(spSearchSettings[uiSearchSettCount - 1].FilterWords, m_FilterWords, strlen(m_FilterWords));
	spSearchSettings[uiSearchSettCount - 1].FilterWords[strlen(m_FilterWords)] = 0;
	ConvertFilterToList(spSearchSettings[uiSearchSettCount - 1].FilterWords, 
						&spSearchSettings[uiSearchSettCount - 1].FilterList, 
						&spSearchSettings[uiSearchSettCount - 1].FilterListLen);

	spSearchSettings[uiSearchSettCount - 1].ImageUrlTags = (char*)malloc(strlen(m_ImageUrlTags) + 1);
	memcpy(spSearchSettings[uiSearchSettCount - 1].ImageUrlTags, m_ImageUrlTags, strlen(m_ImageUrlTags));
	spSearchSettings[uiSearchSettCount - 1].ImageUrlTags[strlen(m_ImageUrlTags)] = 0;
	
	spSearchSettings[uiSearchSettCount - 1].HistorySize = Str2Int((LPTSTR)(LPCTSTR)m_HistorySize);

	spSearchSettings[uiSearchSettCount - 1].ShowDontChanged = m_ShowDontChanged;
	spSearchSettings[uiSearchSettCount - 1].ShowDontChangeLimit = Str2Int((LPTSTR)(LPCTSTR)m_ShowLimitDetect);;

	spSearchSettings[uiSearchSettCount - 1].UseFilter = m_UseFilter;
	
	i = m_CodePageList.GetCurSel();
	if (i == -1) i = 0;
	spSearchSettings[uiSearchSettCount - 1].CodePage = i;
	i = m_ClickModeList.GetCurSel();
	if (i == -1) i = 0;
	spSearchSettings[uiSearchSettCount - 1].ClickMode = i;

	spSearchSettings[uiSearchSettCount - 1].LastDateScan = (char*)malloc(128);
	memset(spSearchSettings[uiSearchSettCount - 1].LastDateScan, 0, 128);
	strcpy(spSearchSettings[uiSearchSettCount - 1].LastDateScan, "Not scanned");
	
	spSearchSettings[uiSearchSettCount - 1].LastChangeDate = (char*)malloc(128);
	memset(spSearchSettings[uiSearchSettCount - 1].LastChangeDate, 0, 128);
	strcpy(spSearchSettings[uiSearchSettCount - 1].LastChangeDate, "Not scanned");
	
	SaveSettings(AppConfigPath);

	int iNewPos = m_ParamList.AddString(spSearchSettings[uiSearchSettCount - 1].Name);
	if (iNewPos >= 0)
	{
		m_ParamList.SetItemData(iNewPos, iNewPos);
		m_ParamList.SetCurSel(iNewPos);
	}

	BlockSection.Unlock();
	lPrint("MfcWebAgent: OnInsert Out");
}


void CMfcWebAgentDlg::InitSettings()
{
	//lPrint("MfcWebAgent: InitSettings In");
	uiTimeCount = 0;	
	m_hBuffer = 0;	
	cMessageReaded = 0;
	cAnimationIconStatus = 0;
	cAnimationFirstIcon = ICON_TYPE_ANIMATE1;
	cAnimationIconCnt = 2;
	cStatus = 1;
		
	SetTimer(0, 1000, NULL);
	SetTimer(1, 500, NULL);
	//lPrint("MfcWebAgent: InitSettings Out");
}

void CMfcWebAgentDlg::OnChangeCurParam() 
{
	//lPrint("MfcWebAgent: OnChangeCurParam In");
	if (!m_ParamList.GetCount()) return;
	BlockSection.Lock();

	unsigned int uiCurNum = m_ParamList.GetItemData(m_ParamList.GetCurSel());
	lPrint("MfcWebAgent: Select targrt:%i %i", m_ParamList.GetCurSel(), uiCurNum);

	m_StatusList.SetCurSel(spSearchSettings[uiCurNum].Status);
	
	m_Name = spSearchSettings[uiCurNum].Name;
	m_URL = spSearchSettings[uiCurNum].URL;
	m_Login = spSearchSettings[uiCurNum].Login;
	m_Password = spSearchSettings[uiCurNum].Password;

	char cBuff[128];
	memset(cBuff, 0, 128);
	itoa(spSearchSettings[uiCurNum].ScanInterval, cBuff, 10);
	m_ScanInterval = cBuff;
	
	m_BeginTag = spSearchSettings[uiCurNum].BeginTag;
	m_EndTag = spSearchSettings[uiCurNum].EndTag;
	m_SearchKeyTags = spSearchSettings[uiCurNum].SearchKeyTags;
	m_ShowKeyTags = spSearchSettings[uiCurNum].ShowKeyTags;
	m_ClickCmdTags = spSearchSettings[uiCurNum].ClickCmdTags;
	m_ImageUrlTags = spSearchSettings[uiCurNum].ImageUrlTags;
	m_FilterKeyTags = spSearchSettings[uiCurNum].FilterKeyTags;
	m_FilterWords = spSearchSettings[uiCurNum].FilterWords;
	m_MailKeyTags = spSearchSettings[uiCurNum].MailKeyTags;
	m_EndPageKey = spSearchSettings[uiCurNum].EndPageKeys;
	
	memset(cBuff, 0, 128);
	itoa(spSearchSettings[uiCurNum].HistorySize, cBuff, 10);
	m_HistorySize = cBuff;

	memset(cBuff, 0, 128);
	itoa(spSearchSettings[uiCurNum].HistoryCount, cBuff, 10);
	m_HistoryCnt = cBuff;


	m_CodePageList.SetCurSel(spSearchSettings[uiCurNum].CodePage);
	m_ClickModeList.SetCurSel(spSearchSettings[uiCurNum].ClickMode);

	m_LastDateScan = spSearchSettings[uiCurNum].LastDateScan;
	m_LastChangeDate = spSearchSettings[uiCurNum].LastChangeDate;

	memset(cBuff, 0, 128);
	itoa(spSearchSettings[uiCurNum].LastCountScan, cBuff, 10);
	m_LastLoadedCnt = cBuff;
	memset(cBuff, 0, 128);
	itoa(spSearchSettings[uiCurNum].DontChangeCnt, cBuff, 10);
	m_DontChangeCnt = cBuff;

	m_ShowDontChanged = spSearchSettings[uiCurNum].ShowDontChanged & 1;

	m_UseFilter = spSearchSettings[uiCurNum].UseFilter & 1;

	memset(cBuff, 0, 128);
	itoa(spSearchSettings[uiCurNum].ShowDontChangeLimit, cBuff, 10);
	m_ShowLimitDetect = cBuff;
	

	UpdateData(false);

	BlockSection.Unlock();
	//lPrint("MfcWebAgent: OnChangeCurParam Out");
}

void CMfcWebAgentDlg::UpdateWindowSurface()
{
	//lPrint("MfcWebAgent: UpdateWindowSurface In");
	BlockSection.Lock();

	m_ParamList.ResetContent();
	for (unsigned int n = 0; n < uiSearchSettCount; n++)
	{
		m_ParamList.InsertString(n, spSearchSettings[n].Name);
		m_ParamList.SetItemData(n, n);		
	}
	
	m_ParamList.SetCurSel(0);
	OnChangeCurParam();

	BlockSection.Unlock();
	//lPrint("MfcWebAgent: UpdateWindowSurface Out");
}

void CMfcWebAgentDlg::OnDelete() 
{
	lPrint("MfcWebAgent: OnDelete In");
	BlockSection.Lock();

	if ((!m_ParamList.GetCount()) || (!spSearchSettings) || (!uiSearchSettCount)) return;
	int iCurSel = m_ParamList.GetCurSel();
	if (iCurSel == -1) iCurSel = 0;
	int iCurNum = m_ParamList.GetItemData(iCurSel);

	Search_Param *SP = (Search_Param*)malloc((uiSearchSettCount - 1) * sizeof(Search_Param));
	int SpCnt = 0;
	for (int n = 0;n < (int)uiSearchSettCount; n++)
		if (n != iCurNum) 
		{
			memcpy(&SP[SpCnt], &spSearchSettings[n], sizeof(Search_Param));
			SpCnt++;
		}
		else
		{
			if (spSearchSettings[n].Name) free(spSearchSettings[n].Name);
			if (spSearchSettings[n].URL) free(spSearchSettings[n].URL);
			if (spSearchSettings[n].Login) free(spSearchSettings[n].Login);
			if (spSearchSettings[n].Password) free(spSearchSettings[n].Password);			
			if (spSearchSettings[n].EndPageKeys) free(spSearchSettings[n].EndPageKeys);
			if (spSearchSettings[n].EndPageKeysList) free(spSearchSettings[n].EndPageKeysList);			
			if (spSearchSettings[n].BeginTag) free(spSearchSettings[n].BeginTag);
			if (spSearchSettings[n].EndTag) free(spSearchSettings[n].EndTag);
			if (spSearchSettings[n].SearchKeyTags) free(spSearchSettings[n].SearchKeyTags);
			if (spSearchSettings[n].ShowKeyTags) free(spSearchSettings[n].ShowKeyTags);
			if (spSearchSettings[n].ClickCmdTags) free(spSearchSettings[n].ClickCmdTags);
			if (spSearchSettings[n].FilterKeyTags) free(spSearchSettings[n].FilterKeyTags);
			if (spSearchSettings[n].FilterWords) free(spSearchSettings[n].FilterWords);
			if (spSearchSettings[n].FilterList) free(spSearchSettings[n].FilterList);
			if (spSearchSettings[n].ImageUrlTags) free(spSearchSettings[n].ImageUrlTags);
			if (spSearchSettings[n].LastDateScan) free(spSearchSettings[n].LastDateScan);
			if (spSearchSettings[n].LastChangeDate) free(spSearchSettings[n].LastChangeDate);
			if (spSearchSettings[n].MailKeyTags) free(spSearchSettings[n].MailKeyTags);			
		}

	free(spSearchSettings);
	spSearchSettings = SP;
	uiSearchSettCount = SpCnt;

	if (m_ParamList.GetCount()) m_ParamList.SetCurSel(0);

	SaveSettings(AppConfigPath);
	
	UpdateWindowSurface();

	BlockSection.Unlock();
	lPrint("MfcWebAgent: OnDelete Out");
}

void CMfcWebAgentDlg::OnSave() 
{
	lPrint("MfcWebAgent: OnSave In");
	BlockSection.Lock();

	UpdateData(true);

	uiPauseAfterScan = Str2Int((LPTSTR)(LPCTSTR)m_PauseAfterScan);
	memset(cBrowserString, 0, 1024);
	strcpy(cBrowserString, (LPTSTR)(LPCTSTR)m_BrowserName);

	uiUseMailingTimer = m_UseMailingTimer;

	uiMailingDays = 0;
	uiMailingDays |= m_MailingDay7;
	uiMailingDays <<= 1;
	uiMailingDays |= m_MailingDay6;
	uiMailingDays <<= 1;
	uiMailingDays |= m_MailingDay5;
	uiMailingDays <<= 1;
	uiMailingDays |= m_MailingDay4;
	uiMailingDays <<= 1;
	uiMailingDays |= m_MailingDay3;
	uiMailingDays <<= 1;
	uiMailingDays |= m_MailingDay2;
	uiMailingDays <<= 1;
	uiMailingDays |= m_MailingDay1;

	uiMailingInterval = Str2Int((LPTSTR)(LPCTSTR)m_MailingInterval);
	uiMailingTimeOn = Str2Int((LPTSTR)(LPCTSTR)m_MailingOnTime);
	uiMailingTimeOff = Str2Int((LPTSTR)(LPCTSTR)m_MailingOffTime);
	uiMailingMessCnt = Str2Int((LPTSTR)(LPCTSTR)m_MailingMessCnt);

	memset(cMailingEmail, 0, 64);
	if (m_MailingEmail.GetLength() < 64) strcpy(cMailingEmail, (LPTSTR)(LPCTSTR)m_MailingEmail);
	memset(cMailingServer, 0, 64);
	if (m_MailingServer.GetLength() < 64) strcpy(cMailingServer, (LPTSTR)(LPCTSTR)m_MailingServer);
	memset(cMailingLogin, 0, 64);
	if (m_MailingLogin.GetLength() < 64) strcpy(cMailingLogin, (LPTSTR)(LPCTSTR)m_MailingLogin);
	memset(cMailingPassword, 0, 64);
	if (m_MailingPassword.GetLength() < 64) strcpy(cMailingPassword, (LPTSTR)(LPCTSTR)m_MailingPassword);
	memset(cMailingAuth, 0, 64);
	if (m_MailingAuth.GetLength() < 64) strcpy(cMailingAuth, (LPTSTR)(LPCTSTR)m_MailingAuth);
	memset(cMailingDestMail, 0, 64);
	if (m_MailingDestMail.GetLength() < 64) strcpy(cMailingDestMail, (LPTSTR)(LPCTSTR)m_MailingDestMail);
	
	int i = m_StatusList.GetCurSel();
	if (i == -1) i = 0;
	int iCurSel = m_ParamList.GetCurSel();
	if (iCurSel == -1) return;
	int iCurNum = m_ParamList.GetItemData(iCurSel);

	if ((iCurNum < 0) || (iCurNum >= (int)uiSearchSettCount))
	{
		BlockSection.Unlock();
		lPrint("MfcWebAgent: Error selected param OnSave", iCurNum, uiSearchSettCount);
		return;
	}

	spSearchSettings[iCurNum].Status = i;

	if (spSearchSettings[iCurNum].Name) free(spSearchSettings[iCurNum].Name);
	spSearchSettings[iCurNum].Name = (char*)malloc(strlen(m_Name) + 1);
	memcpy(spSearchSettings[iCurNum].Name, m_Name, strlen(m_Name));
	spSearchSettings[iCurNum].Name[strlen(m_Name)] = 0;
	
	if (spSearchSettings[iCurNum].URL) free(spSearchSettings[iCurNum].URL);
	spSearchSettings[iCurNum].URL = (char*)malloc(strlen(m_URL) + 1);
	memcpy(spSearchSettings[iCurNum].URL, m_URL, strlen(m_URL));
	spSearchSettings[iCurNum].URL[strlen(m_URL)] = 0;
	
	if (spSearchSettings[iCurNum].Login) free(spSearchSettings[iCurNum].Login);
	spSearchSettings[iCurNum].Login = (char*)malloc(strlen(m_Login) + 1);
	memcpy(spSearchSettings[iCurNum].Login, m_Login, strlen(m_Login));
	spSearchSettings[iCurNum].Login[strlen(m_Login)] = 0;
	
	if (spSearchSettings[iCurNum].Password) free(spSearchSettings[iCurNum].Password);
	spSearchSettings[iCurNum].Password = (char*)malloc(strlen(m_Password) + 1);
	memcpy(spSearchSettings[iCurNum].Password, m_Password, strlen(m_Password));
	spSearchSettings[iCurNum].Password[strlen(m_Password)] = 0;
	
	spSearchSettings[iCurNum].ScanInterval = Str2Int((LPTSTR)(LPCTSTR)m_ScanInterval);
	
	if (spSearchSettings[iCurNum].EndPageKeys) free(spSearchSettings[iCurNum].EndPageKeys);
	spSearchSettings[iCurNum].EndPageKeys = (char*)malloc(strlen(m_EndPageKey) + 1);
	memcpy(spSearchSettings[iCurNum].EndPageKeys, m_EndPageKey, strlen(m_EndPageKey));
	spSearchSettings[iCurNum].EndPageKeys[strlen(m_EndPageKey)] = 0;
	ConvertFilterToList(spSearchSettings[iCurNum].EndPageKeys, 
						&spSearchSettings[iCurNum].EndPageKeysList, 
						&spSearchSettings[iCurNum].EndPageKeysListLen);
	
	if (spSearchSettings[iCurNum].BeginTag) free(spSearchSettings[iCurNum].BeginTag);
	spSearchSettings[iCurNum].BeginTag = (char*)malloc(strlen(m_BeginTag) + 1);
	memcpy(spSearchSettings[iCurNum].BeginTag, m_BeginTag, strlen(m_BeginTag));
	spSearchSettings[iCurNum].BeginTag[strlen(m_BeginTag)] = 0;

	if (spSearchSettings[iCurNum].EndTag) free(spSearchSettings[iCurNum].EndTag);
	spSearchSettings[iCurNum].EndTag = (char*)malloc(strlen(m_EndTag) + 1);
	memcpy(spSearchSettings[iCurNum].EndTag, m_EndTag, strlen(m_EndTag));
	spSearchSettings[iCurNum].EndTag[strlen(m_EndTag)] = 0;
	
	if (spSearchSettings[iCurNum].SearchKeyTags) free(spSearchSettings[iCurNum].SearchKeyTags);
	spSearchSettings[iCurNum].SearchKeyTags = (char*)malloc(strlen(m_SearchKeyTags) + 1);
	memcpy(spSearchSettings[iCurNum].SearchKeyTags, m_SearchKeyTags, strlen(m_SearchKeyTags));
	spSearchSettings[iCurNum].SearchKeyTags[strlen(m_SearchKeyTags)] = 0;

	if (spSearchSettings[iCurNum].ShowKeyTags) free(spSearchSettings[iCurNum].ShowKeyTags);
	spSearchSettings[iCurNum].ShowKeyTags = (char*)malloc(strlen(m_ShowKeyTags) + 1);
	memcpy(spSearchSettings[iCurNum].ShowKeyTags, m_ShowKeyTags, strlen(m_ShowKeyTags));
	spSearchSettings[iCurNum].ShowKeyTags[strlen(m_ShowKeyTags)] = 0;
	
	if (spSearchSettings[iCurNum].ClickCmdTags) free(spSearchSettings[iCurNum].ClickCmdTags);
	spSearchSettings[iCurNum].ClickCmdTags = (char*)malloc(strlen(m_ClickCmdTags) + 1);
	memcpy(spSearchSettings[iCurNum].ClickCmdTags, m_ClickCmdTags, strlen(m_ClickCmdTags));
	spSearchSettings[iCurNum].ClickCmdTags[strlen(m_ClickCmdTags)] = 0;
	
	if (spSearchSettings[iCurNum].FilterKeyTags) free(spSearchSettings[iCurNum].FilterKeyTags);
	spSearchSettings[iCurNum].FilterKeyTags = (char*)malloc(strlen(m_FilterKeyTags) + 1);
	memcpy(spSearchSettings[iCurNum].FilterKeyTags, m_FilterKeyTags, strlen(m_FilterKeyTags));
	spSearchSettings[iCurNum].FilterKeyTags[strlen(m_FilterKeyTags)] = 0;
	
	if (spSearchSettings[iCurNum].MailKeyTags) free(spSearchSettings[iCurNum].MailKeyTags);
	spSearchSettings[iCurNum].MailKeyTags = (char*)malloc(strlen(m_MailKeyTags) + 1);
	memcpy(spSearchSettings[iCurNum].MailKeyTags, m_MailKeyTags, strlen(m_MailKeyTags));
	spSearchSettings[iCurNum].MailKeyTags[strlen(m_MailKeyTags)] = 0;

	if (spSearchSettings[iCurNum].FilterWords) free(spSearchSettings[iCurNum].FilterWords);
	spSearchSettings[iCurNum].FilterWords = (char*)malloc(strlen(m_FilterWords) + 1);
	memcpy(spSearchSettings[iCurNum].FilterWords, m_FilterWords, strlen(m_FilterWords));
	spSearchSettings[iCurNum].FilterWords[strlen(m_FilterWords)] = 0;
	ConvertFilterToList(spSearchSettings[iCurNum].FilterWords, 
						&spSearchSettings[iCurNum].FilterList, 
						&spSearchSettings[iCurNum].FilterListLen);
	
	if (spSearchSettings[iCurNum].ImageUrlTags) free(spSearchSettings[iCurNum].ImageUrlTags);
	spSearchSettings[iCurNum].ImageUrlTags = (char*)malloc(strlen(m_ImageUrlTags) + 1);
	memcpy(spSearchSettings[iCurNum].ImageUrlTags, m_ImageUrlTags, strlen(m_ImageUrlTags));
	spSearchSettings[iCurNum].ImageUrlTags[strlen(m_ImageUrlTags)] = 0;
	
	spSearchSettings[iCurNum].HistorySize = Str2Int((LPTSTR)(LPCTSTR)m_HistorySize);

	spSearchSettings[iCurNum].ShowDontChanged = m_ShowDontChanged;
	spSearchSettings[iCurNum].ShowDontChangeLimit = Str2Int((LPTSTR)(LPCTSTR)m_ShowLimitDetect);;
	
	spSearchSettings[iCurNum].UseFilter = m_UseFilter;
	
	i = m_CodePageList.GetCurSel();
	if (i == -1) i = 0;
	spSearchSettings[iCurNum].CodePage = i;
	
	i = m_ClickModeList.GetCurSel();
	if (i == -1) i = 0;
	spSearchSettings[iCurNum].ClickMode = i;
	
/*	if (spSearchSettings[iCurNum].LastDateScan) free(spSearchSettings[iCurNum].LastDateScan);
	spSearchSettings[iCurNum].LastDateScan = (char*)malloc(128);
	memset(spSearchSettings[iCurNum].LastDateScan, 0, 128);
	strcpy(spSearchSettings[iCurNum].LastDateScan, "Not scanned");
	
	if (spSearchSettings[iCurNum].LastChangeDate) free(spSearchSettings[iCurNum].LastChangeDate);
	spSearchSettings[iCurNum].LastChangeDate = (char*)malloc(128);
	memset(spSearchSettings[iCurNum].LastChangeDate, 0, 128);
	strcpy(spSearchSettings[iCurNum].LastChangeDate, "Not scanned");*/
	
	SaveSettings(AppConfigPath);

	if (spSearchSettings[iCurNum].HistoryCount > spSearchSettings[iCurNum].HistorySize)
	{
		unsigned int uiHistCnt = 0;
		GroupDataType *cHistory = (GroupDataType*)malloc(sizeof(GroupDataType) * spSearchSettings[iCurNum].HistorySize);
		memset(cHistory, 0, sizeof(GroupDataType) * spSearchSettings[iCurNum].HistorySize);
		
		unsigned int iMax = 0;
		unsigned int iFinded;
		unsigned int m;

		for (m = 0; m < spSearchSettings[iCurNum].HistoryCount; m++) 
			spSearchSettings[iCurNum].HistoryData[m].Service = 0;

		for (m = 0; m < spSearchSettings[iCurNum].HistorySize; m++)
		{
			iMax = 0;
			iFinded = spSearchSettings[iCurNum].HistoryCount;
			for (unsigned int k = 0; k < spSearchSettings[iCurNum].HistoryCount; k++)
			{
				if ((iMax < spSearchSettings[iCurNum].HistoryData[k].DetectCount) &&
					(spSearchSettings[iCurNum].HistoryData[k].Service == 0))
				{
					iMax = spSearchSettings[iCurNum].HistoryData[k].DetectCount;
					iFinded = k;
				}
			}	
			if (iFinded != spSearchSettings[iCurNum].HistoryCount)
			{
				spSearchSettings[iCurNum].HistoryData[iFinded].Service = 1;
				memcpy(&cHistory[uiHistCnt], &spSearchSettings[iCurNum].HistoryData[iFinded], sizeof(GroupDataType));
				uiHistCnt++;
			} else break;
		}

		for (m = 0; m < spSearchSettings[iCurNum].HistoryCount; m++)
			if (spSearchSettings[iCurNum].HistoryData[iFinded].Service == 0)
			{
				if (spSearchSettings[iCurNum].HistoryData[m].Body) free(spSearchSettings[iCurNum].HistoryData[m].Body);
				if (spSearchSettings[iCurNum].HistoryData[m].ClickCmd) free(spSearchSettings[iCurNum].HistoryData[m].ClickCmd);
				if (spSearchSettings[iCurNum].HistoryData[m].ImageUrl) free(spSearchSettings[iCurNum].HistoryData[m].ImageUrl);
				if (spSearchSettings[iCurNum].HistoryData[m].ShowKey) free(spSearchSettings[iCurNum].HistoryData[m].ShowKey);
				if (spSearchSettings[iCurNum].HistoryData[m].SearchKey) free(spSearchSettings[iCurNum].HistoryData[m].SearchKey);
				if (spSearchSettings[iCurNum].HistoryData[m].Text) free(spSearchSettings[iCurNum].HistoryData[m].Text);
				if (spSearchSettings[iCurNum].HistoryData[m].FilterKey) free(spSearchSettings[iCurNum].HistoryData[m].FilterKey);
				if (spSearchSettings[iCurNum].HistoryData[m].MailKey) free(spSearchSettings[iCurNum].HistoryData[m].MailKey);
			}
		free(spSearchSettings[iCurNum].HistoryData);
		spSearchSettings[iCurNum].HistoryData = cHistory;
		spSearchSettings[iCurNum].HistoryCount = spSearchSettings[iCurNum].HistorySize;
		
		SaveHistory(&spSearchSettings[iCurNum]);
	}

	if (spSearchSettings[iCurNum].ChangedHistory)
		SaveHistory(&spSearchSettings[iCurNum]);

	m_ParamList.DeleteString(iCurSel);	
	m_ParamList.InsertString(iCurSel, spSearchSettings[iCurNum].Name);
	m_ParamList.SetCurSel(iCurSel);
	m_ParamList.SetItemData(iCurSel, iCurNum);
	//lPrint("MfcWebAgent: >>> %i %i", iCurSel, iCurNum);

	BlockSection.Unlock();
	lPrint("MfcWebAgent: OnSave Out");
}

int TestTarget(int iCurNum)
{
	char *pBuffer = NULL;
	unsigned int uiLen = 0;
	
	if ((iCurNum < 0) || (iCurNum >= (int)uiSearchSettCount))
	{
		lPrint("MfcWebAgent: Error selected param TestTarget", iCurNum, uiSearchSettCount);
		return 0;
	}

	int ret, res;
	char cPath[MAX_PATH];
	memset(cPath, 0, MAX_PATH);
	strcpy(cPath, AppCookiePath);
	strcat(cPath, spSearchSettings[iCurNum].Name);
	strcat(cPath, ".txt");
	//lPrint("MfcWebAgent: COOKIE FILE '%s'", cPath);
	lPrint("MfcWebAgent: Test page start: '%s'", spSearchSettings[iCurNum].Name);
	res = DownloadAddress(&pBuffer, &uiLen, spSearchSettings[iCurNum].URL, spSearchSettings[iCurNum].Login, spSearchSettings[iCurNum].Password, cPath, cBrowserString);
	//res = LoadFileInBuffer("Page.html", &pBuffer, &uiLen);
	if (res)
	{
		//lPrint("MfcWebAgent: '%s'", spSearchSettings[iCurNum].URL);	
		//SaveData(pBuffer, uiLen, "Page.html");
		if (spSearchSettings[iCurNum].CodePage == CODE_PAGE_UTF8)
		{
			WCHAR *WideData;
			unsigned int WideSize;
			Utf8ToWide(pBuffer, uiLen, &WideData, &WideSize);
			free(pBuffer);
			pBuffer = NULL;
			uiLen = 0;
			WideToAscii(WideData, &pBuffer, &uiLen);
			free(WideData);
		}
		if (spSearchSettings[iCurNum].CodePage == CODE_PAGE_1251)
		{
			WCHAR *WideData;
			unsigned int WideSize;
			W1251ToWide(pBuffer, uiLen, &WideData, &WideSize);
			free(pBuffer);
			pBuffer = NULL;
			uiLen = 0;
			WideToAscii(WideData, &pBuffer, &uiLen);
			free(WideData);
		}
		ClearSpecsSymb((unsigned char*)pBuffer, (int*)&uiLen);

		if (ConvertData(iCurNum, &spSearchSettings[iCurNum].GroupData, &spSearchSettings[iCurNum].GroupLen, pBuffer, uiLen) < 0) res = -1;
		if (pBuffer) free(pBuffer);
		if (spSearchSettings[iCurNum].GroupLen) 
		{
			//SaveData(GData[0].Body, GData[0].BodySize, "Frame.html");
			GenerateKeys(&spSearchSettings[iCurNum]);
			int n, i;
			int iChangedHistory = 0;
			//lPrint("MfcWebAgent: Get compare");
			
			for (n = 0; n < (int)spSearchSettings[iCurNum].GroupLen; n++)
			{
				for (i = 0; i < (int)spSearchSettings[iCurNum].HistoryCount; i++)
				{
					//if (strcmp(spSearchSettings[iCurNum].GroupData[n].SearchKey, spSearchSettings[iCurNum].HistoryData[i].SearchKey) == 0) break;	
					if (CompareStrings(spSearchSettings[iCurNum].GroupData[n].SearchKey, spSearchSettings[iCurNum].HistoryData[i].SearchKey)) break;
				}
				if ((spSearchSettings[iCurNum].HistorySize != 0) && (i == (int)spSearchSettings[iCurNum].HistoryCount))
				{					
					if (spSearchSettings[iCurNum].HistoryCount < spSearchSettings[iCurNum].HistorySize)
					{
						//lPrint("MfcWebAgent: Add key %i '%s' '%s'", ret, spSearchSettings[iCurNum].GroupData[n].SearchKey, spSearchSettings[iCurNum].GroupData[n].ShowKey);
						ret = spSearchSettings[iCurNum].HistoryCount;
						spSearchSettings[iCurNum].HistoryCount++;
						spSearchSettings[iCurNum].HistoryData = (GroupDataType*)realloc(spSearchSettings[iCurNum].HistoryData, spSearchSettings[iCurNum].HistoryCount * sizeof(GroupDataType));						
					}
					else
					{	
						ret = ReleaseMinHistory(spSearchSettings[iCurNum].HistoryData, spSearchSettings[iCurNum].HistoryCount);
						//lPrint("MfcWebAgent: Replace key %i '%s' '%s'", ret, spSearchSettings[iCurNum].GroupData[n].SearchKey, spSearchSettings[iCurNum].GroupData[n].ShowKey);
					}
					memset(&spSearchSettings[iCurNum].HistoryData[ret], 0, sizeof(GroupDataType));

					time_t seconds = time(NULL);
					tm* timeinfo = localtime(&seconds);
					spSearchSettings[iCurNum].HistoryData[ret].DetectCount =	(timeinfo->tm_year - 100) * 1000000 +
																				(timeinfo->tm_mon + 1) * 10000 + 
																				timeinfo->tm_mday * 100 + 
																				timeinfo->tm_hour;
					spSearchSettings[iCurNum].HistoryData[ret].Status = 0;
					spSearchSettings[iCurNum].HistoryData[ret].SearchKey = spSearchSettings[iCurNum].GroupData[n].SearchKey;
					spSearchSettings[iCurNum].HistoryData[ret].ShowKey = spSearchSettings[iCurNum].GroupData[n].ShowKey;
					spSearchSettings[iCurNum].HistoryData[ret].ClickCmd = spSearchSettings[iCurNum].GroupData[n].ClickCmd;
					spSearchSettings[iCurNum].HistoryData[ret].ImageUrl = spSearchSettings[iCurNum].GroupData[n].ImageUrl;
					spSearchSettings[iCurNum].HistoryData[ret].FilterKey = spSearchSettings[iCurNum].GroupData[n].FilterKey;
					spSearchSettings[iCurNum].HistoryData[ret].MailKey = spSearchSettings[iCurNum].GroupData[n].MailKey;
					spSearchSettings[iCurNum].GroupData[n].SearchKey = NULL;
					spSearchSettings[iCurNum].GroupData[n].ShowKey = NULL;
					spSearchSettings[iCurNum].GroupData[n].ClickCmd = NULL;
					spSearchSettings[iCurNum].GroupData[n].ImageUrl = NULL;		
					spSearchSettings[iCurNum].GroupData[n].FilterKey = NULL;					
					spSearchSettings[iCurNum].GroupData[n].MailKey = NULL;					
					iChangedHistory = 1;
				} 
				/*else
				{
					//lPrint("MfcWebAgent: Skipped %i", n);
					spSearchSettings[iCurNum].HistoryData[i].DetectCount++;
					spSearchSettings[iCurNum].ChangedHistory = 1;
				}*/
				if (spSearchSettings[iCurNum].GroupData[n].Body)		free(spSearchSettings[iCurNum].GroupData[n].Body);
				if (spSearchSettings[iCurNum].GroupData[n].Text)		free(spSearchSettings[iCurNum].GroupData[n].Text);
				if (spSearchSettings[iCurNum].GroupData[n].SearchKey)	free(spSearchSettings[iCurNum].GroupData[n].SearchKey);
				if (spSearchSettings[iCurNum].GroupData[n].ShowKey)		free(spSearchSettings[iCurNum].GroupData[n].ShowKey);
				if (spSearchSettings[iCurNum].GroupData[n].ClickCmd)	free(spSearchSettings[iCurNum].GroupData[n].ClickCmd);
				if (spSearchSettings[iCurNum].GroupData[n].ImageUrl)	free(spSearchSettings[iCurNum].GroupData[n].ImageUrl);
				if (spSearchSettings[iCurNum].GroupData[n].FilterKey)	free(spSearchSettings[iCurNum].GroupData[n].FilterKey);	
				if (spSearchSettings[iCurNum].GroupData[n].MailKey)		free(spSearchSettings[iCurNum].GroupData[n].MailKey);	
			}
			spSearchSettings[iCurNum].LastCountScan = spSearchSettings[iCurNum].GroupLen;
			
			free(spSearchSettings[iCurNum].GroupData);
			spSearchSettings[iCurNum].GroupLen = 0;
			spSearchSettings[iCurNum].GroupData = NULL;

			if (spSearchSettings[iCurNum].UseFilter)
			{
				for (i = 0; i < (int)spSearchSettings[iCurNum].HistoryCount; i++)
				{
					if (spSearchSettings[iCurNum].HistoryData[i].Status == 0)
					{
						for (n = 0; n < (int)spSearchSettings[iCurNum].FilterListLen; n++)
						{
							if (SearchStrInDataCaseIgn(spSearchSettings[iCurNum].HistoryData[i].FilterKey, strlen(spSearchSettings[iCurNum].HistoryData[i].FilterKey), 0,
													spSearchSettings[iCurNum].FilterList[n].Word) > 0) 
							{
								spSearchSettings[iCurNum].HistoryData[i].Status = 2;
								lPrint("MfcWebAgent: Filtered in '%s' key: '%s' word '%s'", spSearchSettings[iCurNum].Name, 
																spSearchSettings[iCurNum].HistoryData[i].FilterKey, 
																spSearchSettings[iCurNum].FilterList[n].Word);
								iChangedHistory = 1;
								break;
							}
						/*	else
							{
								lPrint("MfcWebAgent: Not Filtered in '%s' key: '%s' word '%s' res %i", spSearchSettings[iCurNum].Name, 
																spSearchSettings[iCurNum].HistoryData[i].FilterKey, 
																spSearchSettings[iCurNum].FilterList[n].Word,
																SearchStrInDataCaseIgn(spSearchSettings[iCurNum].HistoryData[i].FilterKey, 
																					strlen(spSearchSettings[iCurNum].HistoryData[i].FilterKey), 0,
																					spSearchSettings[iCurNum].FilterList[n].Word));
							}*/
						}	
					}
				}
			}
			//lPrint("MfcWebAgent: Done compare");
			if (iChangedHistory)
			{			
				time_t seconds = time(NULL);
				tm* timeinfo = localtime(&seconds);
				memset(spSearchSettings[iCurNum].LastChangeDate, 0, 128);
				sprintf(spSearchSettings[iCurNum].LastChangeDate, "%.2i.%.2i.%.4i  %.2i:%.2i:%.2i", 
					timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
				
				spSearchSettings[iCurNum].DontChangeCnt = 0;
				SaveHistory(&spSearchSettings[iCurNum]);
			}
			else
			{
				spSearchSettings[iCurNum].DontChangeCnt++;
			}
		}
	}
	lPrint("MfcWebAgent: Test page done: '%s'", spSearchSettings[iCurNum].Name);
	return res;
}

void CMfcWebAgentDlg::OnStop() 
{
	BlockSection.Lock();
	cScanStatus = 0;
	BlockSection.Unlock();
	m_StopBtn.EnableWindow(0);
	m_StartBtn.EnableWindow(1);	
	m_MailerBtn.EnableWindow(1);
	m_ScanStatus = "Stop";
	UpdateData(false);
	AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_MODIFY, ICON_TYPE_STOP, "Disable");
}

void CMfcWebAgentDlg::OnStart() 
{
	BlockSection.Lock();
	cScanStatus = 1;
	BlockSection.Unlock();
	m_StopBtn.EnableWindow(1);
	m_StartBtn.EnableWindow(0);	
	m_MailerBtn.EnableWindow(1);
	m_ScanStatus = "Cast";
	IconBmpIdle = ICON_TYPE_WAIT;
	UpdateData(false);
	AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_MODIFY, IconBmpIdle, IconCaption);
}

int ConvertData(unsigned int uiParamNum, GroupDataType **GroupData, unsigned int *uiGroupLen, char* cHtmlData, int uiHtmlLen)
{
	unsigned int iPos = 0;
	int n, ret = 0;
	int iBeginPos, iEndPos, i, iCntOpenedTags, iEnteredInTag, iCntClosingTag, iType;
	GroupDataType *GData = NULL;
	int iRes = 0;
	unsigned int uiDataCnt = 0;

	if (spSearchSettings[uiParamNum].EndPageKeysListLen)
	{
		int iNewHtmlLen = 0;
		for (n = 0; n < (int)spSearchSettings[uiParamNum].EndPageKeysListLen; n++)
		{
			lPrint("MfcWebAgent: Search EndPageKey '%s'", spSearchSettings[uiParamNum].EndPageKeysList[n].Word);
				
			iNewHtmlLen = SearchStrInDataCaseIgn(cHtmlData, uiHtmlLen, 0, spSearchSettings[uiParamNum].EndPageKeysList[n].Word);
			if (iNewHtmlLen > 0) 
			{
				lPrint("MfcWebAgent: EndPageKey detected '%s' pos: %i (%i)", spSearchSettings[uiParamNum].EndPageKeysList[n].Word, iNewHtmlLen, uiHtmlLen);
				uiHtmlLen = iNewHtmlLen;
				break;
			}
		}
		if (iNewHtmlLen <= 0) lPrint("MfcWebAgent: EndPageKeys not detected");
	}

	do
	{
		iBeginPos = SearchStrInDataCaseIgn(cHtmlData, uiHtmlLen, iPos, spSearchSettings[uiParamNum].BeginTag);
		if (iBeginPos > 0)
		{
			iRes = 1;
			//iBeginPos += strlen(spSearchSettings[uiParamNum].BeginTag);
			//lPrint("MfcWebAgent: Finded BeginTag pos: %i", iBeginPos);

			iBeginPos--;
			ret = 0;
			iCntOpenedTags = 0;		//Detected Open Tag
			iEnteredInTag = 0;		//Detected Tag
			iCntClosingTag = 0;		//Detected Closing Tag
			iType = 0;

			for (; iBeginPos > 0; iBeginPos--)
				if (cHtmlData[iBeginPos] == 60)	break;

			/*int iBeginTagLen = strlen(spSearchSettings[uiParamNum].BeginTag);

			for (i = 0; i < iBeginTagLen; i++)
			{
				if (spSearchSettings[uiParamNum].BeginTag[i] == 60)	iCntOpenedTags++;
				if (spSearchSettings[uiParamNum].BeginTag[i] == 62)	iCntOpenedTags--;
			}
			if (iCntOpenedTags < 0) iCntOpenedTags = 0;
			if (iCntOpenedTags == 0) iEnteredInTag = 1;*/

			//lPrint("MfcWebAgent: %i %i %i", iCntOpenedTags, iEnteredInTag, iCntClosingTag);

			//SaveData(&cHtmlData[iBeginPos], 2048, "sector0.html");

			for (i = iBeginPos; i < uiHtmlLen; i++)
			{
				//lPrint("MfcWebAgent: ### %i %i  %c", i, cHtmlData[i], cHtmlData[i]);

				if ((cHtmlData[i] == 60) && ((uiHtmlLen - i) > 10) && (cHtmlData[i+1] != 33))						//	"<"
				{
					iEnteredInTag = 1;
					if (cHtmlData[i + 1] == 47)	
					{
						iCntClosingTag = 1;						//	"</"
						//lPrint("!!!:%.10s", &cHtmlData[i]); 
					}
					else 
					{
						//lPrint("+++:%.10s", &cHtmlData[i]);
						iCntOpenedTags++;
					}
					if (SearchStrInDataCaseIgn(&cHtmlData[i], 7, 0, "<img ") > 0) iType = 1;
					if (SearchStrInDataCaseIgn(&cHtmlData[i], 7, 0, "<meta ") > 0) iType = 2;
					if (SearchStrInDataCaseIgn(&cHtmlData[i], 7, 0, "<br>") > 0) iType = 3;
					if (SearchStrInDataCaseIgn(&cHtmlData[i], 7, 0, "<wbr>") > 0) iType = 4;
					if (SearchStrInDataCaseIgn(&cHtmlData[i], 7, 0, "<link ") > 0) iType = 5;					
				}
				
				if (iEnteredInTag && (cHtmlData[i] == 62))								//	">"
				{
					iEnteredInTag = 0;
					
					if ((i > iBeginPos) && (cHtmlData[i - 1] == 47)) 
					{
						//lPrint("xxx:%.10s", &cHtmlData[i]);
						iType = 100;		//"/>"
					} //else lPrint("---:%.10s", &cHtmlData[i]);
					if (iType) 
					{
						iType = 0;
						iCntOpenedTags--;
					}	
					if (iCntClosingTag)
					{
						iCntClosingTag = 0;
						iCntOpenedTags--;
					}
				}
				
				//	lPrint("MfcWebAgent: %c %i %i %i", cHtmlData[i], iCntOpenedTags, iEnteredInTag, iCntClosingTag);
				if (iCntOpenedTags <= 0) 
				{
					//lPrint("MfcWebAgent: %.10s %i %i %i", &cHtmlData[i-5], iCntOpenedTags, iEnteredInTag, iCntClosingTag);
					iEndPos = i + 1; 
					break;
				}
				//if ((i - iBeginPos) > 1000) {iEndPos = uiHtmlLen; break;}
			}
		//	lPrint("MfcWebAgent: Endpos %i %i", iEndPos, i >= uiHtmlLen);
			if (i >= uiHtmlLen)
			{
				iEndPos = 0;
				lPrint("MfcWebAgent: Not finded end for BEGIN KEY TAG (from: %i) (Lost:%i): '%s'", iBeginPos, iCntOpenedTags, spSearchSettings[uiParamNum].Name);
			}	
			if (strlen(spSearchSettings[uiParamNum].EndTag))
			{
				if (iEndPos) 
				{
					i = SearchStrInDataCaseIgn(cHtmlData, uiHtmlLen, iEndPos, spSearchSettings[uiParamNum].EndTag);
					if (i > 0) 
						iEndPos = i;
						else 
						{
							lPrint("MfcWebAgent: Not finded END KEY TAG(%s): '%s'", spSearchSettings[uiParamNum].EndTag, spSearchSettings[uiParamNum].Name);
							iRes = -2;
						}	
				}
				else
				{
					iEndPos = SearchStrInDataCaseIgn(cHtmlData, uiHtmlLen, iBeginPos, spSearchSettings[uiParamNum].EndTag);
					if (iEndPos < 0) 
					{
						iEndPos = 0;
						lPrint("MfcWebAgent: Not finded END KEY TAG(%s): '%s'", spSearchSettings[uiParamNum].EndTag, spSearchSettings[uiParamNum].Name);
						iRes = -3;
					}	
				}
			}
			//lPrint("MfcWebAgent: Endpos %i", iEndPos);
			if (iEndPos)
			{
				uiDataCnt++;
				GData = (GroupDataType*)realloc(GData, uiDataCnt * sizeof(GroupDataType));
				memset(&GData[uiDataCnt - 1], 0, sizeof(GroupDataType));
				GData[uiDataCnt - 1].BodySize = iEndPos - iBeginPos + 1;
				GData[uiDataCnt - 1].Body = (char*)malloc(GData[uiDataCnt - 1].BodySize);
				memcpy(GData[uiDataCnt - 1].Body, &cHtmlData[iBeginPos], GData[uiDataCnt - 1].BodySize - 1);
				GData[uiDataCnt - 1].Body[GData[uiDataCnt - 1].BodySize - 1] = 0;
				//lPrint("MfcWebAgent: Last symb '%s'", &GData[uiDataCnt - 1].Body[GData[uiDataCnt - 1].BodySize - 2]);
				//lPrint("MfcWebAgent: '%s'", GData[uiDataCnt - 1].Body);
				iPos = iEndPos;
				//SaveData(GData[uiDataCnt - 1].Body, GData[uiDataCnt - 1].BodySize, "sector.html"); 
			} 
			else
			{
				lPrint("MfcWebAgent: Not finded END KEY TAG or not closed tag: '%s'", spSearchSettings[uiParamNum].Name);
				iRes = -1;
				break;
			}
		}
		else
		{
			if (uiDataCnt == 0) lPrint("MfcWebAgent: Not finded BEGIN KEY TAG: '%s'", spSearchSettings[uiParamNum].Name);
			break;
		}	
	} while (iBeginPos);
	lPrint("MfcWebAgent: Total finded(%i): %i", uiParamNum, uiDataCnt);


	if (uiDataCnt)
	{	
		*uiGroupLen = uiDataCnt;
		*GroupData = GData;
	}
	else
	{
		*uiGroupLen = 0;
		*GroupData = NULL;
	}	

	return iRes;
}

int GenerateKeys(Search_Param *sParam)
{
	int i, iLen;
	GroupDataType *GroupData = sParam->GroupData;
	int uiGroupLen = sParam->GroupLen;

	//for (i = 0; i < uiGroupLen; i++) lPrint("MfcWebAgent: GenerateKeys : Size (%i) '%i'", i, GroupData[i].BodySize);

	char *cBuffer = NULL;
	
	for (i = 0; i < uiGroupLen; i++)
	{
		cBuffer = NULL;
		iLen = ExtractHtmlText(GroupData[i].Body, GroupData[i].BodySize, &cBuffer);
		
		GroupData[i].Text = (char*)malloc(iLen + 1);
		memcpy(GroupData[i].Text, cBuffer, iLen);
		GroupData[i].Text[iLen] = 0;
		
		free(cBuffer);
		ClearHtmlSpecs(GroupData[i].Text, &iLen);
		//lPrint("MfcWebAgent: Text : '%s'", GroupData[i].Text);
	}
	
	for (i = 0; i < uiGroupLen; i++)
	{
		GroupData[i].SearchKey = NULL;
		cBuffer = NULL;		
		iLen = RenderKeyData(&GroupData[i], sParam->SearchKeyTags, &cBuffer);

		GroupData[i].SearchKey = (char*)malloc(iLen + 1);
		if (iLen) memcpy(GroupData[i].SearchKey, cBuffer, iLen);
		GroupData[i].SearchKey[iLen] = 0;
				
		free(cBuffer);				
		//lPrint("MfcWebAgent: Result Search key '%s'", GroupData[i].SearchKey);
	}
	
	for (i = 0; i < uiGroupLen; i++)
	{
		GroupData[i].ShowKey = NULL;
		cBuffer = NULL;		
		iLen = RenderKeyData(&GroupData[i], sParam->ShowKeyTags, &cBuffer);
		if (iLen)
		{
			GroupData[i].ShowKey = (char*)malloc(iLen + 1);
			memcpy(GroupData[i].ShowKey, cBuffer, iLen);
			GroupData[i].ShowKey[iLen] = 0;			
			ClearHtmlSpecs(GroupData[i].ShowKey, &iLen);
		}
		else
		{
			GroupData[i].ShowKey = (char*)malloc(1);
			GroupData[i].ShowKey[0] = 0;
		}
		free(cBuffer);
		//lPrint("MfcWebAgent: Result Show key '%s'", GroupData[i].ShowKey);
	}
	
	for (i = 0; i < uiGroupLen; i++)
	{
		GroupData[i].ClickCmd = NULL;
		cBuffer = NULL;		
		iLen = RenderKeyData(&GroupData[i], sParam->ClickCmdTags, &cBuffer);
		
		GroupData[i].ClickCmd = (char*)malloc(iLen + 1);
		if (iLen) memcpy(GroupData[i].ClickCmd, cBuffer, iLen);
		GroupData[i].ClickCmd[iLen] = 0;

		free(cBuffer);				
		//lPrint("MfcWebAgent: Result Click Cmd '%s'", GroupData[i].ClickCmd);
	}
	
	for (i = 0; i < uiGroupLen; i++)
	{
		GroupData[i].ImageUrl = NULL;
		cBuffer = NULL;		
		iLen = RenderKeyData(&GroupData[i], sParam->ImageUrlTags, &cBuffer);
		
		GroupData[i].ImageUrl = (char*)malloc(iLen + 1);
		if (iLen) memcpy(GroupData[i].ImageUrl, cBuffer, iLen);
		GroupData[i].ImageUrl[iLen] = 0;

		free(cBuffer);				
		//lPrint("MfcWebAgent: Result Image Url '%s'", GroupData[i].ImageUrl);
	}
	
	for (i = 0; i < uiGroupLen; i++)
	{
		GroupData[i].FilterKey = NULL;
		cBuffer = NULL;		
		iLen = RenderKeyData(&GroupData[i], sParam->FilterKeyTags, &cBuffer);
		if (iLen)
		{
			GroupData[i].FilterKey = (char*)malloc(iLen + 1);
			memcpy(GroupData[i].FilterKey, cBuffer, iLen);
			GroupData[i].FilterKey[iLen] = 0;			
			ClearHtmlSpecs(GroupData[i].FilterKey, &iLen);
		}
		else
		{
			GroupData[i].FilterKey = (char*)malloc(1);
			GroupData[i].FilterKey[0] = 0;
		}
		free(cBuffer);
		//lPrint("MfcWebAgent: Result Show key '%s'", GroupData[i].ShowKey);
	}
	
	for (i = 0; i < uiGroupLen; i++)
	{
		GroupData[i].MailKey = NULL;
		cBuffer = NULL;		
		iLen = RenderKeyData(&GroupData[i], sParam->MailKeyTags, &cBuffer);
		if (iLen)
		{
			GroupData[i].MailKey = (char*)malloc(iLen + 1);
			memcpy(GroupData[i].MailKey, cBuffer, iLen);
			GroupData[i].MailKey[iLen] = 0;			
			ClearHtmlSpecs(GroupData[i].MailKey, &iLen);
		}
		else
		{
			GroupData[i].MailKey = (char*)malloc(1);
			GroupData[i].MailKey[0] = 0;
		}
		free(cBuffer);
		//lPrint("MfcWebAgent: Result Show key '%s'", GroupData[i].ShowKey);
	}
		
	return 1;
}

int ClearHtmlSpecs(char *cBuff, int *iOldLen)
{
	int i;
	int iLen = *iOldLen;
	int iNewLen = 0;
	int iMode = 0;
	char cPrev = 0;
	
	for (i = 0; i < iLen; i++)
	{
		if ((iMode == 0) && (cBuff[i] == 38)) iMode = 1;
		if ((iMode == 0) && ((cBuff[i] < 0) || (cBuff[i] > 31)) && ((cPrev != 32) || (cPrev != cBuff[i])))
		{
			if (cBuff[i] != 124) 
			{
				cBuff[iNewLen] = cBuff[i];
				cPrev = cBuff[i];
			}
			else
			{
				cBuff[iNewLen] = 32;
				cPrev = 32;
			}
			iNewLen++;			
		}
		if ((iMode == 1) && (cBuff[i] == 59)) 
		{
			iMode = 0;
			if (cPrev != 32)
			{
				cBuff[iNewLen] = 32;
				cPrev = 32;
				iNewLen++;
			}
		}	
	}
	
	if (iLen > iNewLen) memset(&cBuff[iNewLen], 0, iLen - iNewLen);
	*iOldLen = iNewLen;
	return 1;
}

int ClearSpecsSymb(unsigned char *cBuff, int *iOldLen)
{
	int i;
	int iLen = *iOldLen;
	int iNewLen = 0;
	unsigned char cPrev = 0;
	
	for (i = 0; i < iLen; i++)
	{
		if (cBuff[i] > 31)
		{
			if (cBuff[i] == 124) cBuff[i] = 32;
			if ((cPrev != 32) || (cBuff[i] != 32))
			{				
				cBuff[iNewLen] = cBuff[i];
				cPrev = cBuff[i];			
				iNewLen++;			
			}
		}
		else
		{
			if (cPrev != 32)
			{
				cBuff[iNewLen] = 32;
				cPrev = 32;
				iNewLen++;
			}
		}
	}
	
	if (iLen > iNewLen) memset(&cBuff[iNewLen], 0, iLen - iNewLen);
	*iOldLen = iNewLen;
	return 1;
}

int ExtractHtmlText(char *cHtmlData, int iHtmlSize, char **cOutBuffer)
{
	int i;
	int iCnt1 = 0;
	int iCnt2 = 0;
	int iLen = 0;
	int iSize = 1024;
	char cPrev = 0;
	char *cBuff = (char*)malloc(iSize);

	for (i = 0; i < iHtmlSize; i++)
	{
		if (cHtmlData[i] == 60) iCnt1 = 1;								//	"<"
		if (cHtmlData[i] == 62)	iCnt1 = 0;								//	">"
		if ((iCnt1 == 0) && (cHtmlData[i] == 38)) iCnt2 = 1;			//	"&"	
		
		if ((iCnt1 == 0) && (iCnt2 == 0) && (cHtmlData[i] != 60) && (cHtmlData[i] != 62) && ((cPrev != 32) || (cPrev != cHtmlData[i])))
		{
			cBuff[iLen] = cHtmlData[i];
			cPrev = cHtmlData[i];
			iLen++;
			if (iLen >= iSize)
			{
				iSize += 1024;
				cBuff = (char*)realloc(cBuff, iSize);
			}
		}
		
		if ((iCnt1 == 0) && (cHtmlData[i] == 59) && iCnt2) iCnt2 = 0;	//	";"		
	}
	*cOutBuffer = cBuff;
	return iLen;
}

int RenderKeyData(GroupDataType *GroupData, char *cMaskStr, char **cOutBuffer)
{
	char *cHtmlData = GroupData->Body;
	unsigned int uiHtmlSize = GroupData->BodySize;

	int i, iFirst, n;
	int iMaskSize = strlen(cMaskStr);
	int iCnt = 0;
	int iSize = 128;
	int iLen = 0;
	char *cBuffer = (char*)malloc(iSize);
	char *cBuff;
	char cRes;

	for (i = 0; i < iMaskSize; i++)
	{
		if (cMaskStr[i] == 60) 
		{
			iCnt++;	
			iFirst = i + 1;
		}
		if (iCnt == 0) 
		{
			cBuffer[iLen] = cMaskStr[i];
			iLen++;
			if (iLen >= iSize)
			{
				iSize += 128;
				cBuffer = (char*)realloc(cBuffer, iSize);
			}
			cBuffer[iLen] = 0;
		}
		
		if (cMaskStr[i] == 62) 
		{
			iCnt--;
			n = i - iFirst;
			cBuff = (char*)malloc(n + 1);
			memcpy(cBuff, &cMaskStr[iFirst], n);
			cBuff[n] = 0;
			cRes = 0;

			//lPrint("RenderKeyData1: %s", cBuff);
		
					
			if ((n == 4) && (SearchStrInDataCaseIgn(cBuff, n, 0, "Text") > 0))
			{
				n = strlen(GroupData->Text);
				if ((iLen + n + 1) >= iSize)
				{
					iSize += n + 128;
					cBuffer = (char*)realloc(cBuffer, iSize);
				}
				memcpy(&cBuffer[iLen], GroupData->Text, n);
				iLen += n;
				cBuffer[iLen] = 0;
				cRes = 1;
			}
			/*if ((cBuff[0] == "#") && (n == 1))
			{
				n = 1;
				if ((iLen + n + 1) >= iSize)
				{
					iSize += n + 128;
					cBuffer = (char*)realloc(cBuffer, iSize);
				}
				cBuffer[iLen] = 13;
				iLen++;
				cBuffer[iLen] = 0;
				cRes = 1;
			}*/
		
			if (SearchStrInDataCaseIgn(cBuff, n, 0, "Replace:") > 0)
			{
				char *cNewStr = NULL;
				char *cOldSubStr = NULL;
				char *cNewSubStr = NULL;
				int iFirstPos = strlen("Replace:");
				int iStrNum = 0;

				for (int k = iFirstPos; k < n; k++)
				{
					if ((iStrNum == 0) && (cBuff[k] == 61) && ((k + 1) < n))
					{
						iStrNum = k - iFirstPos;
						if (iStrNum > 0)
						{
							cOldSubStr = (char*)malloc(iStrNum + 1);
							memcpy(cOldSubStr, &cBuff[iFirstPos], iStrNum);
							cOldSubStr[iStrNum] = 0;
							iFirstPos = k + 1;
							iStrNum = 1;
							break;
						} else iStrNum = 0;
					}
				}
				if (iStrNum)
				{
					iStrNum = n - iFirstPos;
					cNewSubStr = (char*)malloc(iStrNum + 1);
					memcpy(cNewSubStr, &cBuff[iFirstPos], iStrNum);
					cNewSubStr[iStrNum] = 0;

					//lPrint("########### '%s' '%s' '%s'", cBuffer, cOldSubStr, cNewSubStr);
					
					if (ReplaceSubstr(&cBuffer, (unsigned int*)&iLen, cOldSubStr, cNewSubStr, 1) > 0) iSize = iLen + 1;
					cRes = 1;
				}				
			}
			if (cRes == 0)
			{
				char *cParam = NULL;
				n = GetHtmlParam(cHtmlData, uiHtmlSize, cBuff, n, &cParam, 0);
				if (n)
				{
			
					if ((iLen + n + 1) >= iSize)
					{
						iSize += n + 128;
						cBuffer = (char*)realloc(cBuffer, iSize);
					}
					memcpy(&cBuffer[iLen], cParam, n);
					iLen += n;
					cBuffer[iLen] = 0;
			
				}
				free(cParam);
			}
			free(cBuff);			
		}	
	}

	*cOutBuffer = cBuffer;
	return iLen;
}

int GetHtmlParam(char *cHtmlData, unsigned int uiHtmlSize, char *cParam, unsigned int uiParamSize, char **cOutValue, char cDebug)
{
	int i, k;
	int ret = 0;
	int iSpecFlag = 0;
	int iParamPos = 0;
	int iFirstPos = 0;
	int iNextPos = 0;
	int iLastPos = 0;
	char cLastSymb[2] = {0,0};
	char *cSpecTag = NULL;
	char *cParamName = NULL;
	int iParamNameSize = 0;

	if (cDebug) lPrint("MfcWebAgent: GetHtmlParam: Begin: cParam '%s' Size %i", cParam, uiHtmlSize);

	for (i = 0; i < (int)uiParamSize; i++) 
		if (cParam[i] == 46) 
		{
			cSpecTag = (char*)malloc(i + 3);
			cSpecTag[0] = 60;
			memcpy(&cSpecTag[1], cParam, i);
			cSpecTag[i + 1] = 32;
			cSpecTag[i + 2] = 0;
			iSpecFlag = 1;
			cLastSymb[0] = 62;
			if (i < (int)(uiParamSize - 1)) iParamPos = i + 1;
			//if (cDebug) lPrint("MfcWebAgent: GetHtmlParam: Search Tag '%s' param '%s'", cSpecTag, cParam);
			break;
		}

	iParamNameSize = uiParamSize - iParamPos + 2;
	cParamName = (char*)malloc(iParamNameSize + 1);	
	cParamName[0] = 32;
	memcpy(&cParamName[1], &cParam[iParamPos], iParamNameSize - 1);
	cParamName[iParamNameSize - 1] = 61;
	cParamName[iParamNameSize] = 0;
	if (cDebug) lPrint("MfcWebAgent: GetHtmlParam: ParamName '%s'", cParamName);
	do 
	{
		//Ищем Тег если он указан
		if (iSpecFlag)
		{
			if (cDebug) lPrint("MfcWebAgent: GetHtmlParam: Search: Tag '%s' Len: %i '%.30s'", cSpecTag, uiHtmlSize, &cHtmlData[iFirstPos]);
			i = SearchStrInDataCaseIgn(cHtmlData, uiHtmlSize, iFirstPos, cSpecTag);
			if (i > 0) 
			{
				iFirstPos = i + strlen(cSpecTag) - 2;
				if (cDebug) lPrint("MfcWebAgent: GetHtmlParam: Finded: Tag in '%i' Len: %i '%.30s'", iFirstPos, uiHtmlSize - iFirstPos, &cHtmlData[iFirstPos]);
			} 
			else 
			{
				if (cDebug) lPrint("MfcWebAgent: GetHtmlParam: NOT Finded: Tag in '%i' Len: %i '%.30s'", iFirstPos, uiHtmlSize - iFirstPos, &cHtmlData[iFirstPos]);			
				iFirstPos = i;
				//SaveData(&cHtmlData[iFirstPos], uiHtmlSize, "Sector.html");
				//SaveData(cSpecTag, strlen(cSpecTag), "Sector0.html");
			}
		}
		
		//Ищем Текст в теге если нужно
		if (SearchStrInDataCaseIgn(cParamName, iParamNameSize, 0, " TEXT=") == 1)
		{
			if (cDebug) lPrint("MfcWebAgent: GetHtmlParam: Searching Text");
			for (i = iFirstPos; i < (int)uiHtmlSize; i++) if (cHtmlData[i] == 62) break;
			i++;
			if (i < (int)uiHtmlSize)
			{
				//if (cDebug) lPrint("!!! '%.30s'", &cHtmlData[i]);			
				k = i;
				for (; i < (int)uiHtmlSize; i++) if (cHtmlData[i] == 60) break;
				//if (cDebug) lPrint("!!!! '%.30s'", &cHtmlData[i]);
				ret = i - k;
				*cOutValue = (char*)malloc(ret + 1);
				memcpy(*cOutValue, &cHtmlData[k], ret);
				(*cOutValue)[ret] = 0;
				if (cDebug) lPrint("MfcWebAgent: GetHtmlParam: Text founded: '%s'", *cOutValue);
			}
			else
			{
				if (cDebug) lPrint("MfcWebAgent: GetHtmlParam: Text not found");			
				*cOutValue = (char*)malloc(1);
				(*cOutValue)[0] = 0;
			}
			break;
		}

		
		//Ищем значение параметра с окончанием '=' до символа '>'
		if (cDebug) lPrint("MfcWebAgent: GetHtmlParam: Search '%s' to '>'", cParamName);
		cLastSymb[0] = 62;
		iNextPos = SearchStrInDataCaseIgnBeforeSymb(cHtmlData, uiHtmlSize, iFirstPos, cParamName, cLastSymb[0]);
		if (iNextPos <= 0)
		{
			//Ищем значение параметра с окончанием ' ' до символа '>'
			cParamName[iParamNameSize - 1] = 32;
			if (cDebug) lPrint("MfcWebAgent: GetHtmlParam: Search '%s' to '>'", cParamName);
			iNextPos = SearchStrInDataCaseIgnBeforeSymb(cHtmlData, uiHtmlSize, iFirstPos, cParamName, cLastSymb[0]);
		}
		if (cDebug)
		{
			if (iNextPos <= 0) 
				lPrint("MfcWebAgent: GetHtmlParam: Param not found");
				else
				lPrint("MfcWebAgent: GetHtmlParam: Param: '%s' found on pos:%i ", cParamName, iNextPos);
		}

		if (iNextPos > 0)
		{
			iNextPos += iParamNameSize - 2;
			if (cDebug) lPrint("MfcWebAgent: GetHtmlParam: Search begin pos for value from %i to [=]", iNextPos);
			//if ((cHtmlData[iNextPos] == 34) || (cHtmlData[iNextPos] == 32))
			{
				for (; iNextPos < (int)uiHtmlSize; iNextPos++) if (cHtmlData[iNextPos] == 61) break;
				if (iNextPos >= (int)uiHtmlSize) break;
				iNextPos++;
				if (cDebug) lPrint("MfcWebAgent: GetHtmlParam: Skip [ ] from begin");
				for (; iNextPos < (int)uiHtmlSize; iNextPos++) if (cHtmlData[iNextPos] != 32) break;
				if (iNextPos >= (int)uiHtmlSize) break;	
				
				if (cDebug) lPrint("MfcWebAgent: GetHtmlParam: Search done");			
				if (iNextPos < (int)(uiHtmlSize - 3))
				{
					if (cHtmlData[iNextPos] == 34) {cLastSymb[0] = 34; iNextPos++;} else cLastSymb[0] = 62;
					
					if (cDebug) lPrint("MfcWebAgent: GetHtmlParam: Search end pos for value from %i to [%c]", iNextPos, cLastSymb[0]);
					
					for (i = iNextPos; i < (int)uiHtmlSize; i++)
						if (((cLastSymb[0] == 34) && (cHtmlData[i] == 34))
							||
							((cLastSymb[0] == 62) && ((cHtmlData[i] == 62) || (cHtmlData[i] == 32))))
						{
							ret = i - iNextPos;
							*cOutValue = (char*)malloc(ret + 1);
							memcpy(*cOutValue, &cHtmlData[iNextPos], ret);
							(*cOutValue)[ret] = 0;
							if (cDebug) lPrint("MfcWebAgent: GetHtmlParam: Param finded '%s'", *cOutValue);
							break;
						}
					if (i >= (int)uiHtmlSize) iNextPos = -2;	
				} else break;
			} 				
			/*	else 
			{
				cLastSymb[0] = iNextPos;
				iNextPos = -2;
			}*/
		}
		if (iNextPos == -2) 
		{
			iFirstPos = SearchStrInDataCaseIgn(cHtmlData, uiHtmlSize, iFirstPos, cLastSymb);
		} else break;		
	} while(iFirstPos != -1);
	if (cSpecTag) free(cSpecTag);
	if (cParamName) free(cParamName);

	if (cDebug) 
	{
		if (ret) 
			lPrint("MfcWebAgent: GetHtmlParam: End: cParam '%s:%s'", cParam, *cOutValue);
			else
			lPrint("MfcWebAgent: GetHtmlParam: End: cParam '%s' not finded", cParam);
	}
	
	if ((ret == 0) && !cDebug) GetHtmlParam(cHtmlData, uiHtmlSize, cParam, uiParamSize, cOutValue, 1);
	
	return ret;	
}

void ShowFirstMessage()
{
	cMessageReaded = 0;
	unsigned int i, n, ret;
	char cStat = 0;
	for (n = 0; n < uiSearchSettCount; n++)
	{
		for (i = 0; i < spSearchSettings[n].HistoryCount; i++)					
			if (spSearchSettings[n].HistoryData[i].Status == 0)
			{
				uiShowSett = n;
				uiShowPos = i;

				m_wndTaskbarNotifier1.BlockSection.Lock();
				if (m_picture) 
				{
					delete m_picture; 
					m_picture = NULL;
				}
				//lPrint("MfcWebAgent: Get0 Load image %s", spSearchSettings[n].ImageUrlTags);
					
				if (strlen(spSearchSettings[n].ImageUrlTags) && strlen(spSearchSettings[n].HistoryData[i].ImageUrl))
				{
					char cPath[MAX_PATH];
					char *pImageBuffer = NULL;
					unsigned int uiLen = 0;
					memset(cPath, 0, MAX_PATH);
					strcpy(cPath, AppCookiePath);
					strcat(cPath, spSearchSettings[n].Name);
					strcat(cPath, ".txt");
					//lPrint("MfcWebAgent: Get Load image %s", spSearchSettings[n].HistoryData[i].ImageUrl);
					ret = DownloadAddress(&pImageBuffer, &uiLen, spSearchSettings[n].HistoryData[i].ImageUrl, spSearchSettings[n].Login, spSearchSettings[n].Password, cPath, cBrowserString);
					if (ret)
					{	
						//lPrint("MfcWebAgent: Loaded image %i size %i", ret, uiLen);
						if (m_hBuffer) ::GlobalFree(m_hBuffer);
						m_hBuffer  = ::GlobalAlloc(GMEM_MOVEABLE, uiLen);
						if (m_hBuffer)
						{
							void* pBuffer = ::GlobalLock(m_hBuffer);
							if (pBuffer)
							{
								CopyMemory(pBuffer, pImageBuffer, uiLen);
								IStream* pStream = NULL;
								if (::CreateStreamOnHGlobal(m_hBuffer, FALSE, &pStream) == S_OK)
								{
									m_picture = Gdiplus::Image::FromStream(pStream);
									if (m_picture)
									{ 
										if (m_picture->GetLastStatus() != Gdiplus::Ok) 
										{
											lPrint("MfcWebAgent: Error load attach in GDI %i %i", m_picture->GetLastStatus(), GetLastError());								
										}	
									}
									pStream->Release();
																						
								}
								::GlobalUnlock(m_hBuffer);
							}				
							//	m_hBuffer = NULL;
						}
						if (pImageBuffer) free(pImageBuffer);
					}
				}
				m_wndTaskbarNotifier1.pGdiImage = m_picture;
				lPrint("MfcWebAgent: Show == '%s'", spSearchSettings[n].HistoryData[i].ShowKey);
				m_wndTaskbarNotifier1.Show(spSearchSettings[n].Name, spSearchSettings[n].HistoryData[i].ShowKey, spSearchSettings[n].HistoryData[i].ClickCmd, 500, uiShowTime, 1, 3);
				cStat = 1;
				break;
			}
			if (cStat) break;
	}	
}

void ShowErrorMessage(char* cMessLogo, char* cMessCaption, char* cMessBody)
{
	cMessageReaded = 0;
	uiShowSett = uiSearchSettCount;
				
	m_wndTaskbarNotifier1.BlockSection.Lock();
	if (m_picture) 
	{
		delete m_picture; 
		m_picture = NULL;
	}
				
	m_wndTaskbarNotifier1.pGdiImage = m_picture;
	lPrint("MfcWebAgent: Show == '%s'", cMessCaption);
	m_wndTaskbarNotifier1.Show(cMessLogo, cMessCaption, cMessBody, 500, uiShowTime, 1, 3);

}

void CMfcWebAgentDlg::OnChekNow() 
{
	BlockSection.Lock();
	unsigned int n;
	char cStat = 0;
	for (n = 0; n < uiSearchSettCount; n++)
	{
		if (spSearchSettings[n].Status)
		{
			spSearchSettings[n].Counter = spSearchSettings[n].ScanInterval;
			spSearchSettings[n].CounterNA = NA_TIMER_SKIP;
		}
	}
	uiPauseTimer = 0;
	BlockSection.Unlock();
}

void CMfcWebAgentDlg::OnCheckCurrent() 
{
	BlockSection.Lock();

	int iCurSel = m_ParamList.GetCurSel();
	if (iCurSel == -1) return;
	int iCurNum = m_ParamList.GetItemData(iCurSel);

	if ((iCurNum < 0) || (iCurNum >= (int)uiSearchSettCount))
	{
		BlockSection.Unlock();
		lPrint("MfcWebAgent: Error selected param OnCheckCurrent", iCurNum, uiSearchSettCount);
		return;
	}

	spSearchSettings[iCurNum].Counter = spSearchSettings[iCurNum].ScanInterval;
	spSearchSettings[iCurNum].CounterNA = NA_TIMER_SKIP;
	uiPauseTimer = 0;

	BlockSection.Unlock();
}

void CopyToBuffer(char *cText)
{
	CString source = cText;
	
	if(OpenClipboard(0))
	{
		HGLOBAL hgBuffer;
		char* chBuffer;
		EmptyClipboard(); 
		hgBuffer= GlobalAlloc(GMEM_DDESHARE, source.GetLength()+1);
		chBuffer= (char*)GlobalLock(hgBuffer);
		strcpy(chBuffer, LPCSTR(source));
		GlobalUnlock(hgBuffer);
		SetClipboardData(CF_TEXT, hgBuffer);
		CloseClipboard(); 
	}
}

void CMfcWebAgentDlg::OnExit() 
{
	DestroyWindow();
}

void CMfcWebAgentDlg::OnSavePage() 
{
	char *pBuffer = NULL;
	unsigned int uiLen = 0;
		
	int iCurSel = m_ParamList.GetCurSel();
	if (iCurSel == -1) return;
	int iCurNum = m_ParamList.GetItemData(iCurSel);

	if ((iCurNum < 0) || (iCurNum >= (int)uiSearchSettCount))
	{
		lPrint("MfcWebAgent: Error selected param OnSavePage", iCurNum, uiSearchSettCount);
		return;
	}
		
	int res;
	char cPath[MAX_PATH];
	memset(cPath, 0, MAX_PATH);
	strcpy(cPath, AppCookiePath);
	strcat(cPath, spSearchSettings[iCurNum].Name);
	strcat(cPath, ".txt");
	//lPrint("MfcWebAgent: COOKIE FILE '%s'", cPath);
	lPrint("MfcWebAgent: Get Download page '%s'", spSearchSettings[iCurNum].Name);	
	res = DownloadAddress(&pBuffer, &uiLen, spSearchSettings[iCurNum].URL, spSearchSettings[iCurNum].Login, spSearchSettings[iCurNum].Password, cPath, cBrowserString);
	if (res)
	{
		lPrint("MfcWebAgent: Downloaded page '%s'", spSearchSettings[iCurNum].Name);	
		memset(cPath, 0, MAX_PATH);
		strcpy(cPath, spSearchSettings[iCurNum].Name);
		strcat(cPath, ".html");
		SaveData(pBuffer, uiLen, cPath);

		memset(cPath, 0, MAX_PATH);
		strcpy(cPath, spSearchSettings[iCurNum].Name);
		strcat(cPath, "_clean.html");
		ClearSpecsSymb((unsigned char*)pBuffer, (int*)&uiLen);
		SaveData(pBuffer, uiLen, cPath);
		
		lPrint("MfcWebAgent: Saved page '%s'", spSearchSettings[iCurNum].Name);
		MessageBox(spSearchSettings[iCurNum].Name, "Download successful", MB_OK);
	} 
	else
	{
		lPrint("MfcWebAgent: Error download page: '%s'", spSearchSettings[iCurNum].Name);
		MessageBox(spSearchSettings[iCurNum].Name, "Download error", MB_OK);
	}
}

int ReleaseMinHistory(GroupDataType *gdtHistory, unsigned int uiHistoryCount)
{
	if (!uiHistoryCount) return 0;
	int iRet = uiHistoryCount - 1;
	unsigned int iMinCount = 99999999;

	for (unsigned int m = 0; m < uiHistoryCount; m++)
		if (iMinCount > gdtHistory[m].DetectCount) 
		{
			iMinCount = gdtHistory[m].DetectCount;
			iRet = m;
		}
	//lPrint("MfcWebAgent: ReleaseMinHistory %i '%s' '%s'", iMinCount, gdtHistory[iRet].SearchKey, gdtHistory[iRet].ShowKey);	
	if (gdtHistory[iRet].Body)	free(gdtHistory[iRet].Body);
	if (gdtHistory[iRet].Text)	free(gdtHistory[iRet].Text);
	if (gdtHistory[iRet].SearchKey)	free(gdtHistory[iRet].SearchKey);
	if (gdtHistory[iRet].ShowKey)	free(gdtHistory[iRet].ShowKey);
	if (gdtHistory[iRet].ClickCmd)	free(gdtHistory[iRet].ClickCmd);
	if (gdtHistory[iRet].ImageUrl)	free(gdtHistory[iRet].ImageUrl);
	if (gdtHistory[iRet].FilterKey)	free(gdtHistory[iRet].FilterKey);
	if (gdtHistory[iRet].MailKey)	free(gdtHistory[iRet].MailKey);
	memset(&gdtHistory[iRet], 0, sizeof(GroupDataType));
	return iRet;
}


void CMfcWebAgentDlg::OnHideWindow() 
{
	ShowWindow(SW_HIDE);
}

void CMfcWebAgentDlg::OnResetChangeCount() 
{
	char *pBuffer = NULL;
	unsigned int uiLen = 0;
	
	int iCurSel = m_ParamList.GetCurSel();
	if (iCurSel == -1) return;
	int iCurNum = m_ParamList.GetItemData(iCurSel);
	
	if ((iCurNum < 0) || (iCurNum >= (int)uiSearchSettCount))
	{
		lPrint("MfcWebAgent: Error selected param OnResetChangeCount", iCurNum, uiSearchSettCount);
		return;
	}
	
	spSearchSettings[iCurNum].DontChangeCnt = 0;
	m_DontChangeCnt = "0";

	UpdateData(false);
	UpdateShowStatus();	
	AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_MODIFY, IconBmpIdle, IconCaption);	
}

void CMfcWebAgentDlg::OnOpenUrl() 
{
	unsigned int uiLen = 0;
	
	int iCurSel = m_ParamList.GetCurSel();
	if (iCurSel == -1) return;
	int iCurNum = m_ParamList.GetItemData(iCurSel);
	
	if ((iCurNum < 0) || (iCurNum >= (int)uiSearchSettCount))
	{
		lPrint("MfcWebAgent: Error selected param OnOpenUrl", iCurNum, uiSearchSettCount);
		return;
	}
	ShellExecute(0, "Open", spSearchSettings[iCurNum].URL, NULL, NULL, SW_SHOW);
}


void ConvertFilterToList(char *cWords, FilterDataType **ftListOut, unsigned int *uiLestLen)
{
	unsigned int first = 0;
	unsigned int last = 0;
	unsigned int n = 0;
				
	if ((*ftListOut) != NULL) free(*ftListOut);
	*uiLestLen = 0;
	FilterDataType *List = NULL;;
	unsigned int listlen = 0;
	unsigned int wlen = strlen(cWords);
	for (n = 0; n < wlen; n++)
	{
		if ((cWords[n] == 124) || (n == (wlen - 1)))
		{
			if (cWords[n] == 124) last = n; else last = n + 1;
			if ((last > first) && ((last - first) < 128)) 
			{	
				listlen++;
				List = (FilterDataType*)realloc(List, sizeof(FilterDataType) * listlen);
				memset(List[listlen - 1].Word, 0, 128); 
				memcpy(List[listlen - 1].Word, &cWords[first], last - first);
				//lPrint("ConvertFilterToList2 %i '%s'", listlen, List[listlen - 1].Word);
			}	
			first = n + 1;
		}
	}
	*ftListOut = List;
	*uiLestLen = listlen;
}

void SendNews()
{
	unsigned int i, n, ret, b;
	unsigned int uiCont = 0;
	unsigned int uiBodyLen = 0;
	char *cBody = NULL;
	char cStat = 0;
	for (n = 0; n < uiSearchSettCount; n++)
	{
		for (i = 0; i < spSearchSettings[n].HistoryCount; i++)					
			if (spSearchSettings[n].HistoryData[i].Status == 0)
			{
				if (spSearchSettings[n].HistoryData[i].MailKey)
				{			
					spSearchSettings[n].HistoryData[i].Status = 3;
					if (uiCont == 0)
					{
						ret = strlen("<HTML><BODY>");
						cBody = (char*)malloc(ret);
						memcpy(cBody, "<HTML><BODY>", ret);
						uiBodyLen += ret;
					}
					ret = strlen(spSearchSettings[n].HistoryData[i].MailKey);
					cBody = (char*)realloc(cBody, uiBodyLen + ret);
					memcpy(&cBody[uiBodyLen], spSearchSettings[n].HistoryData[i].MailKey, ret);
					
					for (b = 0; b < ret; b++) 
					{
						if (cBody[uiBodyLen + b] == 91) cBody[uiBodyLen + b] = 60;
						if (cBody[uiBodyLen + b] == 93) cBody[uiBodyLen + b] = 62;
					}
					uiBodyLen += ret;
					
					uiCont++;
					if (uiCont >= uiMailingMessCnt) break;
				}
			}
		if (uiCont >= uiMailingMessCnt) break;
	}

	if (uiCont) 
	{
		ret = strlen("</BODY></HTML>");
		cBody = (char*)realloc(cBody, uiBodyLen + ret + 1);
		memcpy(&cBody[uiBodyLen], "</BODY></HTML>", ret);
		uiBodyLen += ret;
		cBody[uiBodyLen] = 0;

		ret = 0;
		if (SendHtmlMail(cMailingDestMail, cMailingEmail, "MfcWebAgent", cBody, uiBodyLen, cMailingServer, cMailingLogin, cMailingPassword, cMailingAuth))
		{
			for (n = 0; n < uiSearchSettCount; n++)
			{
				ret = 0;
				for (i = 0; i < spSearchSettings[n].HistoryCount; i++)					
					if (spSearchSettings[n].HistoryData[i].Status == 3)
					{
						spSearchSettings[n].HistoryData[i].Status = 4;
						ret++;
					}
				if (ret) 
				{
					lPrint("Save %i pos", ret);
					SaveHistory(&spSearchSettings[n]);
				}	
			}
			UpdateShowStatus();
		}
		else
		{
			for (n = 0; n < uiSearchSettCount; n++)
				for (i = 0; i < spSearchSettings[n].HistoryCount; i++)					
					if (spSearchSettings[n].HistoryData[i].Status == 3)
						spSearchSettings[n].HistoryData[i].Status = 0;
		}		
	}	
	if (cBody) free(cBody);	
}

unsigned char base64_decode(char *data, unsigned int input_length, char **out_data, unsigned int *output_length) 
{
	char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	char *decoding_table = (char*)malloc(256);
	int mod_table[] = {0, 2, 1};
	
	unsigned int i, j;
    for (i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;

	if (input_length % 4 != 0) 
	{
		*out_data = NULL;
		*output_length = 0;
		return NULL;
	}
	
    if (output_length) *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;
	
	unsigned char *decoded_data = (unsigned char*)malloc(*output_length);
    if (decoded_data == NULL) 
	{
		*out_data = NULL;
		*output_length = 0;
		return NULL;
	}
	
    for (i = 0, j = 0; i < input_length;) 
	{
		if (data[i] == 0) {i++; continue;}
        unsigned int sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        unsigned int sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        unsigned int sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        unsigned int sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
		
        unsigned int triple = (sextet_a << 3 * 6)
			+ (sextet_b << 2 * 6)
			+ (sextet_c << 1 * 6)
			+ (sextet_d << 0 * 6);
		
        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }
	
	free(decoding_table);
	*out_data = (char*)decoded_data;
    return 1;
}

int base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len, unsigned char** data_encoded, unsigned int *encoded_len) 
{
	char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	unsigned int out_len = 0;
	unsigned char* data_out = (unsigned char*)malloc(in_len * 3);
	memset(data_out, 0, out_len);
	
	while (in_len--) 
	{
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) 
		{
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;
			
			for(i = 0; i <4; i++)
			{
				data_out[out_len] = base64_chars[char_array_4[i]];
				out_len++;
			}
			i = 0;
		}
	}
	
	if (i)
	{
		for(j = i; j < 3; j++) char_array_3[j] = '\0';
		
		char_array_4[0] = ( char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] =   char_array_3[2] & 0x3f;
		
		for (j = 0; (j < i + 1); j++) 
		{
			data_out[out_len] = base64_chars[char_array_4[j]];
			out_len++;
		}
		
		while((i++ < 3))
		{
			data_out[out_len] = '=';
			out_len++;
		}
	}
	*encoded_len = out_len;
	*data_encoded = (unsigned char*)malloc(out_len);
	memcpy(*data_encoded, data_out, out_len);
	free(data_out);
	return 1;
}

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
	struct upload_status *upload_ctx = (struct upload_status *)userp;
	char *data;
	
	if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) 
	{
		return 0;
	}
	
	if (upload_ctx->lines_read < upload_ctx->lines_count) 
	{		
		data = upload_ctx->cMailBody[upload_ctx->lines_read];
		//lPrint("payload ---");
		//lPrint("payload: %i %i %i\n", upload_ctx->lines_read, upload_ctx->lines_count, strlen(upload_ctx->cMailBody[upload_ctx->lines_read]));
		
		size_t len = upload_ctx->BodyLen[upload_ctx->lines_read] - upload_ctx->BodyPos[upload_ctx->lines_read];

		if (len > 76)
		{
			memcpy(ptr, &data[upload_ctx->BodyPos[upload_ctx->lines_read]], 76);
			len = 76;
			upload_ctx->BodyPos[upload_ctx->lines_read] += 76;
		}
		else
		{
			memcpy(ptr, &data[upload_ctx->BodyPos[upload_ctx->lines_read]], len);
			upload_ctx->lines_read++;			
		}
				
		//lPrint(">>>> '%s'", ptr);
		return len;
	}
	return 0;
}

int SendHtmlMail(char *mToAddress, char *mFromAddr, char *mMainText, char *mBodyText, unsigned int uiBodyTextLen, char *mServer, char *mLogin, char *mPassword, char *mAuth)
{	
	CURL *curl = curl_easy_init();
	if (!curl) 
	{
		lPrint("MfcWebAgent: Error init CURL");
		return 0;
	}

	lPrint("MfcWebAgent: Send Email:");
	lPrint("MfcWebAgent: To: %s", mToAddress);
	lPrint("MfcWebAgent: From: %s", mFromAddr);
	lPrint("MfcWebAgent: MainText: %s", mMainText);	
	lPrint("MfcWebAgent: Body: %s", mBodyText);
	lPrint("MfcWebAgent: BodyLenght: %i", uiBodyTextLen);
	lPrint("MfcWebAgent: Server: %s", mServer);
	lPrint("MfcWebAgent: Login: %s", mLogin);
	lPrint("MfcWebAgent: Password: %s", mPassword);
	lPrint("MfcWebAgent: Auth: %s", mAuth);
	
	int n;
	
	int ret = 1;	
	CURLcode res = CURLE_OK;
	struct curl_slist *recipients = NULL;
	char cBuff[128];
	struct upload_status upload_ctx;
	time_t seconds = time(NULL);
	tm* timeinfo = localtime(&seconds);

	upload_ctx.lines_count = 0;
	upload_ctx.lines_read = 0;
	memset(&upload_ctx, 0, sizeof(upload_ctx));

	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)malloc(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	strftime(upload_ctx.cMailBody[upload_ctx.lines_count], 128, "Date: %Y-%m-%d %H:%M:%S\r\n", timeinfo);
	upload_ctx.BodyLen[upload_ctx.lines_count] = strlen(upload_ctx.cMailBody[upload_ctx.lines_count]);
	upload_ctx.lines_count++;
	
	unsigned int uid = (unsigned int)(upload_ctx.cMailBody[0]) & 0xEFFFFFFF;
	
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)malloc(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "To: <%s>\r\n", mToAddress);
	upload_ctx.BodyLen[upload_ctx.lines_count] = strlen(upload_ctx.cMailBody[upload_ctx.lines_count]);
	upload_ctx.lines_count++;

	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)malloc(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "From: <%s>\r\n", mFromAddr);
	upload_ctx.BodyLen[upload_ctx.lines_count] = strlen(upload_ctx.cMailBody[upload_ctx.lines_count]);
	upload_ctx.lines_count++;
		
	unsigned char *base64;
	unsigned int newLen;
	base64_encode((unsigned char*)mMainText, strlen(mMainText), &base64, &newLen);		
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)malloc(newLen + 50);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, newLen + 50);
	strcat(upload_ctx.cMailBody[upload_ctx.lines_count], "Subject: =?utf-8?B?");
	memcpy(&upload_ctx.cMailBody[upload_ctx.lines_count][strlen(upload_ctx.cMailBody[upload_ctx.lines_count])], base64, newLen);
	strcat(upload_ctx.cMailBody[upload_ctx.lines_count], "?=\r\n");
	//sprintf_s(upload_ctx.cMailBody[upload_ctx.lines_count], newLen + 50, "Subject: =?utf-8?B?%s?=\r\n", base64);
	upload_ctx.BodyLen[upload_ctx.lines_count] = strlen(upload_ctx.cMailBody[upload_ctx.lines_count]);
	upload_ctx.lines_count++;
	free(base64);

	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)malloc(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "MIME-Version: 1.0\r\n");
	upload_ctx.BodyLen[upload_ctx.lines_count] = strlen(upload_ctx.cMailBody[upload_ctx.lines_count]);
	upload_ctx.lines_count++;
	
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)malloc(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "Content-Type: multipart/mixed; boundary=\"%i\"\r\n\r\n", uid);
	upload_ctx.BodyLen[upload_ctx.lines_count] = strlen(upload_ctx.cMailBody[upload_ctx.lines_count]);
	upload_ctx.lines_count++;
	
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)malloc(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "This is a multi-part message in MIME format.\r\n");
	upload_ctx.BodyLen[upload_ctx.lines_count] = strlen(upload_ctx.cMailBody[upload_ctx.lines_count]);
	upload_ctx.lines_count++;
	
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)malloc(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "--%i\r\n", uid);	
	upload_ctx.BodyLen[upload_ctx.lines_count] = strlen(upload_ctx.cMailBody[upload_ctx.lines_count]);
	upload_ctx.lines_count++;
	
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)malloc(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "Content-Type: text/html; charset=utf-8\r\n"); 
	upload_ctx.BodyLen[upload_ctx.lines_count] = strlen(upload_ctx.cMailBody[upload_ctx.lines_count]);
	upload_ctx.lines_count++;
	
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)malloc(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "Content-transfer-encoding:base64\r\n\r\n");
	upload_ctx.BodyLen[upload_ctx.lines_count] = strlen(upload_ctx.cMailBody[upload_ctx.lines_count]);
	upload_ctx.lines_count++;

	//upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)malloc(uiBodyTextLen * 3);
	//memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, uiBodyTextLen * 3);
	WCHAR *WideData;
	unsigned int WideSize;
	AsciiToWide(mBodyText, uiBodyTextLen, &WideData, &WideSize);
	char *StrUtf8;
	unsigned int StrUtf8Len;
	WideToUtf8(WideData, &StrUtf8, &StrUtf8Len);
	//AnsiToKoi((unsigned char*)param_BodyText, uiBodyTextLen);
	//lPrint("Mailer: 4");
	base64_encode((unsigned char*)StrUtf8, StrUtf8Len, &base64, &newLen);
	//base64_encode((unsigned char*)mBodyText, uiBodyTextLen, &base64, &newLen);	
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)malloc(newLen + 10);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, newLen + 10);
	memcpy(upload_ctx.cMailBody[upload_ctx.lines_count], base64, newLen);
	strcat(upload_ctx.cMailBody[upload_ctx.lines_count], "\r\n\r\n");
	upload_ctx.BodyLen[upload_ctx.lines_count] = strlen(upload_ctx.cMailBody[upload_ctx.lines_count]);
	upload_ctx.lines_count++;
	free(base64);
	free(StrUtf8);
	free(WideData);

	/*lPrint("Mailer: 6");
	upload_ctx.cMailBody[upload_ctx.lines_count] = (char*)malloc(128);
	memset(upload_ctx.cMailBody[upload_ctx.lines_count], 0, 128);	
	sprintf(upload_ctx.cMailBody[upload_ctx.lines_count], "--%i\r\n", uid);
	upload_ctx.lines_count++;	*/
	{
		curl_easy_setopt(curl, CURLOPT_USERNAME, mLogin);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, mPassword);

		memset(cBuff, 0, 128);
		sprintf(cBuff, "%s", mServer);
		curl_easy_setopt(curl, CURLOPT_URL, cBuff);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
		
		//curl_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
		//curl_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_NONE);
		
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		
		memset(cBuff, 0, 128);
		sprintf(cBuff, "<%s>", mFromAddr);
		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, cBuff);
		
		memset(cBuff, 0, 128);
		sprintf(cBuff, "<%s>", mToAddress);
		recipients = curl_slist_append(recipients, cBuff);

		
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
		
		//curl_setopt(curl, CURLOPT_INFILESIZE, datalen); 
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
		curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
		
		if ((mAuth) && (strlen(mAuth)))
		{
			memset(cBuff, 0, 128);
			sprintf(cBuff, "AUTH=%s", mAuth);		
			curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, cBuff);
		}	
		
		/* Send the message */
		res = curl_easy_perform(curl);
		
		if(res != CURLE_OK)
		{
			ret = 0;		
			lPrint("SendMail: curl_easy_perform() failed: %s", curl_easy_strerror(res));
		}
		else
		{
			ret = 1;	
			lPrint("SendMail: Sended OK");
		}
		curl_slist_free_all(recipients);
		
		curl_easy_cleanup(curl);
	}

	for (n = 0; n < upload_ctx.lines_count; n++) free(upload_ctx.cMailBody[n]);
		
		
	return ret;
}

void CMfcWebAgentDlg::OnMailingOn() 
{
	//SendNews();
	BlockSection.Lock();
	cScanStatus = 2;
	m_ScanStatus = "Cast";
	IconBmpIdle = ICON_TYPE_WIFI;
	BlockSection.Unlock();
	m_StopBtn.EnableWindow(1);
	m_StartBtn.EnableWindow(1);
	m_MailerBtn.EnableWindow(0);	
	UpdateData(false);
	AnimateIcon(AfxGetInstanceHandle(), m_hWnd, NIM_MODIFY, IconBmpIdle, IconCaption);
}

void CMfcWebAgentDlg::OnSendMail() 
{
	BlockSection.Lock();
	SendNews();
	BlockSection.Unlock();
//	m_wndTaskbarNotifier1.Show("dddd", "gvvvv", "grgrGRg");
}
