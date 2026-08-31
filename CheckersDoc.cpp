
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
#define SIMULATION_COUNT	(100)
#else
#define SIMULATION_COUNT	(1'000)
#endif

#define TIMER_ELLAPLE	(100)
#define LEARNING_RATE	(.001)
#define STALE_COUNT		(3ull)
#define NNET_FILENAME	_T("net.bin")

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


	ON_COMMAND(ID_START_LEARNING_THREAD, &CCheckersDoc::OnStartLearningThread)
	ON_UPDATE_COMMAND_UI(ID_START_LEARNING_THREAD, &CCheckersDoc::OnUpdateStartLearningThread)
END_MESSAGE_MAP()

// CCheckersDoc construction/destruction
constexpr auto
section_key{ _T("Settings") },
white_human{ _T("WhiteIsHuman") },
black_human{ _T("BlackIsHuman") };

CCheckersDoc::CCheckersDoc() noexcept
	:m_isWhiteHuman{ FALSE }
	, m_isBlackHuman{ FALSE }
	, m_idTimer{ 0 }
	, m_uGameCount{}
	, m_uMoveCount{}
	, m_winWhite{}
	, m_winBlack{}
	, m_Tree{ Checkers{ Color::White }, m_Net }
	, m_pLearnTh{ nullptr }
{
	class ms_timer
	{
		std::chrono::steady_clock::time_point from;
		size_t mss;
	public:
		ms_timer() :from{ std::chrono::steady_clock::now() }, mss{} {}
		size_t stop()
		{
			return mss = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - from).count();
		}
		void out_dbg(size_t sample_count = 1ull)
		{
			CString str;
			str.Format(_T("Total time over %I64u samples is %I64u(ms). Average %.3f(ms)/sample"), sample_count, mss, double(mss) / sample_count);
			OutputDebugString(str);
		}
	};
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

	/*
	m_Net.think(encode_board(m_Game));								// 1. forward pass
	auto legal = encode_legal_moves(m_Game, rules_engine_output);	// 2. your move-encoding step maps legal Jumps → indices
	auto priors = mask_and_softmax(m_Net.policy_logits(), legal);	// 3. masking + softmax happens here
	double v = m_Net.value();											// 4. value head, no masking needed (scalar)
	*/

	/*
	Position pos{ Row{1}, Column{1} };
	auto s{ pos.operator CString() };

	NNet net;
	net.init();
	MCTS search{ m_Game, net };

	auto const N{ 1'000 };
	ms_timer t;

	for (int i = 0; i < N; ++i)
		search.run_simulation();

	t.stop();
	t.out_dbg(N);
	search.debug_dump_root(20);
	*/
#ifdef _DEBUG
	::srand(10);
#else
	::srand((unsigned)::time(nullptr));
#endif // DEBUG

	m_Net.init();

	std::ifstream s{ NNET_FILENAME, std::ios::binary };
	if (s)
		try { s >> m_Net; }
	catch (const std::exception& e)
	{
		::MessageBox(NULL, CA2W{ e.what() }, _T("Error loading nnet!"), MB_OK | MB_ICONERROR);
	}

	m_isWhiteHuman = theApp.GetProfileInt(section_key, white_human, TRUE);
	m_isBlackHuman = theApp.GetProfileInt(section_key, black_human, FALSE);
}

