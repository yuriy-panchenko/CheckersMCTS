#include "pch.h"
#include "HeuristicTree.h"

using namespace game;

namespace heur
{
	Tree::Tree(Limits limits, unsigned seed)
		:m_Limits{ limits }
		, m_Rng{ seed }
		, m_Root{ Color::Black }
	{}

	void Tree::Reset(Checkers const& root)
	{
		m_Root = root;
		m_nNodes = 0;
		//	TODO: forget the old tree
	}

	void Tree::Advance(Move const& m)
	{
		//	the game moved on by m: step the real position the way MCTS::advance_root does, one jump at a time
		for (auto const& j : m)
			m_Root.Do(j);
		//	TODO: keep the branch that was played (if it is worth keeping) or rebuild
	}

	Move Tree::Choose()
	{
		//	TODO: the tree. This placeholder plays like the old random opponent, so everything runs until it is replaced.
		auto const moves{ m_Root.GetAvailableMoves() };
		m_nNodes = moves.size();
		return moves[std::uniform_int_distribution<size_t>{ 0, moves.size() - 1 }(m_Rng)];
	}

	int Tree::Material(id::zip64 const& board, Color who)
	{
		constexpr int man{ 100 }, king{ 300 };
		int sum{};
		for (size_t i = 0; i < 64; ++i)
			if (id::zip64::is_dark_square(i) && board.has(i))
			{
				int const v{ board.is_queen(i) ? king : man };
				sum += board.is_white(i) == (who == Color::White) ? v : -v;
			}
		return sum;
	}
}
