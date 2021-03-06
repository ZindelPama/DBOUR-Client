#pragma once


#include "Attr_Page.h"
#include "afxwin.h"


// CAttr_COND_SSM 대화 상자입니다.

class CAttr_COND_SSM : public CAttr_Page
{
	DECLARE_SERIAL(CAttr_COND_SSM)

public:
	CAttr_COND_SSM();
	virtual ~CAttr_COND_SSM();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_TS_COND_SSM_ATTR_DIAG };

	virtual CString	GetPageData( void );
	virtual void	UnPakingPageData( CString& strKey, CString& strValue );

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	CComboBox m_ctrOPType;
	CComboBox m_ctrSSMId;
	DWORD m_dwValue;
};
