#pragma once
#include "CMatchThread.h"

// CMatchWnd

class CMatchWnd : public CWnd
{
	DECLARE_DYNAMIC(CMatchWnd)

public:
	CMatchWnd();
	virtual ~CMatchWnd();

	void SetResult(MatchResult const&);
	void SetScore(BOOL vsNet, MatchScore const&);
	void Reset(size_t);

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()

private:
	MatchResult m_Result;
	BOOL m_bCompleted;
};


