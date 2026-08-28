#pragma once
#include <memory>
#include "defines.h"

namespace mcts
{
	struct Node;

	//struct SearchState
	//{
	//	game::Checkers game;                    // full snapshot — cheap to copy, it's just bitboards
	//	game::Position forced;   // set if mid-chain; determines GetAvailableMoves() vs GetAvailableMoves(pos)
	//};

	struct Edge
	{
		int    action_index;   // policy vector index (from jump_to_policy_index)
		game::Jump   j;           // the actual Jump to apply if this edge is taken
		double P;              // prior probability, from mask_and_softmax at parent expansion
		int    N = 0;           // visit count
		double W = 0.0;         // total backed-up value
		double Q = 0.0;         // mean value = W / N (0 if N == 0)
		std::unique_ptr<Node> child;   // null until this edge is first traversed (lazy expansion)
	};
	
	struct Node
	{
		game::Checkers state;
		bool expanded = false;
		bool terminal = false;
		double terminal_value = 0.0;   // only meaningful if terminal
		std::vector<Edge> edges;       // populated once, at expansion time
	};

	class MCTS
	{
		std::unique_ptr<Node> root;
	public:
		explicit MCTS(game::Checkers const& initial_state);
		void run_simulation();     // one selection→expansion→backup pass — next step
		game::Move select_move() const;  // pick real move from root edge visit counts — later step
	};
}