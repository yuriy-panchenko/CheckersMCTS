
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

#define TIMER_ELLAPLE	(100)

using namespace game;

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
	ON_COMMAND(ID_EDIT_UNDO, &CCheckersDoc::OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, &CCheckersDoc::OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_REDO, &CCheckersDoc::OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, &CCheckersDoc::OnUpdateEditRedo)
END_MESSAGE_MAP()

// CCheckersDoc construction/destruction

CCheckersDoc::CCheckersDoc() noexcept
	:m_isWhiteHuman{ FALSE }
	, m_isBlackHuman{ FALSE }
	, m_idTimer{ 0 }
	, m_uGameCount{}
	, m_uMoveCount{}
	, m_winWhite{}
	, m_winBlack{}
{
	/*long long diff{};
	auto const start{ std::chrono::steady_clock::now() };

	for (size_t i = 0; i < 1'000; i++)
	{
		game::Checkers ch;
		auto mvs{ ch.GetAvailableMoves() };
		while (!mvs.empty())
			mvs = ch.Do(mvs[rand() % mvs.size()]);

		auto const end{ std::chrono::steady_clock::now() };
		diff += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	}

	CString str;
	str.Format(_T("Total over 1000 is %.2f ms"), diff / 1'000'000.);
	OutputDebugString(str);*/
	m_Net.init();

	m_Net.think(encode_board());                                    // 1. forward pass
	auto legal = encode_legal_moves(board, rules_engine_output);  // 2. your move-encoding step maps legal Jumps → indices
	auto priors = mask_and_softmax(m_Net.policy_logits(), legal);    // 3. masking + softmax happens here
	double v = m_Net.value();                                       // 4. value head, no masking needed (scalar)
}

game::Board const& CCheckersDoc::GetBoard() const
{
	return m_Game.GetBoard();
}

BOOL CCheckersDoc::IsMoveable(game::Position pos) const
{
	for (auto& m : m_PossibleMoves)
		if (m.front().From() == pos)
			return TRUE;
	return FALSE;
}

BOOL CCheckersDoc::IsPossible2Move2(game::Position root, game::Position testPos) const
{
	for (auto& m : m_PossibleMoves)
		if (m.front().From() == root)
			for (auto& j : m)
				if (j.To() == testPos)
					return TRUE;
	return FALSE;
}

game::Move CCheckersDoc::FindMove(game::Position from, game::Position to) const
{
	for (auto& m : m_PossibleMoves)
		if (m.front().From() == from && m.back().To() == to)
			return m;
	return {};
}

CCheckersDoc::~CCheckersDoc()
{
	if (m_idTimer && ::KillTimer(NULL, m_idTimer))
		m_idTimer = 0;
}

BOOL CCheckersDoc::IsHuman(game::Color col) const
{
	return col == Color::White ? m_isWhiteHuman : m_isBlackHuman;
}

void CCheckersDoc::MakeMove(game::Move const& m)
{
	if (!m.empty())
	{
		++m_uMoveCount;
		auto pMain{ static_cast<CMainFrame*>(theApp.GetMainWnd()) };
		ASSERT(pMain);
		CString str;
		str.Format(_T("%I64u : %s, %I64u poss"), m_uMoveCount, ToString(m), m_PossibleMoves.size());
		pMain->GetOutputWnd().AddBuildString(str);
		m_Undo.push_back({ m_Game.GetBoard().GetZipID(), m });
	}

	m_PossibleMoves = m_Game.Do(m);
	if (!m_PossibleMoves.empty() && Test4Stale())
	{
		//EndGame(!m_Game.WhoMakesTurn());
		EndGame();
		return;
	}

	if (!m_PossibleMoves.empty())
	{
		(m_Game.WhoMakesTurn() == Color::White ? m_wPosibleTotal : m_bPosTotal) *= m_PossibleMoves.size();
		(m_Game.WhoMakesTurn() == Color::White ? m_wNoCh : m_bNoCh) += m_PossibleMoves.size() - 1;
	}

	//	testing moves for ambiguity
	/*for (auto it1{ m_PossibleMoves.begin() }; it1 != m_PossibleMoves.end(); ++it1)
		for (auto it2{ std::next(it1) }; it2 != m_PossibleMoves.end(); ++it2)
		{
			bool const sameStart{ it1->front().From() == it2->front().From() };
			bool const sameEnd{ it1->back().To() == it2->back().To() };
			assert(!(sameStart && sameEnd));
		}*/

	if (m_PossibleMoves.empty())
		EndGame(!m_Game.WhoMakesTurn());
	else if (!IsHuman(m_Game.WhoMakesTurn()))
		m_idTimer = ::SetTimer(NULL, 0, TIMER_ELLAPLE, AutoMoveProc);
}

