#pragma once
#include <atomic>
#include <memory>
#include <random>
#include <vector>
#include "defines.h"
#include "mcts.h"

#define MATCH_CHAMP_GAMES	(100)	//	games against the champion, colours alternate
#define MATCH_HEUR_DEPTH	(4)		//	the no-net opponent (heur::Tree): plies (jumps) it looks ahead
#define MATCH_HEUR_NODES	(20000u)	//	and its budget of positions per move, 0 = depth only
#define MATCH_RANDOM_GAMES	(10)	//	games against a random mover: a sanity check (real nets win them all), so few
									//	a match has no deadline: the thread plays all the time and takes a fresh net whenever it is free
#define MATCH_PROMOTE		(.60)	//	score needed to become / replace the champion
#define MATCH_RANDOM_PLIES	(2u)		//	opening plies played at random, otherwise two deterministic nets replay one game
#define MATCH_MAX_PLIES		(400u)	//	safety net: a game that long is a draw
#ifdef _DEBUG
#define MATCH_SIMS			(50u)
#else
#define MATCH_SIMS			(1000u)	//	simulations per move, deliberately lower than self-play
#endif

//	learning thread -> match thread
constexpr UINT WM_RUN_MATCH{ WM_APP + 0x0010 };			//	LPARAM = new MatchJob*, the thread deletes it
//	match thread -> UI (posted to the notify window)
constexpr UINT WM_MATCH_FINISHED{ WM_APP + 0x0011 };	//	LPARAM = new MatchResult*, the receiver deletes it
constexpr UINT WM_MATCH_SCORE{ WM_APP + 0x0012 };	//	LPARAM = new MatchScore*, the receiver deletes it
constexpr UINT WM_MATCH_STARTED{ WM_APP + 0x0013 };	//	LPARAM = game bunch (id)
constexpr UINT WM_MATCH_LIVE{ WM_APP + 0x0014 };		//	LPARAM = new MatchLive*, the receiver deletes it; snapshot of the running match (board, ply, per-game results, timing)

//	a frozen copy of the net being tested; the match thread is its only user
struct MatchJob
{
	MatchJob(size_t g, NNet const& n) :trainGames{ g }, net{ n } {}
	size_t trainGames;		//	games.csv game count when the copy was taken
	NNet net;
};

//	score of the challenger: wins + half the draws, over all games
struct MatchScore
{
	int w{}, d{}, l{};
	int games()const { return w + d + l; }
	double value()const { return games() ? (w + .5 * d) / games() : .0; }
};

enum class MatchPhase { Idle, VsChampion, VsRandom };

constexpr int MatchGames(bool vsChampion) { return vsChampion ? MATCH_CHAMP_GAMES : MATCH_RANDOM_GAMES; }

//	snapshot of the match in progress: posted after every game and, at most every 100 ms, while a game is being played
struct MatchLive
{
	MatchPhase phase{};
	bool hadChampion{};
	size_t championGame{};			//	0 = loaded from disk
	int game{};						//	0-based index of the game in progress in this phase; the phase's game count when the phase is complete
	bool challengerWhite{};
	bool challengerToMove{};
	size_t ply{};					//	plies played in the current game
	std::vector<signed char> champGames, randGames;	//	+1 win, 0 draw, -1 loss: one per finished game
	size_t finishedPlies{};			//	plies of all finished games of this match
	ULONGLONG elapsedMs{};			//	since the match started
	id::zip64 board;				//	position being played
	CString lastMove;
};

struct MatchResult
{
	size_t trainGames{};
	bool hadChampion{};
	size_t championGame{};		//	training game count at which the champion (the one the challenger faced) was promoted, 0 = loaded from disk
	MatchScore vsChampion;		//	empty if there was no champion yet
	MatchScore vsRandom;		//	the fixed anchor
	bool promoted{};
};

// CMatchThread
//
//	Plays challenger-vs-champion and challenger-vs-random matches in the background so the training thread never waits.
//	It plays all the time: whenever it is idle, the training thread hands it the net as it is at the end of the next training game.
//	Champion rule: with no champion, a challenger that scores >= MATCH_PROMOTE against random becomes the champion;
//	afterwards a challenger that scores >= MATCH_PROMOTE against the champion replaces it.
//	Every champion is kept as champion_<training game>.bin; the highest number is the current one (an old unnumbered champion.bin loads as game 0).

class CMatchThread : public CWinThread
{
	DECLARE_DYNCREATE(CMatchThread)

protected:
	CMatchThread();
	virtual ~CMatchThread();

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	void SetNotifyWnd(HWND h) { m_hNotify = h; }		//	set before ResumeThread(), or nullptr to stop notifications
	void RequestStop() { m_bStop = true; }				//	any thread; the running match ends at its next ply
	bool IsIdle()const { return !m_bBusy; }				//	any thread: no match is running, so Submit() will take a job
	bool Submit(std::unique_ptr<MatchJob> job);			//	any thread; false (job untouched) if a match is still running or the queue is not ready

protected:
	afx_msg void OnRunMatch(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

private:
	//	challenger `a` against `b` (nullptr = random mover); +1 a wins, 0 draw, -1 a loses
	int PlayGame(NNet& a, NNet* b, game::Color aColor);
	MatchScore PlayMatch(NNet& a, NNet* b);
	void SaveChampion()const;
	void AppendCsv(MatchResult const&)const;
	void PostLive(bool force);			//	throttled unless forced

private:
	std::unique_ptr<NNet> m_pChampion;
	size_t m_uChampionGame{};
	std::mt19937 m_Rng{ std::random_device{}() };
	std::atomic<bool> m_bStop{ false }, m_bBusy{ false };
	std::atomic<HWND> m_hNotify{ NULL };

	MatchLive m_Live;					//	match-thread only; copies go to the UI
	ULONGLONG m_tStart{}, m_tLastPost{};
};
