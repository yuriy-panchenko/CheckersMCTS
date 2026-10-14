// CMatchThread.cpp : implementation file
//
//	Background matches for the training run. Nothing here touches the learning thread's net: the job carries its own copy.

#include "pch.h"
#include "CMatchThread.h"

using namespace game;
using namespace mcts;

#define CHAMPION_FILENAME	_T("champion.bin")
#define MATCHES_FILENAME	_T("matches.csv")

IMPLEMENT_DYNCREATE(CMatchThread, CWinThread)

CMatchThread::CMatchThread()
{}

CMatchThread::~CMatchThread()
{}

BOOL CMatchThread::InitInstance()
{
	std::ifstream s{ CHAMPION_FILENAME, std::ios::binary };
	if (s)
		try
		{
			auto n{ std::make_unique<NNet>() };
			s >> *n;		//	adopts the shape stored in the file
			m_pChampion = std::move(n);
		}
		catch (const std::exception&)
		{
			m_pChampion.reset();	//	unreadable (old format?): start without a champion
		}
		return TRUE;
}

int CMatchThread::ExitInstance()
{
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CMatchThread, CWinThread)
	ON_THREAD_MESSAGE(WM_RUN_MATCH, &OnRunMatch)
END_MESSAGE_MAP()

bool CMatchThread::Submit(std::unique_ptr<MatchJob> job)
{
	if (m_bBusy.exchange(true))
		return false;
	if (!PostThreadMessage(WM_RUN_MATCH, 0, reinterpret_cast<LPARAM>(job.get())))
	{
		m_bBusy = false;
		return false;
	}
	job.release();		//	the thread owns it now
	return true;
}

void CMatchThread::OnRunMatch(WPARAM, LPARAM lp)
{
	std::unique_ptr<MatchJob> job{ reinterpret_cast<MatchJob*>(lp) };
	if (auto const h{ m_hNotify.load() })
		::PostMessage(h, WM_MATCH_STARTED, 0, static_cast<LPARAM>(job->trainGames));

	auto res{ std::make_unique<MatchResult>() };
	res->trainGames = job->trainGames;
	res->hadChampion = (bool)m_pChampion;
	res->championGame = m_uChampionGame;

	if (m_pChampion)
		res->vsChampion = PlayMatch(job->net, m_pChampion.get());
	res->vsRandom = PlayMatch(job->net, nullptr);

	if (m_bStop)		//	shutting down: the numbers are incomplete
	{
		m_bBusy = false;
		return;
	}

	res->promoted = (res->hadChampion ? res->vsChampion : res->vsRandom).value() >= MATCH_PROMOTE;
	if (res->promoted)
	{
		m_pChampion = std::make_unique<NNet>(job->net);
		m_uChampionGame = job->trainGames;
		SaveChampion();
	}
	AppendCsv(*res);

	m_bBusy = false;
	if (auto const h{ m_hNotify.load() })
		if (::PostMessage(h, WM_MATCH_FINISHED, 0, reinterpret_cast<LPARAM>(res.get())))
			(void)res.release();		//	the receiver deletes it
}

MatchScore CMatchThread::PlayMatch(NNet& a, NNet* b)
{
	MatchScore sc;
	for (int g = 0; g < MATCH_GAMES && !m_bStop; ++g)
	{
		switch (PlayGame(a, b, g % 2 ? Color::Black : Color::White))
		{
		case  1: ++sc.w; break;
		case  0: ++sc.d; break;
		case -1: ++sc.l; break;
		}

		auto ptr{ std::make_unique<MatchScore>(sc) };
		if (auto const h{ m_hNotify.load() })
			if (::PostMessage(h, WM_MATCH_SCORE, reinterpret_cast<WPARAM>(b), reinterpret_cast<LPARAM>(ptr.get())))
				ptr.release();
	}
	return sc;
}

int CMatchThread::PlayGame(NNet& a, NNet* b, Color aColor)
{
	//	one tree per side, both follow the real game; only the side to move searches. The b tree is just a game-state tracker when b is random.
	struct Side { NNet* net; std::unique_ptr<MCTS> tree; };
	auto const make{ [](NNet* n)
		{
			return std::make_unique<MCTS>(Checkers{ Color::Black },
				[n](MCTS::vdb const& inp) -> MCTS::out_nnet
				{
					return n ? n->think(inp) : MCTS::out_nnet{};
				});
		} };
	Side side[2]{ { &a, make(&a) }, { b, make(b) } };

	for (auto& s : side)		//	same start as the learning thread: Black "to move", Do({}) hands the first move to White
		s.tree->current_state().Do(Move{});

	auto const result{ [&](std::optional<Color> winner) { return !winner ? 0 : *winner == aColor ? 1 : -1; } };

	for (size_t ply = 0; !m_bStop; ++ply)
	{
		auto& tree{ *side[0].tree };
		auto const& state{ tree.current_state() };
		auto const moves{ state.GetAvailableMoves() };

		//	the same end-of-game rules, in the same order, as CLearningThread::MakeMove
		if (moves.empty())
			return result(!state.WhoMakesTurn());
		if (state.GetBoard().GetZipID().is_king_vs_king()
			|| tree.root_repeats() >= MCTS::stale_limit
			|| tree.root_quiet() >= MCTS::no_progress_limit
			|| ply >= MATCH_MAX_PLIES)
			return 0;

		//	read the mover from the state every time: a partial capture chain leaves the same side to move
		Side& s{ state.WhoMakesTurn() == aColor ? side[0] : side[1] };

		Move m;
		if (moves.size() == 1)			//	no choice, no search
			m = moves.front();
		else if (!s.net || ply < MATCH_RANDOM_PLIES)
			m = moves[std::uniform_int_distribution<size_t>{ 0, moves.size() - 1 }(m_Rng)];
		else
		{
			//	no root noise and most-visited move: a match measures strength, not exploration
			for (size_t i = 0; i < MATCH_SIMS; ++i)
				s.tree->run_simulation();
			m = s.tree->select_move();
		}

		for (auto& t : side)
			t.tree->advance_root(m);
	}
	return 0;		//	stopped
}

void CMatchThread::SaveChampion()const
{
	std::ofstream s{ CHAMPION_FILENAME, std::ios::binary };
	if (s)
		try { s << *m_pChampion; }
	catch (const std::exception&) {}
}

void CMatchThread::AppendCsv(MatchResult const& r)const
{
	bool const fresh{ !std::filesystem::exists(MATCHES_FILENAME) };
	std::ofstream s{ MATCHES_FILENAME, std::ios::app };
	if (!s)
		return;
	if (fresh)
		s << "train_game,champion_game,champ_w,champ_d,champ_l,rand_w,rand_d,rand_l,promoted\n";
	s << r.trainGames << ',' << (r.hadChampion ? r.championGame : 0) << ','
		<< r.vsChampion.w << ',' << r.vsChampion.d << ',' << r.vsChampion.l << ','
		<< r.vsRandom.w << ',' << r.vsRandom.d << ',' << r.vsRandom.l << ','
		<< (r.promoted ? 1 : 0) << '\n';
}