void CCheckersDoc::AutoMove()
{
	if (m_Redo.empty())
	{
		//MakeMove(m_PossibleMoves.front());
		MakeMove(m_PossibleMoves[rand() % m_PossibleMoves.size()]);
	}
	else
	{
		//m = Convert(m_Redo.top());
		//m_Redo.pop();
	}

	UpdatePicture();
}

void CCheckersDoc::EndGame(Color winner)
{
	UpdatePicture();

	if (winner == Color::White)
		++m_winWhite;
	else ++m_winBlack;

	CString str;
	str.Format(_T("%I64u ..%s.. [%I64u:%I64u], mvs = %d, WhPos=%g, BlPos=%g, wNo:%I64u, bNo:%I64u"),
		m_winWhite + m_winBlack,
		(winner == Color::White ? _T("W") : _T("B")),
		m_winWhite,
		m_winBlack,
		m_Undo.size(),
		m_wPosibleTotal,
		m_bPosTotal,
		m_wNoCh,
		m_bNoCh);
	((CMainFrame*)theApp.GetMainWnd())->GetOutputWnd().AddDebugString(str);

	if (IsHuman(Color::White) || IsHuman(Color::Black))
	{
		str.Format(_T("Game over! %s is a winner."), winner == Color::Black ? _T("WHITE") : _T("BLACK"));
		AfxMessageBox(str);
	}
	OnNewDocument();
}

void CCheckersDoc::EndGame()
{
	UpdatePicture();

	CString str;
	str.Format(_T("%I64u ..%s.. [%I64u:%I64u], mvs = %d, WhPos=%g, BlPos=%g, wNo:%I64u, bNo:%I64u"),
		m_winWhite + m_winBlack,
		_T("X"),
		m_winWhite,
		m_winBlack,
		m_Undo.size(),
		m_wPosibleTotal,
		m_bPosTotal,
		m_wNoCh,
		m_bNoCh);
	((CMainFrame*)theApp.GetMainWnd())->GetOutputWnd().AddDebugString(str);

	//if (IsHuman(Color::White) || IsHuman(Color::Black))
	AfxMessageBox(_T("Game over! Noone is a winner."));
	OnNewDocument();
}

void CCheckersDoc::UpdatePicture()
{
	auto pos{ GetFirstViewPosition() };
	while (pos)
		if (auto pView{ static_cast<CCheckersView*>(GetNextView(pos)) })
			pView->UpdatePicture();
}

BOOL CCheckersDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	if (m_idTimer && ::KillTimer(NULL, m_idTimer))
		m_idTimer = 0;
	static_cast<CMainFrame*>(theApp.GetMainWnd())->GetOutputWnd().ClearBuild();

	++m_uGameCount;
	m_uMoveCount = 0;
	m_Undo = {};
	m_Redo = {};
	m_wPosibleTotal = m_bPosTotal = 1.;
	m_wNoCh = m_bNoCh = 0ULL;
	m_idCount.clear();

	m_Game = { Color::Black };
	/*
	m_Game.GetBoard().Clear();
	m_Game.GetBoard().SetPieces({
		{ {Color::Black,Rank::Queen},{Row{1},Column{0}}} ,
		{ {Color::White,Rank::Queen},{Row{0},Column{5}}} });
	auto id1 = m_Game.GetBoard().GetZipID();
	m_Game.Do(Jump{ {Row{0},Column{1}}, {Row{1},Column{0}} });
	auto id2 = m_Game.GetBoard().GetZipID();
	ASSERT(id1 != id2);
	m_Game.Do(Jump{ {Row{1},Column{0}}, {Row{0},Column{1}} });
	auto id3 = m_Game.GetBoard().GetZipID();
	ASSERT(id1 == id3);*/

	MakeMove({});

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

void CCheckersDoc::AutoMoveProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dw)
{
	if (auto pDoc{ dynamic_cast<CCheckersDoc*>(theApp.GetDocument()) })
	{
		if (::KillTimer(NULL, pDoc->m_idTimer))
			pDoc->m_idTimer = 0;

		ASSERT(!pDoc->m_PossibleMoves.empty());
		//pDoc->MakeMove(pDoc->m_PossibleMoves[rand() % pDoc->m_PossibleMoves.size()]);
		ASSERT(!pDoc->m_PossibleMoves.empty());
		pDoc->AutoMove();
	}
}

// CCheckersDoc commands


void CCheckersDoc::OnUpdateIdsIndicatorWhite(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("White %d"), m_Game.GetBoard().GetPieces(Color::White).size());
	pCmdUI->SetText(str);
	pCmdUI->Enable(m_Game.WhoMakesTurn() == Color::White);
}


