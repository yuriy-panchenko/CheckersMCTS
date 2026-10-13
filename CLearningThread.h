#pragma once
#include <atomic>
#include <mutex>
#include "defines.h"
#include "mcts.h"
#include "GameStats.h"

//	UI -> thread
constexpr UINT WM_NEXT_MOVE{ WM_APP + 0x0001 };		//	AI picks and plays the move for the side to move
constexpr UINT WM_NEW_GAME{ WM_APP + 0x0002 };
constexpr UINT WM_HUMAN_MOVE{ WM_APP + 0x0003 };	//	LPARAM = new game::Move*, the thread deletes it
//	thread -> UI (posted to the notify window; the UI then calls TakeView())
constexpr UINT WM_MOVE_FINISHED{ WM_APP + 0x0004 };

//	Everything the UI is allowed to know about the game. Built by the learning thread, copied out under a mutex.
struct GameView
{
	game::Checkers state{ game::Color::White };
	Moves possible;
	bool over{};
	std::optional<game::Color> winner;
	size_t gameCount{}, moveCount{};
	int quiet{};
	size_t adjusts{}, learns{};

	//	one-shot output for the output window; TakeView() hands it over once and clears it in the thread
	bool newGame{};
	CString summary;
	std::vector<CString> build, debug;
	std::vector<double> chart;
};

// CLearningThread

class CLearningThread : public CWinThread
{
	DECLARE_DYNCREATE(CLearningThread)

protected:
	CLearningThread();           // protected constructor used by dynamic creation
	virtual ~CLearningThread();

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	void SetNotifyWnd(HWND h) { m_hNotify = h; }	//	set before ResumeThread(), or nullptr to stop notifications
	GameView TakeView();							//	any thread

protected:
	afx_msg void OnNextMove(WPARAM, LPARAM);
	afx_msg void OnNewGame(WPARAM, LPARAM);
	afx_msg void OnHumanMove(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

private:
	void MakeMove(game::Move const& m);
	auto& GetGame() { return m_Tree.current_state(); }
	auto& GetGame()const { return m_Tree.current_state(); }
	Sample MakeSample()const;
	void EndGame(std::optional<game::Color> winner, GameStats::Reason why);
	std::pair<double, double> TrainOnSamples(std::optional<game::Color> winner);   // avg policy, avg value loss
	NNet::out_pair F1(std::vector<double> const&);
	void Publish();
	void AddBuild(CString const&);

private:
	//	thread-owned, never touched by the UI
	Moves m_PossibleMoves;
	mcts::MCTS m_Tree;
	NNet m_Net;
	std::vector<Sample> m_Samples;
	GameStats m_Stats;
	size_t m_uGameCount{}, m_uMoveCount{}, m_winWhite{}, m_winBlack{};
	double m_FirstValue{};
	int m_MaxQuiet{};          // per-game diagnostics for games.csv
	size_t m_MaxRepeats{};
	double m_QAbsMax{}, m_QLast{};   // largest |root value| in the game; root value at the final move
	bool m_bOver{};
	std::optional<game::Color> m_Winner;

	//	shared with the UI
	std::mutex m_mtx;
	GameView m_View;			//	guarded by m_mtx
	std::atomic<HWND> m_hNotify{ nullptr };
};
