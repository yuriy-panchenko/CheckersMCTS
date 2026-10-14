// CMatchWnd.cpp : implementation file
//

#include "pch.h"
#include "Checkers.h"
#include "CMatchWnd.h"


// CMatchWnd

IMPLEMENT_DYNAMIC(CMatchWnd, CWnd)

CMatchWnd::CMatchWnd()
	:m_Result{}
	, m_Score{}
{

}

CMatchWnd::~CMatchWnd()
{}


BEGIN_MESSAGE_MAP(CMatchWnd, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()



// CMatchWnd message handlers



void CMatchWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect canvas;
	GetClientRect(canvas);
	dc.FillSolidRect(canvas, RGB(200, 200, 200));

	/////////////////////////////////////////////////////////////////////////////////////////
	CString s;
	int x = 50, y = 100, line_height{ 25 }, column_width{ 250 };
	s.Format(_T("MATCH_GAMES = %d"), MATCH_GAMES);
	dc.TextOut(x, y, s);

	y += line_height;
	s.Format(_T("MATCH_PROMOTE = %.2g"), MATCH_PROMOTE);
	dc.TextOut(x, y, s);

	y += line_height;
	s.Format(_T("MATCH_RANDOM_PLIES = %d"), MATCH_RANDOM_PLIES);
	dc.TextOut(x, y, s);

	y += line_height;
	s.Format(_T("MATCH_MAX_PLIES = %d"), MATCH_MAX_PLIES);
	dc.TextOut(x, y, s);

	y += line_height;
	s.Format(_T("MATCH_SIMS = %d"), MATCH_SIMS);
#ifdef _DEBUG
	s.Append(_T(" (Debug)"));
#endif
	dc.TextOut(x, y, s);

	/////////////////////////////////////////////////////////////////////////////////////////
	x += column_width, y = 100;
	dc.TextOut(x, y, _T("SCORE"));

	y += 30;
	s.Format(_T("Win: %d (%.3f)"), m_Score.w, m_Score.value());
	dc.TextOut(x, y, s);

	y += 30;
	s.Format(_T("Draw: %d"), m_Score.d);
	dc.TextOut(x, y, s);

	y += 30;
	s.Format(_T("Loss: %d"), m_Score.l);
	dc.TextOut(x, y, s);

	y += 30;
	s.Format(_T("Total: %d"), m_Score.games());
	dc.TextOut(x, y, s);

	/////////////////////////////////////////////////////////////////////////////////////////

	x += column_width, y = 100;
	dc.TextOut(x, y, _T("RESULT"));

	y += 30;
	s.Format(_T("TrainGames: %d"), m_Result.trainGames);
	dc.TextOut(x, y, s);

	y += 30;
	s.Format(_T("hadChamp: %s"), m_Result.hadChampion ? _T("Yes") : _T("No"));
	dc.TextOut(x, y, s);

	y += 30;
	s.Format(_T("ChampGame: %d"), m_Result.championGame);
	dc.TextOut(x, y, s);

	y += 30;
	s.Format(_T("vsChamp: %d/%d/%d, %.3f"), m_Result.vsChampion.w, m_Result.vsChampion.d, m_Result.vsChampion.l, m_Result.vsChampion.value());
	dc.TextOut(x, y, s);

	y += 30;
	s.Format(_T("vsRand: %d/%d/%d, %.3f"), m_Result.vsRandom.w, m_Result.vsRandom.d, m_Result.vsRandom.l, m_Result.vsRandom.value());
	dc.TextOut(x, y, s);

	y += 30;
	s.Format(_T("Promoted: %s"), m_Result.promoted ? _T("Yes") : _T("No"));
	dc.TextOut(x, y, s);
}
