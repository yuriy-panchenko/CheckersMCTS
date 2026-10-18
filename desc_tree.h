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
		double score;

		static std::mt19937 s_Rng;

	public:
		~Node()
		{
			for (auto& e : edges)
				delete e.pNode;
		}


		Node(game::Checkers const& g, Node* parent)
			:state{ g }
			, pPerent{ parent }
			, score{}
		{
			for (auto& m : g.GetAvailableMoves())
				edges.emplace_back(m);

			size_t myVal{}, oppVal{};

			for (auto& p : state.GetBoard().GetPieces(state.WhoMakesTurn()))
				myVal += p.first.rank == game::Rank::Pawn ? 1 : 3;
			for (auto& p : state.GetBoard().GetPieces(!state.WhoMakesTurn()))
				oppVal += p.first.rank == game::Rank::Pawn ? 1 : 3;

			if (oppVal)
				score = double(myVal) / oppVal;
			else
				score = 100.;
		}

		size_t expand(int depth)
		{
			assert(depth > -1);

			size_t ret{ edges.size() };
			if (depth)
			{
				for (auto& e : edges)
					ret += e.pNode->expand(depth - 1);
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
			}
			return ret;
		}

		double GetScore(game::Color col)const
		{
			double maxScore{ score };
			if (col != state.WhoMakesTurn())
				maxScore = 1. / maxScore;

			for (auto& e : edges)
				if (e.pNode)
					maxScore = max(maxScore, e.pNode->GetScore(col));

			return maxScore;
		}

		game::Move Pick()const
		{
			Moves mvs;
			double best{ .0 };

			for (auto& e : edges)
				if (e.pNode)
				{
					auto db = e.pNode->GetScore(state.WhoMakesTurn());
					if (db >= best)
					{
						if (db != best)
						{
							best = db;
							mvs.clear();
						}
						mvs.push_back(e.move);
					}
				}

			if (mvs.empty())
				return {};
			else if (mvs.size() == 1)
				return mvs.front();
			else return mvs[std::uniform_int_distribution<size_t>{ 0, mvs.size() - 1 }(s_Rng)];
		}
	};

	std::mt19937 Node::s_Rng{ static_cast<unsigned int>(time(nullptr)) };

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

			return m_Root->Pick();
		}
	};
}
