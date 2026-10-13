#include "pch.h"
#include "mcts.h"
#include <algorithm>

namespace mcts
{
	MCTS::MCTS(game::Checkers const& initial_state, callback&& cb, double _c_puct)
		:root{ std::make_unique<Node>(initial_state) }
		//, pNet{ &_net }
		, m_clbThink{ std::move(cb) }
		, c_puct{ _c_puct }
	{
		m_Seen[initial_state.GetBoard().GetZipID()] = 1;
	}

	MCTS& MCTS::operator=(MCTS&& oth)
	{
		root = std::move(oth.root);
		m_clbThink = std::move(oth.m_clbThink);
		c_puct = oth.c_puct;
		m_Seen = std::move(oth.m_Seen);
		return *this;
	}

	void MCTS::run_simulation()
	{
		select_and_expand(*root);
	}

	game::Move MCTS::select_move() const
	{
		game::Move ret;
		Node const* pNode{ root.get() };

		while (pNode && !pNode->edges.empty())
		{
			auto const mover{ pNode->state.WhoMakesTurn() };
			Edge const* pBest{ &pNode->edges.front() };

			for (auto const& e : pNode->edges)
				if (e.Visits > pBest->Visits)
					pBest = &e;

			ret.push_back(pBest->j);

			if (!pBest->child)
				break;   // never traversed further, chain ends here

			pNode = pBest->child.get();

			if (pNode->state.WhoMakesTurn() != mover)
				break;   // turn passed to opponent, chain complete
		}

		return ret;
	}

	void MCTS::advance_root(game::Move const& move)
	{
		for (auto const& jump : move)
		{
			auto it{ std::find_if(root->edges.begin(), root->edges.end(),
				[&](Edge const& e) {return e.j == jump; }) };

			if (it != root->edges.end() && it->child)
				root = std::move(it->child);   // reuse existing subtree
			else
			{
				auto const quiet{ next_quiet(*root, jump) };   // must be read before the board changes
				root = std::make_unique<Node>(root->state);   // fresh node, no stats to reuse
				root->state.Do(jump);
				root->quiet = quiet;
			}
		}
		++m_Seen[root->state.GetBoard().GetZipID()];
	}

	double MCTS::expand(Node& node, std::vector<game::Move> const& legal_moves)
	{
		//node.expanded = true;

		std::unordered_set<size_t> legal_indices;
		//std::vector<size_t> legal_indices;
		node.edges.reserve(legal_moves.size());

		for (auto const& move : legal_moves)
		{
			size_t const idx{ move.front().to_policy_index(node.state.IsWhiteTurn()) };
			auto res{ legal_indices.insert(idx) };
			if (res.second)
				node.edges.push_back(Edge{ int(idx), move.front(), .0 });
		}

		auto const out{ m_clbThink(node.state.encode_board()) };
		auto const priors{ mask_and_softmax(out.first, legal_indices) };

		for (auto& e : node.edges)
			e.PriorProb = priors[e.action_index];

		return out.second;
	}

	Edge& MCTS::select_edge(Node& node)
	{
		int parent_N{};

		for (auto const& e : node.edges)
			parent_N += e.Visits;

		double const sqrt_parent{ std::sqrt(double((std::max)(parent_N, 1))) };

		Edge* pBest{ nullptr };
		double best_score{ -std::numeric_limits<double>::infinity() };

		for (auto& e : node.edges)
		{
			double const U{ c_puct * e.PriorProb * sqrt_parent / (1 + e.Visits) };
			double const score{ e.Mean() + U };
			if (score > best_score)
			{
				best_score = score;
				pBest = &e;
			}
		}
		return *pBest;
	}

