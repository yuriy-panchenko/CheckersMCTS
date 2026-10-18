#pragma once
#include <random>
#include "defines.h"

//	heur::Tree
//
//	The match opponent that has no net: it looks a few plies ahead, scores positions by material and picks at random among the best moves.
//	It is a black box for CMatchThread::PlayGame, which only does this:
//
//		heur::Tree t{ limits, seed };
//		t.Reset(start_position);			//	once per game: the position after the first Do(Move{}), White to move
//		t.Advance(m);						//	after EVERY ply of the game, whoever made it (the opponent's own moves too)
//		m = t.Choose();						//	when the opponent is to move and has at least two legal moves
//
//	What is inside is up to the implementation: how the nodes are stored, depth-first or breadth-first, whether the branch is kept after Advance()
//	or rebuilt. The tree has nothing to do with mcts::MCTS.
//
//	Things the engine does that are easy to trip over:
//	  - A ply is one JUMP, not one turn: Do(Jump) switches the player only when the capture chain ends (see Checkers::Do(Jump) and MCTS::advance_root).
//		So a double capture is two plies of the same side, and "who moves" must be read from WhoMakesTurn() at every node: no blind sign flip.
//	  - GetAvailableMoves() returns the continuation moves when a chain is pending, and an empty vector when the side to move is stuck (a loss).
//	  - Choose() must return one of Root().GetAvailableMoves(); PlayGame ASSERTs it in debug.
//	  - The caller does the end-of-game rules (repeats, no progress, king vs king, ply cap); the tree only has to find a good move.

namespace heur
{
	struct Limits
	{
		int depth;			//	plies ahead, counted in jumps
		size_t nodes;		//	budget of positions per Choose(); 0 = only the depth limit
	};

	class Tree
	{
	public:
		Tree(Limits limits, unsigned seed);

		void Reset(game::Checkers const& root);
		void Advance(game::Move const& m);
		game::Move Choose();

		game::Checkers const& Root()const { return m_Root; }
		size_t Nodes()const { return m_nNodes; }		//	positions looked at by the last Choose(), for the Match window later

		//	material of `who` minus material of the other side: a man is worth `man`, a king `king` (kings are worth far more)
		static int Material(id::zip64 const& board, game::Color who);

	private:
		Limits m_Limits;
		std::mt19937 m_Rng;
		game::Checkers m_Root;		//	the real game, always
		size_t m_nNodes{};
	};
}
