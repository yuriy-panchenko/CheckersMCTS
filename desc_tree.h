#pragma once
#include <memory>
#include <cassert>
#include "defines.h"

namespace heuristic
{
	struct Node;

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
		Node* expand(int depth)
		{
			assert(depth > 0);

			for (auto& e : edges)
				if (!e.pNode)
				{
					auto s2{ state };
					s2.Do(e.move);
					e.pNode = new Node{ s2, this };
					return e.pNode;
				}

			if(depth>1)
				for (auto& e : edges)
				{
					//if(!e.pNode->expand(depth-1))
				}

			return nullptr;
		}
	};

	class Tree
	{
		std::unique_ptr<Node> m_Root;

	public:
		Tree(game::Checkers const& g)
			: m_Root{ new Node{ g, nullptr } }
		{}

		bool explore_next()	//	returns false if no more edges to explorer left
		{
			return m_Root->expand();
		}

		game::Move get_best()const;


	};
}