game::Board const& CCheckersDoc::GetBoard() const
{
	return m_Tree.current_state().GetBoard();
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
	if (m.empty())
		m_Tree.current_state().Do(m);
	else
	{
		++m_uMoveCount;

		auto const& root{ m_Tree.get_root() };
		int total_visits{};
		for (auto const& e : root.edges)
			total_visits += e.Visits;

		auto const& best_edge{ *std::max_element(root.edges.begin(), root.edges.end(),
			[](auto const& a, auto const& b) { return a.Visits < b.Visits; }) };

		auto pMain{ static_cast<CMainFrame*>(theApp.GetMainWnd()) };
		ASSERT(pMain);
		CString str;
		str.Format(_T("%I64u. %s | root N=%d | best Prob=%.3f Q=%+.3f"),
			m_uMoveCount, ToString(m), total_visits, best_edge.PriorProb, best_edge.Mean);

		pMain->GetOutputWnd().AddBuildString(str);
		m_Tree.advance_root(m);
	}
	m_PossibleMoves = GetGame().GetAvailableMoves();

	//	TestEndOfGame
	if (!m_PossibleMoves.empty() && Test4Stale())
		//EndGame(!GetGame().WhoMakesTurn());
		EndGame({});
	else if (m_PossibleMoves.empty())
		EndGame(!GetGame().WhoMakesTurn());
	else if (!IsHuman(GetGame().WhoMakesTurn()))
		m_idTimer = ::SetTimer(NULL, 0, TIMER_ELLAPLE, AutoMoveProc);
}

void CCheckersDoc::AutoMove()
{
	//MakeMove(m_PossibleMoves[rand() % m_PossibleMoves.size()]);
	//::CWaitCursor _;
	for (size_t i = 0; i < SIMULATION_COUNT; ++i)
		m_Tree.run_simulation();

	m_Tree.add_root_noise();

	m_Samples.push_back(MakeSample());
	if (m_Samples.size() == 1)
		m_FirstValue = m_Net.value();
	MakeMove(m_Tree.select_move());
	UpdatePicture(FALSE);
}

Sample CCheckersDoc::MakeSample()const
{
	// NOW build target_policy from root's edges (this is separate from
		// each Node's own `sample` used internally at expand-time)
	Sample rec{ GetGame().WhoMakesTurn() };
	auto& root{ m_Tree.get_root() };

	int total_visits{};
	for (auto const& e : root.edges)
		total_visits += e.Visits;

	rec.target_policy.resize(896, .0);
	for (auto const& e : root.edges)
		rec.target_policy[e.action_index] = double(e.Visits) / total_visits;

	rec.legal_indices.reserve(root.edges.size());
	for (auto const& e : root.edges)
		rec.legal_indices.push_back((size_t)e.action_index); /* same indices as root->edges' action_index list */

	rec.board = root.state.encode_board();
	return rec;
}

void CCheckersDoc::EndGame(std::optional<Color> winner)
{
	UpdatePicture(TRUE);

	if (winner)
		if (*winner == Color::White)
			++m_winWhite;
		else ++m_winBlack;

	CString str;
	if (IsHuman(Color::White) || IsHuman(Color::Black))
	{
		str.Format(_T("Game over! %s is a winner."), winner == Color::White ? _T("WHITE") : _T("BLACK"));
		AfxMessageBox(str);
	}

	if (winner)
		for (auto& sam : m_Samples)
			sam.real_value = winner ? sam.mover == *winner ? 1. : -1. : .0;

	TrainOnSamples(winner);
	std::ofstream s{ NNET_FILENAME, std::ios::binary };
	if (s)
		try { s << m_Net; }
	catch (const std::exception& e)
	{
		::MessageBox(NULL, CA2W{ e.what() }, _T("Error saving nnet!"), MB_OK | MB_ICONERROR);
	}
	OnNewDocument();
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

	m_FirstValue = .0;

	if (m_idTimer && ::KillTimer(NULL, m_idTimer))
		m_idTimer = 0;
	static_cast<CMainFrame*>(theApp.GetMainWnd())->GetOutputWnd().ClearBuild();

	++m_uGameCount;
	m_uMoveCount = 0;
	m_idCount.clear();

	m_Tree = MCTS{ { Color::Black }, m_Net };
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
	str.Format(_T("W %d"), GetBoard().GetPieces(Color::White).size());
	pCmdUI->SetText(str);
	pCmdUI->Enable(GetGame().WhoMakesTurn() == Color::White);
}


void CCheckersDoc::OnUpdateIdsIndicatorPossibleMoves(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("Av %d"), m_PossibleMoves.size());
	pCmdUI->SetText(str);
}

