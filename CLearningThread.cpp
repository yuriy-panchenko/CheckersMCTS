// CLearningThread.cpp : implementation file
//

#include "pch.h"
#include "Checkers.h"
#include "CLearningThread.h"

using namespace game;
using namespace mcts;
constexpr UINT WM_NEXT_MOVE{ WM_APP + 0x0001 };
constexpr UINT WM_NEW_GAME{ WM_APP + 0x0002 };
#define STALE_COUNT		(3ull)
#ifdef DEBUG
#define SIMULATION_COUNT		(70)
#else
#define SIMULATION_COUNT		(1'000)
#endif // DEBUG
#define NNET_FILENAME	_T("net2.bin")
#define LEARNING_RATE	(.001)

// CLearningThread

IMPLEMENT_DYNCREATE(CLearningThread, CWinThread)

CLearningThread::CLearningThread()
	:m_Tree{ Checkers{ Color::White }, m_Net }
{}

CLearningThread::~CLearningThread()
{}

BOOL CLearningThread::InitInstance()
{
	m_Net.init();
	PostThreadMessage(WM_NEW_GAME, 0, 0);
	return TRUE;
}

int CLearningThread::ExitInstance()
{
	// TODO:  perform any per-thread cleanup here
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CLearningThread, CWinThread)
	ON_THREAD_MESSAGE(WM_NEXT_MOVE, &OnNextMove)
	ON_THREAD_MESSAGE(WM_NEW_GAME, &OnNewGame)
END_MESSAGE_MAP()


// CLearningThread message handlers
void CLearningThread::MakeMove(Move const& m)
{
	if (m.empty())
		m_Tree.current_state().Do(m);
	else
	{
		auto const& root{ m_Tree.get_root() };

		int total_visits{};

		for (auto const& e : root.edges)
			total_visits += e.Visits;

		auto const& best_edge{ *std::max_element(root.edges.begin(), root.edges.end(),
			[](auto const& a, auto const& b) { return a.Visits < b.Visits; }) };

		m_Tree.advance_root(m);
	}
	m_PossibleMoves = GetGame().GetAvailableMoves();

	//	TestEndOfGame
	if (!m_PossibleMoves.empty() && Test4Stale())
		//EndGame(!GetGame().WhoMakesTurn());
		EndGame({});
	else if (m_PossibleMoves.empty())
		EndGame(!GetGame().WhoMakesTurn());
	else PostThreadMessage(WM_NEXT_MOVE, 0, 0);
}

BOOL CLearningThread::Test4Stale()
{
	return ++m_idCount[GetBoard().GetZipID()] >= STALE_COUNT;;
}

Sample CLearningThread::MakeSample() const
{
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

void CLearningThread::EndGame(std::optional<game::Color> winner)
{
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
	PostThreadMessage(WM_NEW_GAME,0,0);
}

void CLearningThread::OnNextMove(WPARAM, LPARAM)
{
	for (size_t i = 0; i < SIMULATION_COUNT; ++i)
		m_Tree.run_simulation();

	m_Tree.add_root_noise();

	m_Samples.push_back(MakeSample());
	MakeMove(m_Tree.select_move());
}

void CLearningThread::OnNewGame(WPARAM, LPARAM)
{
	m_Tree = MCTS{ { Color::Black }, m_Net };
	m_Samples.clear();
	m_idCount.clear();
	MakeMove({});
}

void CLearningThread::TrainOnSamples(std::optional<game::Color> winner)
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

	m_Net.adjust(LEARNING_RATE);

	//policy_loss_sum / m_Samples.size()
}