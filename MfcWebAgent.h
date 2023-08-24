// MfcWebAgent.h : main header file for the MFCWEBAGENT application
//

#if !defined(AFX_MFCWEBAGENT_H__C03E2191_487B_47EA_B308_0338554A4B18__INCLUDED_)
#define AFX_MFCWEBAGENT_H__C03E2191_487B_47EA_B308_0338554A4B18__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMfcWebAgentApp:
// See MfcWebAgent.cpp for the implementation of this class
//

class CMfcWebAgentApp : public CWinApp
{
public:
	CMfcWebAgentApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMfcWebAgentApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMfcWebAgentApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MFCWEBAGENT_H__C03E2191_487B_47EA_B308_0338554A4B18__INCLUDED_)
