// CLearningThread.cpp : implementation file
//
//	Owns the whole game: search tree, network, samples, statistics, end-of-game rules and training.
//	The UI talks to it with thread messages and reads results through TakeView() (mutex-protected copy).

#include "pch.h"
#include "CLearningThread.h"

using namespace game;
using namespace mcts;

#ifdef _DEBUG
#define SIMULATION_COUNT	(100)
#else
#define SIMULATION_COUNT	(1'000)
#endif
#define LEARNING_RATE	(.1)
// value target = lambda*outcome + (1-lambda)*tanh(material/MATERIAL_SCALE); lambda ramps up so material fades out
#define VALUE_MIX_START	(.3)
#define VALUE_MIX_GAMES	(1500.)
#define MATERIAL_SCALE	(4.)
#define NNET_FILENAME	_T("net.bin")
#define STATS_FILENAME	_T("games.csv")

static const NNet::topology g_Topology
{
	{180, 180, 180, 180, 128},	//	body
	{160, 200},					//	policy
	{64, 32},					//	value
};

// CLearningThread
IMPLEMENT_DYNCREATE(CLearningThread, CWinThread)

CLearningThread::CLearningThread()
	:m_Tree{ Checkers{ Color::White }, g_Topology }
{}

CLearningThread::~CLearningThread()
{}

BOOL CLearningThread::InitInstance()
{
	//	rand() state is per thread in the CRT, so it has to be seeded here and not in the document
#ifdef _DEBUG
	::srand(10);
#else
	::srand((unsigned)::time(nullptr));
#endif // DEBUG
	m_Stats.load_csv(STATS_FILENAME);

	std::ifstream s{ NNET_FILENAME, std::ios::binary };
	if (s)
		try { s >> m_Tree.net(); }
	catch (const std::exception& e)
	{
		::MessageBox(NULL, CA2W{ e.what() }, _T("Error loading nnet!"), MB_OK | MB_ICONERROR);
	}

	PostThreadMessage(WM_NEW_GAME, 0, 0);
	return TRUE;
}

int CLearningThread::ExitInstance()
{
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CLearningThread, CWinThread)
	ON_THREAD_MESSAGE(WM_NEXT_MOVE, &OnNextMove)
	ON_THREAD_MESSAGE(WM_NEW_GAME, &OnNewGame)
	ON_THREAD_MESSAGE(WM_HUMAN_MOVE, &OnHumanMove)
END_MESSAGE_MAP()

GameView CLearningThread::TakeView()
{
	std::lock_guard lk{ m_mtx };
	GameView out = m_View;
	m_View.newGame = false;
	m_View.summary.Empty();
	m_View.build.clear();
	m_View.debug.clear();
	m_View.chart.clear();
	return out;
}

void CLearningThread::Publish()
{
	{
		std::lock_guard lk{ m_mtx };
		m_View.state = GetGame();
		m_View.possible = m_PossibleMoves;
		m_View.over = m_bOver;
		m_View.winner = m_Winner;
		m_View.gameCount = m_uGameCount;
		m_View.moveCount = m_uMoveCount;
		m_View.quiet = m_Tree.root_quiet();
		m_View.adjusts = m_Tree.net().get_adjusts();
		m_View.learns = m_Tree.net().get_learns();
	}
	if (auto const h{ m_hNotify.load() })
		::PostMessage(h, WM_MOVE_FINISHED, 0, 0);
}

void CLearningThread::AddBuild(CString const& str)
{
	std::lock_guard lk{ m_mtx };
	m_View.build.push_back(str);
}

// CLearningThread message handlers
void CLearningThread::MakeMove(Move const& m)
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

		CString str;
		str.Format(_T("%I64u. %s | root N=%d | best Prob=%.3f Q=%+.3f"),
			m_uMoveCount, ToString(m), total_visits, best_edge.PriorProb, best_edge.Mean());
		AddBuild(str);

		m_Tree.advance_root(m);
	}
	m_PossibleMoves = GetGame().GetAvailableMoves();

	m_MaxQuiet = (std::max)(m_MaxQuiet, m_Tree.root_quiet());
	m_MaxRepeats = (std::max)(m_MaxRepeats, m_Tree.root_repeats());

	//	TestEndOfGame
	if (!m_PossibleMoves.empty() && GetGame().GetBoard().GetZipID().is_king_vs_king())
		EndGame({}, GameStats::Reason::KvK);
	else if (!m_PossibleMoves.empty() && m_Tree.root_repeats() >= MCTS::stale_limit)
		EndGame({}, GameStats::Reason::Stale);
	else if (!m_PossibleMoves.empty() && m_Tree.root_quiet() >= MCTS::no_progress_limit)
		EndGame({}, GameStats::Reason::NoProgress);
	else if (m_PossibleMoves.empty())
		EndGame(!GetGame().WhoMakesTurn(), GameStats::Reason::NoMoves);
	else
		Publish();
}

Sample CLearningThread::MakeSample() const
{
	// build target_policy from root's edges (separate from each Node's own `sample` used internally at expand-time)
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
		rec.legal_indices.insert((size_t)e.action_index); /* same indices as root->edges' action_index list */

	rec.board = root.state.encode_board();
	return rec;
}

