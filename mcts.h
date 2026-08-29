#pragma once
#include <memory>
#include "defines.h"

namespace mcts
{
	struct Node;

	struct Edge
	{
		int    action_index;   // policy vector index (from jump_to_policy_index)
		game::Jump   j;           // the actual Jump to apply if this edge is taken
		double PriorProb;              // prior probability, from mask_and_softmax at parent expansion
		int    Visits = 0;           // visit count
		double BackedUp = 0.0;         // total backed-up value
		double Mean = 0.0;         // mean value = W / N (0 if N == 0)
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
	public:
		MCTS(game::Checkers const& initial_state, chk::net& _net, double _c_puct = 1.5);
		void run_simulation();
		game::Move select_move()const;

		void debug_dump_root(int top_n)const;

	private:
		double expand(Node& node, std::vector<game::Move> const& legal_moves);
		Edge& select_edge(Node& node);
		double select_and_expand(Node& node);
		static std::vector<double> mask_and_softmax(std::vector<double> const& raw_logits, std::vector<size_t> const& legal_indices);

	private:
		std::unique_ptr<Node> root;
		chk::net& net;
		double c_puct;
	};
}