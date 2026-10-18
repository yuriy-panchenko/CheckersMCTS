// CMatchThread.cpp : implementation file
//
//	Background matches for the training run. Nothing here touches the learning thread's net: the job carries its own copy.

#include "pch.h"
#include "CMatchThread.h"
#include "HeuristicTree.h"

using namespace game;
using namespace mcts;

#define CHAMPION_LEGACY_FILENAME	_T("champion.bin")	//	before the game number moved into the file name
#define CHAMPION_PREFIX				L"champion_"			//	champion_<training game>.bin
#define CHAMPION_EXT				L".bin"
#define MATCHES_FILENAME	_T("matches.csv")

namespace
{
	std::filesystem::path ChampionPath(size_t game)
	{
		return std::filesystem::path{ std::wstring{ CHAMPION_PREFIX } + std::to_wstring(game) + CHAMPION_EXT };
	}

	//	the champion_<game>.bin with the highest game number in the working directory; empty path if there is none
	std::filesystem::path FindChampion(size_t& game)
	{
		namespace fs = std::filesystem;
		std::wstring const prefix{ CHAMPION_PREFIX };
		fs::path best;
		game = 0;
		std::error_code ec;
		for (fs::directory_iterator it{ fs::path{ L"." }, ec }, end; !ec && it != end; it.increment(ec))
		{
			fs::path const& p{ it->path() };
			if (p.extension() != CHAMPION_EXT)
				continue;
			std::wstring const stem{ p.stem().wstring() };
			if (stem.size() <= prefix.size() || stem.compare(0, prefix.size(), prefix) != 0)
				continue;
			std::wstring const digits{ stem.substr(prefix.size()) };
			if (digits.size() > 18 || digits.find_first_not_of(L"0123456789") != std::wstring::npos)
				continue;
			size_t const n{ static_cast<size_t>(std::stoull(digits)) };
			if (best.empty() || n > game)
			{
				best = p;
				game = n;
			}
		}
		return best;
	}
}

IMPLEMENT_DYNCREATE(CMatchThread, CWinThread)

CMatchThread::CMatchThread()
{}

CMatchThread::~CMatchThread()
{}

BOOL CMatchThread::InitInstance()
{
	size_t game{};
	auto path{ FindChampion(game) };
	if (path.empty())
		path = CHAMPION_LEGACY_FILENAME;		//	saved before the number moved into the name: stays game 0, "loaded from disk"

	std::ifstream s{ path, std::ios::binary };
	if (s)
		try
		{
			auto n{ std::make_unique<NNet>() };
			s >> *n;		//	adopts the shape stored in the file
			m_pChampion = std::move(n);
			m_uChampionGame = game;
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

	m_Live = MatchLive{};
	m_Live.hadChampion = res->hadChampion;
	m_Live.championGame = m_uChampionGame;
	m_tStart = ::GetTickCount64();
	m_tLastPost = 0;

	if (m_pChampion)
	{
		m_Live.phase = MatchPhase::VsChampion;
		res->vsChampion = PlayMatch(job->net, m_pChampion.get());
	}
	m_Live.phase = MatchPhase::VsRandom;
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
	auto& history{ m_Live.phase == MatchPhase::VsChampion ? m_Live.champGames : m_Live.randGames };
	int const games{ MatchGames(b != nullptr) };
	for (int g = 0; g < games && !m_bStop; ++g)
	{
		Color const aColor{ g % 2 ? Color::Black : Color::White };
		m_Live.game = g;
		m_Live.challengerWhite = aColor == Color::White;
		m_Live.ply = 0;
		PostLive(true);

		int const r{ PlayGame(a, b, aColor) };
		switch (r)
		{
		case  1: ++sc.w; break;
		case  0: ++sc.d; break;
		case -1: ++sc.l; break;
		}
		history.push_back(static_cast<signed char>(r));
		m_Live.finishedPlies += m_Live.ply;
		m_Live.game = g + 1;

		auto ptr{ std::make_unique<MatchScore>(sc) };
		if (auto const h{ m_hNotify.load() })
			if (::PostMessage(h, WM_MATCH_SCORE, reinterpret_cast<WPARAM>(b), reinterpret_cast<LPARAM>(ptr.get())))
				ptr.release();
		PostLive(true);
	}
	return sc;
}

void CMatchThread::PostLive(bool force)
{
	constexpr ULONGLONG min_interval_ms{ 100 };
	ULONGLONG const now{ ::GetTickCount64() };
	if (!force && now - m_tLastPost < min_interval_ms)
		return;
	m_tLastPost = now;
	m_Live.elapsedMs = now - m_tStart;

	if (auto const h{ m_hNotify.load() })
	{
		auto ptr{ std::make_unique<MatchLive>(m_Live) };
		if (::PostMessage(h, WM_MATCH_LIVE, 0, reinterpret_cast<LPARAM>(ptr.get())))
			(void)ptr.release();		//	the receiver deletes it
	}
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

	//	the opponent without a net (b == nullptr) has its own tree, which is not MCTS; it is told about every ply so it follows the real game
	std::unique_ptr<heur::Tree> opp;
	if (!b)
	{
		opp = std::make_unique<heur::Tree>(heur::Limits{ MATCH_HEUR_DEPTH, MATCH_HEUR_NODES }, static_cast<unsigned>(m_Rng()));
		opp->Reset(side[1].tree->current_state());
	}

	auto const result{ [&](std::optional<Color> winner) { return !winner ? 0 : *winner == aColor ? 1 : -1; } };

	for (size_t ply = 0; !m_bStop; ++ply)
	{
		auto& tree{ *side[0].tree };
		auto const& state{ tree.current_state() };
		auto const moves{ state.GetAvailableMoves() };

		//	what the Match window shows
		m_Live.ply = ply;
		m_Live.board = state.GetBoard().GetZipID();
		m_Live.challengerToMove = state.WhoMakesTurn() == aColor;
		PostLive(false);

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
		else if (ply < MATCH_RANDOM_PLIES)	//	random opening for both sides, or two deterministic players replay one game
			m = moves[std::uniform_int_distribution<size_t>{ 0, moves.size() - 1 }(m_Rng)];
		else if (!s.net)
		{
			m = opp->Choose();
			ASSERT(std::find(moves.begin(), moves.end(), m) != moves.end());	//	the heuristic tree is out of step with the game
		}
		else
		{
			//	no root noise and most-visited move: a match measures strength, not exploration
			for (size_t i = 0; i < MATCH_SIMS; ++i)
				s.tree->run_simulation();
			m = s.tree->select_move();
		}

		m_Live.lastMove = game::ToString(m);
		if (opp)
			opp->Advance(m);
		for (auto& t : side)
			t.tree->advance_root(m);
	}
	return 0;		//	stopped
}

void CMatchThread::SaveChampion()const
{
	namespace fs = std::filesystem;
	fs::path const target{ ChampionPath(m_uChampionGame) };
	fs::path tmp{ target };
	tmp.replace_extension(L".tmp");		//	never matches champion_<n>.bin: a half-written file cannot become "the latest champion"

	bool ok{ false };
	{
		std::ofstream s{ tmp, std::ios::binary };
		if (s)
			try
			{
				s << *m_pChampion;
				ok = static_cast<bool>(s);
			}
			catch (const std::exception&) {}
	}		//	closed (flushed) before the rename

	std::error_code ec;
	if (ok)
		fs::rename(tmp, target, ec);		//	replaces an existing file on Windows
	if (!ok || ec)
		fs::remove(tmp, ec);
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