void CCheckersDoc::OnUpdateIdsIndicatorBlack(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("B %d"), GetBoard().GetPieces(Color::Black).size());
	pCmdUI->SetText(str);
	pCmdUI->Enable(GetGame().WhoMakesTurn() == Color::Black);
}

void CCheckersDoc::OnUpdateIdsIndicatorGameCount(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("G %I64u"), m_uGameCount);
	pCmdUI->SetText(str);
}

void CCheckersDoc::OnUpdateIdsIndicatorAdjusted(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("Aj %I64u"), m_Net.get_adjusts());
	pCmdUI->SetText(str);
}

void CCheckersDoc::OnUpdateIdsIndicatorMoveCount(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("M %I64u"), m_uMoveCount);
	pCmdUI->SetText(str);
}

void CCheckersDoc::OnUpdateIdsIndicatorLearns(CCmdUI* pCmdUI)
{
	CString str;
	str.Format(_T("Lr %I64u"), m_Net.get_learns());
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

BOOL CCheckersDoc::Test4Stale()
{
	return ++m_idCount[GetBoard().GetZipID()] >= STALE_COUNT;
}

void CCheckersDoc::TrainOnSamples(std::optional<game::Color> winner)
{
	double policy_loss_sum{}, value_loss_sum{};
	ASSERT(!m_Samples.empty());

	for (auto const& s : m_Samples)
	{
		m_Net.think(s.board);
		auto const pihat{ MCTS::mask_and_softmax(m_Net.policy_logits(), s.legal_indices) };

		double policy_loss{};
		for (auto idx : s.legal_indices)
			policy_loss -= s.target_policy[idx] * std::log((std::max)(pihat[idx], 1e-8));

		policy_loss_sum += policy_loss;

		double const y{ m_Net.value() };
		value_loss_sum += (y - s.real_value) * (y - s.real_value);

		std::vector dL_policy(896, .0);

		for (auto idx : s.legal_indices)
			dL_policy[idx] = pihat[idx] - s.target_policy[idx];

		m_Net.learn(dL_policy, s.real_value);
	}

	//m_Net.adjust(LEARNING_RATE, m_Samples.size());
	m_Net.adjust(LEARNING_RATE);

	CString str;
	str.Format(_T("%I64u: %s [ %I64u:%I64u ] %zu moves, avg policy loss=%.4f, avg value loss=%.4f, value=%.4f"),
		m_uGameCount,
		winner ? (*winner == Color::White ? _T("W") : _T("B")) : _T("X"),
		m_winWhite,
		m_winBlack,
		m_Samples.size(),
		policy_loss_sum / m_Samples.size(), value_loss_sum / m_Samples.size(),
		m_FirstValue
	);
	static_cast<CMainFrame*>(theApp.GetMainWnd())->GetOutputWnd().AddDebugString(str);
	static_cast<CMainFrame*>(theApp.GetMainWnd())->GetOutputWnd().AddChartData(policy_loss_sum / m_Samples.size());
	static_cast<CMainFrame*>(theApp.GetMainWnd())->GetOutputWnd().SaveChartData();

	m_Samples.clear();
}

void CCheckersDoc::KillLearner()
{
	m_pLearnTh->PostThreadMessage(WM_QUIT, 0, 0);
	::WaitForSingleObject(*m_pLearnTh, INFINITE);
	m_pLearnTh = nullptr;
}

void CCheckersDoc::OnWhiteHuman()
{
	m_isWhiteHuman = !m_isWhiteHuman;
	if (!m_isWhiteHuman)
		AutoMove();
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

void CCheckersDoc::OnStartLearningThread()
{
	if (m_pLearnTh)
		KillLearner();
	else
	{
		m_pLearnTh = static_cast<CLearningThread*>(::AfxBeginThread(RUNTIME_CLASS(CLearningThread)));
	}
}

void CCheckersDoc::OnUpdateStartLearningThread(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_pLearnTh != nullptr);
	pCmdUI->Enable(IsHuman(Color::White) || IsHuman(Color::Black));
}
