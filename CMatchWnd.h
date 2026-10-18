#pragma once
#include <deque>
#include "CMatchThread.h"

// CMatchWnd
//
//	Live view of the background match: result table (current phase live), per-game strips, progress / ETA and the position being played.

class CMatchWnd : public CWnd
{
	DECLARE_DYNAMIC(CMatchWnd)

public:
	CMatchWnd();
	virtual ~CMatchWnd();

	void SetResult(MatchResult const&);
	void SetScore(BOOL vsNet, MatchScore const&);
	void SetLive(MatchLive const&);
	void Reset(size_t);

protected:
	afx_msg int OnCreate(LPCREATESTRUCT);
	afx_msg BOOL OnEraseBkgnd(CDC*);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT, int, int);
	DECLARE_MESSAGE_MAP()

private:
	void Draw(CDC&, CRect const&);
	void Refresh();

private:
	MatchResult m_Result;
	BOOL m_bCompleted;
	MatchLive m_Live;
	std::deque<MatchResult> m_History;	//	finished matches, newest first (the card shows the running or last one big and the few before it small)
	CFont m_fBig, m_fHead, m_fSmall, m_fTitle;
};
