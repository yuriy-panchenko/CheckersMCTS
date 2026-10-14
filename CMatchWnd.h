#pragma once
#include "CMatchThread.h"

// CMatchWnd

class CMatchWnd : public CWnd
{
	DECLARE_DYNAMIC(CMatchWnd)

public:
	CMatchWnd();
	virtual ~CMatchWnd();

	MatchResult m_Result;
	MatchScore m_Score;

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
};


