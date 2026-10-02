#pragma once
#include <memory>
#include <functional>
#include <unordered_set>
#include <random>
#include <map>
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
		int quiet{ 0 };                // plies since the last capture or pawn move (king moves only)
		std::vector<Edge> edges;       // populated once, at expansion time
	};

	class MCTS
	{
	public:
		using vdb = std::vector<double>;
		using out_nnet = std::pair<vdb, double>;
		using callback = std::function<out_nnet(vdb const&)>;
		static constexpr size_t stale_limit{ 3 };       // same board seen this many times => draw
		static constexpr int no_progress_limit{ 40 };   // plies (20 moves per side) with no capture and no pawn move => draw
		// Value of a draw for EACH player (not a signed value: it must not be negated when backing up).
		static constexpr double draw_value{ -.1 };
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
		int root_quiet() const { return root->quiet; }
		size_t root_repeats() const { return seen(root->state.GetBoard().GetZipID()); }

	private:
		// v is from the perspective of the player to move at the node; draw is set when the line ends in a repetition draw
		struct Outcome { double v; bool draw; };
		double expand(Node& node, std::vector<game::Move> const& legal_moves);
		Edge& select_edge(Node& node);
		Outcome select_and_expand(Node& node);
		double gamma_sample(double alpha)const;
		static int next_quiet(Node const& parent, game::Jump const& j);
		size_t seen(id::zip64 const& id) const;

	private:
		std::unique_ptr<Node> root;
		//chk::net* pNet;
		callback m_clbThink;
		double c_puct;

		std::map<id::zip64, size_t> m_Seen;   // real-game positions + positions on the current search path
	};
}