// CMatchWnd.cpp : implementation file
//

#include "pch.h"
#include "Checkers.h"
#include "CMatchWnd.h"

namespace
{
	COLORREF const kBg{ RGB(22, 26, 34) }, kCard{ RGB(33, 39, 51) }, kEdge{ RGB(52, 60, 76) },
		kText{ RGB(226, 231, 240) }, kMuted{ RGB(128, 138, 154) }, kTrack{ RGB(50, 58, 74) },
		kWin{ RGB(88, 196, 126) }, kDraw{ RGB(142, 154, 172) }, kLoss{ RGB(232, 96, 96) },
		kAccent{ RGB(226, 184, 74) }, kLight{ RGB(232, 216, 184) }, kDark{ RGB(122, 86, 58) };

	CString Pct(MatchScore const& s)
	{
		CString c;
		if (s.games())
			c.Format(_T("%.1f%%"), s.value() * 100.);
		else
			c = _T("--");
		return c;
	}

	CString Clock(ULONGLONG ms)
	{
		ULONGLONG const sec{ ms / 1000 };
		CString c;
		c.Format(_T("%02llu:%02llu"), sec / 60, sec % 60);
		return c;
	}

	void MakeFont(CFont& f, int dpi, int pt, int weight)
	{
		f.DeleteObject();
		f.CreateFont(-MulDiv(pt, dpi, 72), 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("Segoe UI"));
	}
}

// CMatchWnd

IMPLEMENT_DYNAMIC(CMatchWnd, CWnd)

CMatchWnd::CMatchWnd()
	:m_Result{}
	, m_bCompleted{ FALSE }
{}

CMatchWnd::~CMatchWnd()
{}

void CMatchWnd::Refresh()
{
	if (GetSafeHwnd() && IsWindowVisible())
		Invalidate(FALSE);
}

void CMatchWnd::SetScore(BOOL vsNet, MatchScore const& score)
{
	(vsNet ? m_Result.vsChampion : m_Result.vsRandom) = score;
	Refresh();
}

void CMatchWnd::Reset(size_t id)
{
	m_bCompleted = FALSE;
	m_Result.vsChampion = m_Result.vsRandom = {};
	m_Result.promoted = false;
	m_Result.trainGames = id;
	m_Live = MatchLive{};
	Refresh();
}

void CMatchWnd::SetResult(MatchResult const& result)
{
	m_Result = result;
	m_bCompleted = TRUE;
	m_Live.phase = MatchPhase::Idle;
	Refresh();
}

void CMatchWnd::SetLive(MatchLive const& live)
{
	m_Live = live;
	Refresh();
}


BEGIN_MESSAGE_MAP(CMatchWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CMatchWnd message handlers

int CMatchWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CClientDC dc(this);
	int const dpi{ dc.GetDeviceCaps(LOGPIXELSY) };
	MakeFont(m_fBig, dpi, 20, FW_SEMIBOLD);
	MakeFont(m_fHead, dpi, 10, FW_SEMIBOLD);
	MakeFont(m_fSmall, dpi, 9, FW_NORMAL);
	MakeFont(m_fTitle, dpi, 8, FW_SEMIBOLD);
	return 0;
}

BOOL CMatchWnd::OnEraseBkgnd(CDC*)
{
	return TRUE;		//	OnPaint covers everything
}

void CMatchWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	Invalidate(FALSE);
}

void CMatchWnd::OnPaint()
{
	CPaintDC dc(this);

	CRect rc;
	GetClientRect(rc);
	if (rc.IsRectEmpty())
		return;

	CDC mem;
	mem.CreateCompatibleDC(&dc);
	CBitmap bmp;
	bmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
	auto* const oldBmp{ mem.SelectObject(&bmp) };

	Draw(mem, rc);

	dc.BitBlt(0, 0, rc.Width(), rc.Height(), &mem, 0, 0, SRCCOPY);
	mem.SelectObject(oldBmp);
}

