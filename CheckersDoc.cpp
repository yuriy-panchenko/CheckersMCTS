
// CheckersDoc.cpp : implementation of the CCheckersDoc class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "Checkers.h"
#include "CheckersView.h"
#include "MainFrm.h"
#endif

#include "CheckersDoc.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace game;
using namespace mcts;

// CCheckersDoc

IMPLEMENT_DYNCREATE(CCheckersDoc, CDocument)

BEGIN_MESSAGE_MAP(CCheckersDoc, CDocument)
	ON_UPDATE_COMMAND_UI(IDS_INDICATOR_WHITE, &CCheckersDoc::OnUpdateIdsIndicatorWhite)
	ON_UPDATE_COMMAND_UI(IDS_INDICATOR_POSSIBLE_MOVES, &CCheckersDoc::OnUpdateIdsIndicatorPossibleMoves)
	ON_UPDATE_COMMAND_UI(IDS_INDICATOR_BLACK, &CCheckersDoc::OnUpdateIdsIndicatorBlack)
	ON_UPDATE_COMMAND_UI(IDS_INDICATOR_GAME_COUNT, &CCheckersDoc::OnUpdateIdsIndicatorGameCount)
	ON_UPDATE_COMMAND_UI(IDS_INDICATOR_MOVE_COUNT, &CCheckersDoc::OnUpdateIdsIndicatorMoveCount)
	ON_COMMAND(ID_START_PAUSE, &CCheckersDoc::OnStartPause)
	ON_UPDATE_COMMAND_UI(ID_START_PAUSE, &CCheckersDoc::OnUpdateStartPause)
	ON_COMMAND(ID_WHITE_HUMAN, &CCheckersDoc::OnWhiteHuman)
	ON_UPDATE_COMMAND_UI(ID_WHITE_HUMAN, &CCheckersDoc::OnUpdateWhiteHuman)
	ON_UPDATE_COMMAND_UI(IDS_INDICATOR_ADJUSTS, &CCheckersDoc::OnUpdateIdsIndicatorAdjusted)
	ON_UPDATE_COMMAND_UI(IDS_INDICATOR_LEARNS, &CCheckersDoc::OnUpdateIdsIndicatorLearns)
	ON_COMMAND(ID_SHOCK, &CCheckersDoc::OnShock)
END_MESSAGE_MAP()

// CCheckersDoc construction/destruction
constexpr auto
section_key{ _T("Settings") },
white_human{ _T("WhiteIsHuman") },
black_human{ _T("BlackIsHuman") };

CCheckersDoc::CCheckersDoc() noexcept
	:m_isWhiteHuman{ FALSE }
	, m_isBlackHuman{ FALSE }
	, m_bRunning{ TRUE }
	, m_bBusy{ FALSE }
	, m_bReady{ FALSE }
	, m_pLearnTh{ nullptr }
{
	m_isWhiteHuman = theApp.GetProfileInt(section_key, white_human, TRUE);
	m_isBlackHuman = theApp.GetProfileInt(section_key, black_human, FALSE);
}

game::Board const& CCheckersDoc::GetBoard() const
{
	return m_View.state.GetBoard();
}

BOOL CCheckersDoc::IsMoveable(game::Position pos) const
{
	if (m_View.over || !IsHuman(m_View.state.WhoMakesTurn()))
		return FALSE;
	for (auto& m : m_View.possible)
		if (m.front().From() == pos)
			return TRUE;
	return FALSE;
}

BOOL CCheckersDoc::IsPossible2Move2(game::Position root, game::Position testPos) const
{
	for (auto& m : m_View.possible)
		if (m.front().From() == root)
			for (auto& j : m)
				if (j.To() == testPos)
					return TRUE;
	return FALSE;
}

game::Move CCheckersDoc::FindMove(game::Position from, game::Position to) const
{
	for (auto& m : m_View.possible)
		if (m.front().From() == from && m.back().To() == to)
			return m;
	return {};
}

CCheckersDoc::~CCheckersDoc()
{
	ASSERT(!m_pLearnTh);	//	OnCloseDocument() stops it
}

BOOL CCheckersDoc::IsHuman(game::Color col) const
{
	return col == Color::White ? m_isWhiteHuman : m_isBlackHuman;
}

void CCheckersDoc::HumanMove(game::Move const& m)
{
	if (m_bBusy || !m_bReady || m_View.over || !IsHuman(m_View.state.WhoMakesTurn()))
		return;
	m_bBusy = TRUE;
	m_bRunning = TRUE;		//	the opponent answers, as before
	auto const p{ new game::Move{ m } };
	if (!m_pLearnTh->PostThreadMessage(WM_HUMAN_MOVE, 0, reinterpret_cast<LPARAM>(p)))
	{
		delete p;
		m_bBusy = FALSE;
	}
}

void CCheckersDoc::Kick()
{
	if (!m_pLearnTh || m_bBusy || !m_bReady || !m_bRunning || m_View.over)
		return;
	if (IsHuman(m_View.state.WhoMakesTurn()))
		return;
	m_bBusy = m_pLearnTh->PostThreadMessage(WM_NEXT_MOVE, 0, 0);
}