	MCTS::Outcome MCTS::select_and_expand(Node& node)
	{
		if (node.terminal_val.has_value())
			return { *node.terminal_val, false };

		if (node.edges.empty())
		{
			auto const legal_moves{ node.state.GetAvailableMoves() };
			if (legal_moves.empty())
			{
				//node.terminal = true;
				node.terminal_val = -1.0;   // no moves => the mover here loses
				return { *node.terminal_val, false };
			}
			return { expand(node, legal_moves), false };
		}

		Edge& best{ select_edge(node) };

		if (!best.child)
		{
			best.child = std::make_unique<Node>(node.state);
			best.child->state.Do(best.j);
			best.child->quiet = next_quiet(node, best.j);
		}

		auto& child{ *best.child };
		bool const turn_passed{ child.state.WhoMakesTurn() != node.state.WhoMakesTurn() };

		Outcome out;
		if (child.quiet >= no_progress_limit)
			out = { draw_value, true };                     // no-progress limit reached (mid-chain nodes follow a capture, so their counter is 0)
		else if (!turn_passed)
			out = select_and_expand(child);                 // mid-chain jump: same player, nothing to count
		else
		{
			auto const id{ child.state.GetBoard().GetZipID() };
			if (id.is_king_vs_king() /*|| id.is_king_vs_king(2) */|| seen(id) + 1 >= stale_limit)
				out = { draw_value, true };                 // king vs king, or this would be the 3rd occurrence: a draw
			else
			{
				++m_Seen[id];
				out = select_and_expand(child);
				auto const it{ m_Seen.find(id) };
				if (--it->second == 0)
					m_Seen.erase(it);                       // keep the map from growing with the tree
			}
		}

		// Edge value from THIS node's mover's perspective.
		// A draw is equally bad for both players, so it is never flipped; everything else flips when the turn passed.
		double const value{ out.draw ? draw_value : turn_passed ? -out.v : out.v };

		++best.Visits;
		best.BackedUp += value;
		return { value, out.draw };
	}

	double MCTS::gamma_sample(double alpha) const
	{
		static std::mt19937 rng{ std::random_device{}() };
		return std::gamma_distribution<double>(alpha, 1.0)(rng);
	}

	// real-game positions + positions on the current search path
	size_t MCTS::seen(id::zip64 const& id) const
	{
		auto const it{ m_Seen.find(id) };
		return it == m_Seen.end() ? 0 : it->second;
	}

	// A capture or a pawn move is irreversible progress and resets the counter; a quiet king move adds one ply.
	int MCTS::next_quiet(Node const& parent, game::Jump const& j)
	{
		bool const progress{ j.IsKiller() || parent.state.GetBoard().at(j.From()).rank == game::Rank::Pawn };
		return progress ? 0 : parent.quiet + 1;
	}

	std::vector<double> MCTS::mask_and_softmax(std::vector<double> const& raw_logits, std::unordered_set<size_t> const& legal_indices)
	{
		std::vector probs(raw_logits.size(), .0);

		// subtract max (over legal moves only) before exponentiating —
		// avoids overflow if a logit is large, standard softmax stability trick
		auto max_logit{ -std::numeric_limits<double>::infinity() };

		for (auto idx : legal_indices)
			max_logit = (std::max)(max_logit, raw_logits[idx]);


		for (auto idx : legal_indices)
			probs[idx] = std::exp(raw_logits[idx] - max_logit);

		auto sum{ .0 };

		for (auto idx : legal_indices)
			sum += probs[idx];

		for (auto idx : legal_indices)
			probs[idx] /= sum;

		return probs;   // size 896, zero everywhere except legal_indices, sums to 1
	}

	// Add to MCTS (public):
	void MCTS::debug_dump_root(int top_n) const
	{
		std::vector<Edge const*> sorted;
		for (auto const& e : root->edges)
			sorted.push_back(&e);

		std::sort(sorted.begin(), sorted.end(),
			[](Edge const* a, Edge const* b) { return a->Visits > b->Visits; });

		int total_N = 0;
		for (auto const& e : root->edges)
			total_N += e.Visits;

		TRACE(_T("root edges: %zu, total N: %d\n"), root->edges.size(), total_N);

		int shown = 0;
		for (auto const* e : sorted)
		{
			TRACE(_T("  idx=%d  N=%-5d  P=%.4f  Q=%+.4f  from=(%d,%d) to=(%d,%d)\n"),
				e->action_index, e->Visits, e->PriorProb, e->Mean(),
				int(e->j.From().row), int(e->j.From().col),
				int(e->j.To().row), int(e->j.To().col));

			if (++shown >= top_n)
				break;
		}
	}

	void MCTS::add_root_noise(double alpha, double eps)
	{
		auto& edges{ root->edges };
		if (edges.empty()) return;

		// sample from Dirichlet(alpha) via normalized Gammas
		std::vector<double> noise(edges.size());
		double sum{};
		for (auto& n : noise)
		{
			n = gamma_sample(alpha);   // see below
			sum += n;
		}
		for (auto& n : noise) n /= sum;

		for (size_t i = 0; i < edges.size(); ++i)
			edges[i].PriorProb = (1 - eps) * edges[i].PriorProb + eps * noise[i];
	}
}
