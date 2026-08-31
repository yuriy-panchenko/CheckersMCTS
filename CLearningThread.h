#pragma once
#include "defines.h"
#include "mcts.h"


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

protected:
	afx_msg void OnNextMove(WPARAM,LPARAM);
	afx_msg void OnNewGame(WPARAM,LPARAM);
	DECLARE_MESSAGE_MAP()

private:
	void MakeMove(game::Move const& m);
	auto& GetGame() { return m_Tree.current_state(); }
	auto& GetGame()const { return m_Tree.current_state(); }
	auto& GetBoard() { return GetGame().GetBoard(); }
	BOOL Test4Stale();
	Sample MakeSample()const;
	void EndGame(std::optional<game::Color> winner);
	void TrainOnSamples(std::optional<game::Color> winner);

private:
	Moves m_PossibleMoves;
	std::map<id::zip64, size_t> m_idCount;
	UINT_PTR m_idTimer;
	mcts::MCTS m_Tree;
	NNet m_Net;
	std::vector<Sample> m_Samples;
	//double m_FirstValue;
};