void CCheckersDoc::OnMoveFinished()
{
	if (!m_pLearnTh)
		return;
	m_bReady = TRUE;
	m_bBusy = FALSE;

	auto v{ m_pLearnTh->TakeView() };	//	one locked copy; everything below runs on the UI thread

	auto& out{ static_cast<CMainFrame*>(theApp.GetMainWnd())->GetOutputWnd() };
	if (v.newGame)
		out.ClearBuild();
	if (!v.summary.IsEmpty())
		out.SetSummary(v.summary);
	for (auto const& s : v.build)
		out.AddBuildString(s);
	for (auto const& s : v.debug)
		out.AddDebugString(s);
	for (auto d : v.chart)
		out.AddChartData(d);
	if (!v.chart.empty())
		out.SaveChartData();

	m_View = std::move(v);
	UpdatePicture(m_View.over);

	if (m_View.over)
	{
		if (IsHuman(game::Color::White) || IsHuman(game::Color::Black))
		{
			CString str;
			if (m_View.winner)
				str.Format(_T("Game over! %s is a winner."), *m_View.winner == game::Color::White ? _T("WHITE") : _T("BLACK"));
			else
				str = _T("Game over! Draw.");
			AfxMessageBox(str);
		}
		if (m_pLearnTh)		//	still alive after the modal box
		{
			m_bBusy = m_pLearnTh->PostThreadMessage(WM_NEW_GAME, 0, 0);
		}
	}
	else
		Kick();
}

void CCheckersDoc::UpdatePicture(BOOL doRedraw)
{
	auto pos{ GetFirstViewPosition() };
	while (pos)
		if (auto pView{ static_cast<CCheckersView*>(GetNextView(pos)) })
		{
			pView->UpdatePicture();
			if (doRedraw)
				pView->RedrawWindow();
		}
}

BOOL CCheckersDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	if (!m_pLearnTh)
	{
		//	first call: start the engine; it posts WM_MOVE_FINISHED to the view once its first game is set up
		auto const pos{ GetFirstViewPosition() };
		auto pPos{ pos };
		auto const pView{ pPos ? GetNextView(pPos) : nullptr };
		m_pLearnTh = static_cast<CLearningThread*>(::AfxBeginThread(RUNTIME_CLASS(CLearningThread), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED));
		if (!m_pLearnTh)
			return FALSE;
		m_pLearnTh->m_bAutoDelete = FALSE;		//	we wait on it and delete it ourselves
		m_pLearnTh->SetNotifyWnd(pView ? pView->GetSafeHwnd() : nullptr);
		m_bBusy = TRUE;
		m_pLearnTh->ResumeThread();
	}
	else if (m_bReady && !m_bBusy)
		m_bBusy = m_pLearnTh->PostThreadMessage(WM_NEW_GAME, 0, 0);

	return TRUE;
}

// CCheckersDoc serialization
void CCheckersDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CCheckersDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CCheckersDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data.
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CCheckersDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl* pChunk = nullptr;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != nullptr)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CCheckersDoc diagnostics
#ifdef _DEBUG
void CCheckersDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CCheckersDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

// CCheckersDoc commands


void CCheckersDoc::OnUpdateIdsIndicatorWhite(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("W %d"), GetBoard().GetPieces(Color::White).size());
	pCmdUI->SetText(str);
	pCmdUI->Enable(m_View.state.WhoMakesTurn() == Color::White);
}


void CCheckersDoc::OnUpdateIdsIndicatorPossibleMoves(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("Av %d"), m_View.possible.size());
	pCmdUI->SetText(str);
}

void CCheckersDoc::OnUpdateIdsIndicatorBlack(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("B %d"), GetBoard().GetPieces(Color::Black).size());
	pCmdUI->SetText(str);
	pCmdUI->Enable(m_View.state.WhoMakesTurn() == Color::Black);
}

void CCheckersDoc::OnUpdateIdsIndicatorGameCount(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("G %I64u"), m_View.gameCount);
	pCmdUI->SetText(str);
}

void CCheckersDoc::OnUpdateIdsIndicatorAdjusted(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("Aj %I64u"), m_View.adjusts);
	pCmdUI->SetText(str);
}

void CCheckersDoc::OnUpdateIdsIndicatorMoveCount(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("%I64u-%d"), m_View.moveCount, m_View.quiet);
	pCmdUI->SetText(str);
}

void CCheckersDoc::OnUpdateIdsIndicatorLearns(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("Lr %I64u"), m_View.learns);
	pCmdUI->SetText(str);
}

void CCheckersDoc::OnStartPause()
{
	m_bRunning = !m_bRunning;
	Kick();
}

void CCheckersDoc::OnUpdateStartPause(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bRunning);
	pCmdUI->Enable(!(IsHuman(Color::White) && IsHuman(Color::Black)));
}

void CCheckersDoc::KillLearner()
{
	m_pLearnTh->SetNotifyWnd(nullptr);		//	no more posts to a view that is going away
	m_pLearnTh->PostThreadMessage(WM_QUIT, 0, 0);
	::WaitForSingleObject(m_pLearnTh->m_hThread, INFINITE);
	delete m_pLearnTh;
	m_pLearnTh = nullptr;
	m_bReady = FALSE;
	m_bBusy = FALSE;
}

void CCheckersDoc::OnWhiteHuman()
{
	m_isWhiteHuman = !m_isWhiteHuman;
	if (!m_isWhiteHuman)
		Kick();
}

void CCheckersDoc::OnUpdateWhiteHuman(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_isWhiteHuman);
}

void CCheckersDoc::OnCloseDocument()
{
	theApp.WriteProfileInt(section_key, white_human, m_isWhiteHuman);
	theApp.WriteProfileInt(section_key, black_human, m_isBlackHuman);

	if (m_pLearnTh)
		KillLearner();

	CDocument::OnCloseDocument();
}

void CCheckersDoc::OnShock()
{
	//m_Net.shock();
}