// material balance from the mover's point of view; board = 32 dark squares x { own man, own king, opp man, opp king }
static double material_balance(std::vector<double> const& b)
{
	double own{}, opp{};
	for (size_t i = 0; i + 3 < b.size(); i += 4)
	{
		own += b[i] + 2. * b[i + 1];
		opp += b[i + 2] + 2. * b[i + 3];
	}
	return own - opp;
}

void CLearningThread::EndGame(std::optional<Color> winner, GameStats::Reason why)
{
	if (winner)
		if (*winner == Color::White)
			++m_winWhite;
		else ++m_winBlack;

	double const lambda{ (std::min)(1., VALUE_MIX_START + (1. - VALUE_MIX_START) * double(m_Stats.games()) / VALUE_MIX_GAMES) };
	for (auto& sam : m_Samples)
	{
		sam.outcome = winner ? sam.mover == *winner ? 1. : -1. : MCTS::draw_value;
		sam.real_value = lambda * sam.outcome + (1. - lambda) * std::tanh(material_balance(sam.board) / MATERIAL_SCALE);
	}

	size_t const plies{ m_Samples.size() };
	auto const [ploss, vloss] = TrainOnSamples(winner);

	auto const count{ [&](Color c)
		{
			int men{}, kings{};
			for (auto const& [p, pos] : GetGame().GetBoard().GetPieces(c))
				(p.rank == Rank::Queen ? kings : men)++;
			return std::pair{ men, kings };
		} };
	auto const [menW, kingW] = count(Color::White);
	auto const [menB, kingB] = count(Color::Black);

	GameStats::Record const rec{ m_Stats.games() + 1, winner, why, plies, ploss, vloss,
		m_MaxQuiet, m_MaxRepeats, m_QAbsMax, m_QLast, menW, kingW, menB, kingB };
	m_Stats.add(rec);
	GameStats::append_csv(STATS_FILENAME, rec);

	std::ofstream s{ NNET_FILENAME, std::ios::binary };
	if (s)
		try { s << m_Tree.net(); }
	catch (const std::exception& e)
	{
		::MessageBox(NULL, CA2W{ e.what() }, _T("Error saving nnet!"), MB_OK | MB_ICONERROR);
	}

	m_bOver = true;
	m_Winner = winner;
	{
		std::lock_guard lk{ m_mtx };
		m_View.summary = m_Stats.summary();
	}
	Publish();		//	UI sees game over, shows the result if it wants to, and posts WM_NEW_GAME
}

void CLearningThread::OnNextMove(WPARAM, LPARAM)
{
	ASSERT(!m_PossibleMoves.empty());
	if (m_PossibleMoves.size() == 1)
		MakeMove(m_PossibleMoves.front());
	else
	{
		m_Tree.run_simulation();    // expands a fresh root, so the noise below has edges to act on
		m_Tree.add_root_noise();    // exploration noise must be in place BEFORE the search it is meant to shape
		for (size_t i = 1; i < SIMULATION_COUNT; ++i)
			m_Tree.run_simulation();

		double const q{ m_Tree.root_value() };   // mover's view; the last one logged belongs to the player who made the final move
		m_QAbsMax = (std::max)(m_QAbsMax, std::abs(q));
		m_QLast = q;

		m_Samples.push_back(MakeSample());
		if (m_Samples.size() == 1)
			m_FirstValue = m_Tree.net().value();
		MakeMove(m_Tree.select_move());
	}
}

void CLearningThread::OnHumanMove(WPARAM, LPARAM lp)
{
	std::unique_ptr<Move> m{ reinterpret_cast<Move*>(lp) };
	m_Tree.run_simulation();	// expands a fresh root so advance_root() finds the edge
	MakeMove(*m);
}

void CLearningThread::OnNewGame(WPARAM, LPARAM)
{
	m_Tree = MCTS{ Color::Black , g_Topology };
	m_Samples.clear();

	m_FirstValue = .0;
	m_MaxQuiet = 0;
	m_MaxRepeats = 0;
	m_QAbsMax = .0;
	m_QLast = .0;
	m_bOver = false;
	m_Winner.reset();
	++m_uGameCount;
	m_uMoveCount = 0;

	{
		std::lock_guard lk{ m_mtx };
		m_View.newGame = true;
		m_View.summary = m_Stats.summary();
	}
	MakeMove({});
}

MCTS::loss_sum_result CLearningThread::TrainOnSamples(std::optional<game::Color> winner)
{
	ASSERT(!m_Samples.empty());
	auto res{ m_Tree.learn(std::move(m_Samples), LEARNING_RATE) };

	CString str;
	str.Format(_T("%I64u: %s [ %I64u:%I64u ] %zu moves, avg policy loss=%.4f, avg value loss=%.4f, value=%.4f"),
		m_uGameCount,
		winner ? (*winner == Color::White ? _T("W") : _T("B")) : _T("X"),
		m_winWhite,
		m_winBlack,
		m_Samples.size(),
		res.policy, res.value,
		m_FirstValue
	);
	{
		std::lock_guard lk{ m_mtx };
		m_View.debug.push_back(str);
		m_View.chart.push_back(res.policy);
	}

	return res;
}
