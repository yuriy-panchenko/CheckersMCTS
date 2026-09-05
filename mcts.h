#pragma once
#include <memory>
#include <functional>
#include <unordered_set>
#include <random>
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
		std::unique_ptr<Node> child;   // null until this edge is first traversed (lazy expansion)
		double Mean()const { return Visits ? BackedUp / Visits : .0; };         // mean value = W / N (0 if N == 0)
	};

	struct Node
	{
		game::Checkers state;
		std::optional<double> terminal_val;
		std::vector<Edge> edges;       // populated once, at expansion time
	};

	class MCTS
	{
	public:
		using vdb = std::vector<double>;
		using out_nnet = std::pair<vdb, double>;
		using callback = std::function<out_nnet(vdb const&)>;
	public:
		MCTS(game::Checkers const& initial_state, callback&& cb, double _c_puct = 1.5);
		MCTS& operator=(MCTS&&);

		void run_simulation();
		game::Move select_move()const;
		void advance_root(game::Move const& move);

		void debug_dump_root(int top_n)const;
		auto& current_state()const { return root->state; }
		Node const& get_root()const { return *root; }

		static std::vector<double> mask_and_softmax(std::vector<double> const& raw_logits, std::unordered_set<size_t> const& legal_indices);
		void add_root_noise(double alpha = .3, double eps = .25);

	private:
		double expand(Node& node, std::vector<game::Move> const& legal_moves);
		Edge& select_edge(Node& node);
		double select_and_expand(Node& node);
		double gamma_sample(double alpha)const;

	private:
		std::unique_ptr<Node> root;
		//chk::net* pNet;
		callback m_clbThink;
		double c_puct;
	};
}