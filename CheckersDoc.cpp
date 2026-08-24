
// CheckersDoc.cpp : implementation of the CCheckersDoc class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "Checkers.h"
#endif

#include "CheckersDoc.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace game;

// CCheckersDoc

IMPLEMENT_DYNCREATE(CCheckersDoc, CDocument)

BEGIN_MESSAGE_MAP(CCheckersDoc, CDocument)
	ON_UPDATE_COMMAND_UI(IDS_INDICATOR_WHITE, &CCheckersDoc::OnUpdateIdsIndicatorWhite)
	ON_UPDATE_COMMAND_UI(IDS_INDICATOR_POSSIBLE_MOVES, &CCheckersDoc::OnUpdateIdsIndicatorPossibleMoves)
	ON_UPDATE_COMMAND_UI(IDS_INDICATOR_BLACK, &CCheckersDoc::OnUpdateIdsIndicatorBlack)
END_MESSAGE_MAP()


// CCheckersDoc construction/destruction

CCheckersDoc::CCheckersDoc() noexcept
	:m_isWhiteHuman{ TRUE }
	, m_isBlackHuman{ FALSE }
	, m_idTimer{ 0 }
{}

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
{}

BOOL CCheckersDoc::IsHuman(game::Color col) const
{
	return col == Color::White ? m_isWhiteHuman : m_isBlackHuman;
}

void CCheckersDoc::MakeMove(game::Move const& m)
{
	for (auto& j : m)
		m_Game.MakeJump(j);

	auto cur{ m_Game.SwitchPlayer() };
	m_PossibleMoves = m_Game.GetAvailableMoves();

	//	testing moves for ambiguity
	for (auto it1{ m_PossibleMoves.begin() }; it1 != m_PossibleMoves.end(); ++it1)
		for (auto it2{ std::next(it1) }; it2 != m_PossibleMoves.end(); ++it2)
		{
			bool const sameStart{ it1->front().From() == it2->front().From() };
			bool const sameEnd{ it1->back().To() == it2->back().To() };
			assert(!(sameStart && sameEnd));
		}

	if (m_PossibleMoves.empty())
		EndGame();
	else if (!IsHuman(cur))
		m_idTimer = ::SetTimer(NULL, 0, 1'500, AutoMoveProc);
}

void CCheckersDoc::EndGame()
{
	//	...................
}

BOOL CCheckersDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	m_Game = { Color::Black };
	//m_PossibleMoves = m_Game.GetAvailableMoves();
	MakeMove({});

	//	if (!IsHuman(m_Game.WhoMakesTurn()))
	//		m_idTimer = ::SetTimer(NULL, 0, 1'500, AutoMoveProc);

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
		pDoc->MakeMove(pDoc->m_PossibleMoves[rand() % pDoc->m_PossibleMoves.size()]);
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