void CMatchWnd::Draw(CDC& dc, CRect const& rc)
{
	dc.FillSolidRect(rc, kBg);
	dc.SetBkMode(TRANSPARENT);

	////////////////////////////////////////////////////////////////////////////////////// drawing helpers
	auto const text = [&](CFont& f, COLORREF c, CString const& s, int x, int y, UINT align = TA_LEFT)
		{
			dc.SelectObject(&f);
			dc.SetTextColor(c);
			dc.SetTextAlign(align | TA_TOP);
			dc.TextOut(x, y, s);
		};
	auto const width = [&](CFont& f, CString const& s)
		{
			dc.SelectObject(&f);
			return dc.GetTextExtent(s).cx;
		};
	auto const fill = [&](CRect const& r, COLORREF c, int radius = 0)
		{
			CBrush br{ c };
			auto* const ob{ dc.SelectObject(&br) };
			auto* const op{ dc.SelectObject(CPen::FromHandle(static_cast<HPEN>(::GetStockObject(NULL_PEN)))) };
			if (radius)
				dc.RoundRect(r.left, r.top, r.right + 1, r.bottom + 1, radius * 2, radius * 2);
			else
				dc.Rectangle(r.left, r.top, r.right + 1, r.bottom + 1);
			dc.SelectObject(ob);
			dc.SelectObject(op);
		};
	auto const outline = [&](CRect const& r, COLORREF c, int w, int radius = 0)
		{
			CPen pen{ PS_SOLID, w, c };
			auto* const op{ dc.SelectObject(&pen) };
			auto* const ob{ dc.SelectObject(CBrush::FromHandle(static_cast<HBRUSH>(::GetStockObject(NULL_BRUSH)))) };
			if (radius)
				dc.RoundRect(r.left, r.top, r.right, r.bottom, radius * 2, radius * 2);
			else
				dc.Rectangle(r.left, r.top, r.right, r.bottom);
			dc.SelectObject(ob);
			dc.SelectObject(op);
		};
	auto const disc = [&](CRect const& r, COLORREF fillColor, COLORREF edgeColor, int w)
		{
			CBrush br{ fillColor };
			CPen pen{ PS_SOLID, w, edgeColor };
			auto* const ob{ dc.SelectObject(&br) };
			auto* const op{ dc.SelectObject(&pen) };
			dc.Ellipse(r);
			dc.SelectObject(ob);
			dc.SelectObject(op);
		};

	////////////////////////////////////////////////////////////////////////////////////// what is going on
	MatchLive const& L{ m_Live };
	bool const started{ m_Result.trainGames != 0 };
	bool const running{ started && !m_bCompleted };
	bool const hadChamp{ m_bCompleted ? m_Result.hadChampion : L.hadChampion };
	size_t const champGame{ m_bCompleted ? m_Result.championGame : L.championGame };
	int const total{ (hadChamp ? MATCH_CHAMP_GAMES : 0) + MATCH_RANDOM_GAMES };
	int const done{ int(L.champGames.size() + L.randGames.size()) };
	auto const& vsChamp{ m_Result.vsChampion };
	auto const& vsRand{ m_Result.vsRandom };

	int const M{ 14 }, G{ 12 };

	////////////////////////////////////////////////////////////////////////////////////// header: status pill, settings, who is testing whom
	{
		CString status;
		COLORREF dot{ kMuted };
		if (running)
		{
			dot = kWin;
			status = L.phase == MatchPhase::VsChampion ? _T("LIVE  \u00B7  vs champion")
				: L.phase == MatchPhase::VsRandom ? _T("LIVE  \u00B7  vs random") : _T("STARTING");
		}
		else if (m_bCompleted)
		{
			dot = kAccent;
			status = _T("FINISHED");
		}
		else
			status = _T("IDLE  \u00B7  waiting for the next checkpoint");

		CRect pill{ M, M, M + width(m_fHead, status) + 42, M + 26 };
		fill(pill, kCard, 13);
		outline(pill, kEdge, 1, 13);
		fill(CRect{ pill.left + 12, pill.top + 8, pill.left + 22, pill.top + 18 }, dot, 5);
		text(m_fHead, kText, status, pill.left + 30, pill.top + 3);

		CString s;
		s.Format(_T("%d + %d games (champion + random)  \u00B7  %u sims  \u00B7  promote at %.0f%%  \u00B7  %u random plies  \u00B7  %u ply cap"),
			MATCH_CHAMP_GAMES, MATCH_RANDOM_GAMES, MATCH_SIMS, MATCH_PROMOTE * 100., MATCH_RANDOM_PLIES, MATCH_MAX_PLIES);
#ifdef _DEBUG
		s.Append(_T("  (Debug)"));
#endif
		text(m_fSmall, kMuted, s, pill.right + 14, M + 5);

		if (started)
		{
			CString champ;
			if (!hadChamp)
				champ = _T("none yet");
			else if (champGame)
				champ.Format(_T("net @ game %zu"), champGame);
			else
				champ = _T("loaded from disk");
			s.Format(_T("Testing net @ game %zu  \u00B7  champion: %s"), m_Result.trainGames, (LPCTSTR)champ);
			text(m_fSmall, kMuted, s, M + 2, M + 32);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////// cards
	int const top{ M + 56 };
	int const ch{ rc.bottom - M - top };
	if (ch < 60)
		return;

	int const side{ max(80, min(280, ((ch - 84) / 8) * 8)) };
	int const boardW{ side + 24 };
	int const boardX{ max(M, rc.right - M - boardW) };
	int avail{ boardX - G - M };
	int resultW{ 340 }, progressW{ 340 };
	if (avail < resultW + G + progressW)
	{
		resultW = max(120, (avail - G) / 2);
		progressW = max(120, avail - G - resultW);
	}
	CRect const rResult{ M, top, M + resultW, top + ch };
	CRect const rProgress{ rResult.right + G, top, rResult.right + G + progressW, top + ch };
	CRect const rBoard{ boardX, top, boardX + boardW, top + ch };

	auto const card = [&](CRect const& r, CString const& title)
		{
			fill(r, kCard, 10);
			outline(r, kEdge, 1, 10);
			text(m_fTitle, kMuted, title, r.left + 14, r.top + 10);
		};

	auto const pointsBar = [&](CRect const& b, MatchScore const& s, int games)
		{
			double const T{ double(games) };
			auto const px{ [&](double pts) { return int(b.Width() * pts / T + .5); } };
			fill(b, kTrack);
			int const x1{ px(s.w) }, x2{ px(s.w + s.d * .5) }, x3{ max(x2, b.Width() - px(s.l + s.d * .5)) };
			if (x1 > 0)
				fill(CRect{ b.left, b.top, b.left + x1 - 1, b.bottom }, kWin);
			if (x2 > x1)
				fill(CRect{ b.left + x1, b.top, b.left + x2 - 1, b.bottom }, kDraw);
			if (x3 < b.Width())
				fill(CRect{ b.left + x3, b.top, b.right - 1, b.bottom }, kLoss);
			int const mx{ b.left + px(MATCH_PROMOTE * T) };
			fill(CRect{ mx - 1, b.top - 3, mx, b.bottom + 3 }, kAccent);
		};

	auto const caption = [&](MatchScore const& s, int games)
		{
			double const T{ double(games) }, need{ MATCH_PROMOTE * T }, pts{ s.w + s.d * .5 }, rem{ T - s.games() };
			CString c;
			if (pts + 1e-9 >= need)
				c = _T("threshold reached");
			else if (pts + rem + 1e-9 < need)
				c = rem > 0 ? _T("out of reach") : _T("below threshold");
			else
				c.Format(_T("needs %.1f more pts"), need - pts);
			return c;
		};

	////////////////////////////////////////////////////////////////////////////////////// RESULT: scores of both opponents, current phase live
	{
		int const save{ dc.SaveDC() };
		dc.IntersectClipRect(rResult);

		CString title{ _T("RESULT") };
		if (started)
			title.AppendFormat(_T("  \u00B7  NET @ GAME %zu"), m_Result.trainGames);
		card(rResult, title);

		auto const row = [&](int y, CString const& label, MatchScore const& s, int games, bool active, bool available)
			{
				int const x{ rResult.left + 14 }, right{ rResult.right - 14 };
				text(m_fHead, kText, label, x, y);
				if (!available)
				{
					text(m_fSmall, kMuted, _T("no champion yet: the first net to beat random becomes one"), x, y + 22);
					return;
				}
				CString cap{ caption(s, games) };
				if (active)
					cap.Insert(0, _T("playing  \u00B7  "));
				text(m_fSmall, active ? kWin : kMuted, cap, right, y + 2, TA_RIGHT);

				CString p{ Pct(s) };
				COLORREF const pc{ !s.games() ? kMuted : s.value() >= MATCH_PROMOTE ? kWin : s.value() < .5 ? kLoss : kText };
				text(m_fBig, pc, p, x, y + 16);
				int tx{ x + width(m_fBig, p) + 14 };
				CString w, d, l, n;
				w.Format(_T("%dW"), s.w);
				d.Format(_T("%dD"), s.d);
				l.Format(_T("%dL"), s.l);
				n.Format(_T("%d / %d games"), s.games(), games);
				text(m_fHead, kWin, w, tx, y + 24);
				tx += width(m_fHead, w) + 10;
				text(m_fHead, kDraw, d, tx, y + 24);
				tx += width(m_fHead, d) + 10;
				text(m_fHead, kLoss, l, tx, y + 24);
				tx += width(m_fHead, l) + 14;
				text(m_fSmall, kMuted, n, tx, y + 27);

				pointsBar(CRect{ x, y + 52, right, y + 60 }, s, games);
			};

		int y{ rResult.top + 34 };
		row(y, _T("vs champion"), vsChamp, MATCH_CHAMP_GAMES, running && L.phase == MatchPhase::VsChampion, hadChamp || !started);
		y += 76;
		row(y, _T("vs random"), vsRand, MATCH_RANDOM_GAMES, running && L.phase == MatchPhase::VsRandom, true);
		y += 76;

		CString foot;
		COLORREF fc{ kMuted };
		if (m_bCompleted)
		{
			foot = m_Result.promoted ? _T("\u2605  PROMOTED: new champion") : _T("not promoted");
			fc = m_Result.promoted ? kAccent : kMuted;
		}
		else
			foot.Format(_T("bar = wins + \u00BD draws; marker = promotion at %.0f%%"), MATCH_PROMOTE * 100.);
		text(m_fSmall, fc, foot, rResult.left + 14, y);

		dc.RestoreDC(save);
	}

	////////////////////////////////////////////////////////////////////////////////////// PROGRESS: games, strips, timing
	{
		int const save{ dc.SaveDC() };
		dc.IntersectClipRect(rProgress);
		card(rProgress, _T("PROGRESS"));

		int const x{ rProgress.left + 14 }, right{ rProgress.right - 14 };
		int y{ rProgress.top + 32 };

		int const current{ started ? min(total, done + (running ? 1 : 0)) : 0 };
		CString s;
		s.Format(_T("Game %d of %d"), current, started ? total : 0);
		text(m_fHead, kText, s, x, y);
		s.Format(_T("%d%%"), started && total ? done * 100 / total : 0);
		text(m_fHead, kAccent, s, right, y, TA_RIGHT);
		y += 24;

		CRect bar{ x, y, right, y + 8 };
		fill(bar, kTrack);
		if (started && total && done)
			fill(CRect{ bar.left, bar.top, bar.left + bar.Width() * done / total, bar.bottom }, kAccent);
		y += 20;

		auto const strip = [&](CString const& label, std::vector<signed char> const& v, int games, int activeIndex, bool available)
			{
				text(m_fSmall, kMuted, label, x, y);
				y += 18;
				if (!available)
					return;
				int const perRow{ (games + 1) / 2 }, gap{ 2 };
				int const cell{ max(4, min(14, (right - x - (perRow - 1) * gap) / perRow)) };
				for (int i = 0; i < games; ++i)
				{
					CRect c{ x + (i % perRow) * (cell + gap), y + (i / perRow) * (cell + gap), 0, 0 };
					c.right = c.left + cell;
					c.bottom = c.top + cell;
					fill(c, i >= int(v.size()) ? kTrack : v[i] > 0 ? kWin : v[i] < 0 ? kLoss : kDraw);
					if (i == activeIndex)
						outline(CRect{ c.left - 1, c.top - 1, c.right + 1, c.bottom + 1 }, kAccent, 1);
				}
				y += 2 * (cell + gap) + 8;
			};

		bool const champActive{ running && L.phase == MatchPhase::VsChampion }, randActive{ running && L.phase == MatchPhase::VsRandom };
		strip(hadChamp ? _T("vs champion") : _T("vs champion  (none yet)"), L.champGames, MATCH_CHAMP_GAMES, champActive ? L.game : -1, hadChamp);
		strip(_T("vs random"), L.randGames, MATCH_RANDOM_GAMES, randActive ? L.game : -1, true);

		CString t;
		if (started)
		{
			t.Format(_T("Elapsed %s"), (LPCTSTR)Clock(L.elapsedMs));
			if (running && done)
				t.AppendFormat(_T("  \u00B7  ETA %s"), (LPCTSTR)Clock(L.elapsedMs * (total - done) / done));
			if (done)
				t.AppendFormat(_T("  \u00B7  avg %zu plies"), L.finishedPlies / done);
		}
		text(m_fSmall, kMuted, t, x, max(y, rProgress.bottom - 26));

		dc.RestoreDC(save);
	}

	////////////////////////////////////////////////////////////////////////////////////// BOARD: the position being played
	{
		int const save{ dc.SaveDC() };
		dc.IntersectClipRect(rBoard);

		CString title;
		if (running && L.phase != MatchPhase::Idle)
			title.Format(_T("GAME %d / %d  \u00B7  PLY %zu"), min(L.game + 1, MatchGames(L.phase == MatchPhase::VsChampion)), MatchGames(L.phase == MatchPhase::VsChampion), L.ply);
		else
			title = m_bCompleted ? _T("LAST POSITION") : _T("BOARD");
		card(rBoard, title);

		int const cell{ side / 8 };
		int const bx{ rBoard.left + 12 }, by{ rBoard.top + 30 };
		for (int r = 0; r < 8; ++r)
			for (int c = 0; c < 8; ++c)
			{
				size_t const idx{ size_t(r * 8 + c) };
				CRect sq{ bx + c * cell, by + r * cell, bx + (c + 1) * cell, by + (r + 1) * cell };
				fill(sq, id::zip64::is_dark_square(idx) ? kDark : kLight);
				if (!L.board.has(idx))
					continue;

				bool const white{ L.board.is_white(idx) };
				int const pad{ max(2, cell * 14 / 100) };
				CRect pc{ sq.left + pad, sq.top + pad, sq.right - pad, sq.bottom - pad };
				disc(pc, white ? RGB(246, 244, 238) : RGB(36, 36, 42), white ? RGB(92, 80, 68) : RGB(200, 200, 210), 1);
				if (L.board.is_queen(idx))
				{
					int const in{ max(2, cell * 22 / 100) };
					CPen ring{ PS_SOLID, 2, kAccent };
					auto* const op{ dc.SelectObject(&ring) };
					auto* const ob{ dc.SelectObject(CBrush::FromHandle(static_cast<HBRUSH>(::GetStockObject(NULL_BRUSH)))) };
					dc.Ellipse(pc.left + in, pc.top + in, pc.right - in, pc.bottom - in);
					dc.SelectObject(ob);
					dc.SelectObject(op);
				}
			}
		outline(CRect{ bx - 1, by - 1, bx + 8 * cell + 1, by + 8 * cell + 1 }, kEdge, 1);

		int const cy{ by + 8 * cell + 8 };
		CString who, last;
		COLORREF wc{ kMuted };
		if (running && L.phase != MatchPhase::Idle)
		{
			bool const moverWhite{ L.challengerToMove ? L.challengerWhite : !L.challengerWhite };
			who.Format(_T("%s (%s) to move"),
				L.challengerToMove ? _T("Challenger") : L.phase == MatchPhase::VsChampion ? _T("Champion") : _T("Random"),
				moverWhite ? _T("White") : _T("Black"));
			wc = L.challengerToMove ? kAccent : kText;
		}
		else if (m_bCompleted)
			who = _T("final position of the last game");
		if (!L.lastMove.IsEmpty())
			last.Format(_T("last move: %s"), (LPCTSTR)L.lastMove);
		text(m_fSmall, wc, who, rBoard.left + 12, cy);
		text(m_fSmall, kMuted, last, rBoard.left + 12, cy + 16);

		dc.RestoreDC(save);
	}
}