void CCheckersDoc::OnUpdateIdsIndicatorPossibleMoves(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("Pos: %d"), m_PossibleMoves.size());
	pCmdUI->SetText(str);
}

void CCheckersDoc::OnUpdateIdsIndicatorBlack(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("Black %d"), m_Game.GetBoard().GetPieces(Color::Black).size());
	pCmdUI->SetText(str);
	pCmdUI->Enable(m_Game.WhoMakesTurn() == Color::Black);
}

void CCheckersDoc::OnUpdateIdsIndicatorGameCount(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("G: %I64u"), m_uGameCount);
	pCmdUI->SetText(str);
}

void CCheckersDoc::OnUpdateIdsIndicatorMoveCount(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("M: %I64u"), m_uMoveCount);
	pCmdUI->SetText(str);
}

void CCheckersDoc::OnStartPause()
{
	if (m_idTimer)
	{
		if (::KillTimer(NULL, m_idTimer))
			m_idTimer = 0;
	}
	else m_idTimer = ::SetTimer(NULL, 0, TIMER_ELLAPLE, AutoMoveProc);
}

void CCheckersDoc::OnUpdateStartPause(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_idTimer != 0);
	pCmdUI->Enable(!(IsHuman(Color::White) && IsHuman(Color::Black)));
}

void CCheckersDoc::OnEditUndo()
{
	auto m{ m_Undo.back() };
	m_Redo.push(m);
	m_Undo.pop_back();

	m_Game.SwitchPlayer();
	m_Game.SetZipID(m.state);
	m_PossibleMoves = m_Game.GetAvailableMoves();

	((CMainFrame*)theApp.GetMainWnd())->GetOutputWnd().RemoveBuildString();

	UpdatePicture();
}

void CCheckersDoc::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!m_Undo.empty() && !m_idTimer);
}

void CCheckersDoc::OnEditRedo()
{
	auto m{ m_Redo.top() };
	m_Redo.pop();
	m_Undo.push_back(m);

	m_Game.SwitchPlayer();
	m_Game.SetZipID(m.state);

	MakeMove(m.m);
	UpdatePicture();
}

void CCheckersDoc::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!m_Redo.empty() && !m_idTimer);
}

BOOL CCheckersDoc::Test4Stale()
{
	return ++m_idCount[m_Game.GetBoard().GetZipID()] >= 3ull;
}

std::vector<double> CCheckersDoc::mask_and_softmax(std::vector<double> const& raw_logits, std::vector<int> const& legal_indices)
{
	std::vector<double> probs(raw_logits.size(), .0);

	// subtract max (over legal moves only) before exponentiating —
	// avoids overflow if a logit is large, standard softmax stability trick
	double max_logit = -std::numeric_limits<double>::infinity();
	for (int idx : legal_indices)
		max_logit = (std::max)(max_logit, raw_logits[idx]);

	double sum{ .0 };
	for (int idx : legal_indices)
	{
		double const e{ std::exp(raw_logits[idx] - max_logit) };
		probs[idx] = e;
		sum += e;
	}

	for (int idx : legal_indices)
		probs[idx] /= sum;

	return probs;   // size 896, zero everywhere except legal_indices, sums to 1
}

std::vector<double> CCheckersDoc::encode_board() const
{
	auto brd{ m_Game.GetBoard().GetZipID() };
	auto const mycol{ m_Game.WhoMakesTurn() };
	bool const im_white{ mycol == Color::White };

	std::vector<double> ret(128, .0);
	auto iter{ ret.begin() };

	auto write_square = [&](size_t i)
		{
			if (brd.has(i))
				if (brd.is_white(i) == im_white)   // my own
					*(iter + (brd.is_queen(i) ? 1 : 0)) = 1.;
				else
					*(iter + (brd.is_queen(i) ? 3 : 2)) = 1.;
			iter += 4;
		};

	if (im_white)
	{
		for (size_t i = 0; i < 64; ++i)
			if (is_dark_square(i))
				write_square(i);
	}
	else
		for (size_t i = 64; i-- > 0; )
			if (is_dark_square(i))
				write_square(i);

	return ret;
}

std::vector<int> CCheckersDoc::encode_legal_moves(game::Checkers const& game, std::optional<game::Position> const& forced)
{
	 auto moves = forced ? game.GetAvailableMoves(*forced) : game.GetAvailableMoves();
    bool const im_white = (game.WhoMakesTurn() == Color::White);
}

bool CCheckersDoc::is_dark_square(size_t index)
{
	return ((index / 8) + (index % 8)) % 2 == 1;   // true = dark/playable square
}