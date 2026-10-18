#pragma once
#include <memory>
#include <cassert>
#include "defines.h"

namespace heuristic
{
	class Node;

	struct Edge
	{
		game::Move move;
		Node* pNode{ nullptr };
	};

	class Node
	{
		game::Checkers state;
		Node* pPerent;
		std::vector<Edge> edges;

	public:
		Node(game::Checkers const& g, Node* parent)
			:state{ g }
			, pPerent{ parent }
		{
			for (auto& m : g.GetAvailableMoves())
				edges.emplace_back(m);
		}

		bool expand(int depth)
		{
			assert(depth > -1);

			if (depth)
			{
				size_t iExpanded{};
				for (auto& e : edges)
				{
					assert(e.pNode);
					if (e.pNode->expand(depth - 1))
						++iExpanded;
				}
				return iExpanded;
			}
			else
			{
				for (auto& e : edges)
					if (!e.pNode)
					{
						auto s2{ state };
						s2.Do(e.move);
						e.pNode = new Node{ s2, this };
					}
				return !edges.empty();
			}
		}
	};

	class Tree
	{
		std::unique_ptr<Node> m_Root;

	public:
		Tree(game::Checkers const& g)
			: m_Root{ new Node{ g, nullptr } }
		{}

		game::Move explore(int const maxDepth, int const maxMoves, int const maxTimeMs)	//	returns false if no more edges/moves
		{
			for (int iDepth = 0; iDepth < maxDepth; iDepth++)
				if (!m_Root->expand(iDepth))
					break;


			Moves poss;
			//
			return poss.empty() ? game::Move{} : poss.front();
		}
	};
}
