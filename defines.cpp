#include "pch.h"
#include "defines.h"

namespace game
{
	Board::Board()
	{
		ZeroMemory(&field, sizeof field);
		auto fill_row = [this](Position pos)
			{
				field[pos.row][pos.col] = new Piece{ pos.row < 4 ? Color::Black : Color::White, Rank::Pawn };
				pos.col += 2;
				field[pos.row][pos.col] = new Piece{ pos.row < 4 ? Color::Black : Color::White, Rank::Pawn };
				pos.col += 2;
				field[pos.row][pos.col] = new Piece{ pos.row < 4 ? Color::Black : Color::White, Rank::Pawn };
				pos.col += 2;
				field[pos.row][pos.col] = new Piece{ pos.row < 4 ? Color::Black : Color::White, Rank::Pawn };
			};

		fill_row({ Row{0}, Column{1} });
		fill_row({ Row{1}, Column{0} });
		fill_row({ Row{2}, Column{1} });

		fill_row({ Row{5}, Column{0} });
		fill_row({ Row{6}, Column{1} });
		fill_row({ Row{7}, Column{0} });
	}

	Board::Board(Board const& oth)
	{
		for (size_t r = 0; r < 8; ++r)
			for (size_t c = 0; c < 8; ++c)
				if (oth.field[r][c])
					field[r][c] = new Piece{ *oth.field[r][c] };
				else field[r][c] = nullptr;
	}

	Board::~Board()
	{
		for (size_t r = 0; r < 8; r++)
			for (size_t c = 0; c < 8; c++)
				delete field[r][c];
	}

	Board& Board::operator=(Board&& oth)
	{
		for (size_t r = 0; r < 8; r++)
			for (size_t c = 0; c < 8; c++)
			{
				delete field[r][c];
				field[r][c] = oth.field[r][c];
				oth.field[r][c] = nullptr;
			}
		return *this;
	}

	void Board::Kill(Position pos)
	{
		ASSERT(field[pos.row][pos.col]);
		delete field[pos.row][pos.col];
		field[pos.row][pos.col] = nullptr;
	}

	Position Board::FindKill(Jump const& j) const
	{
		Position const step_pos{
			Row{j.To().row > j.From().row ? +1 : -1},
			Column{j.To().col > j.From().col ? +1 : -1} };

		for (auto pos{ j.From() + step_pos }; pos != j.To(); pos += step_pos)
			if ((*this)[pos])
				return pos;

		return {};
	}

	Piece*& Board::operator[](Position pos)
	{
		ASSERT(pos);
		return field[pos.row][pos.col];
	}

	Piece const* Board::operator[](Position pos) const
	{
		ASSERT(pos);
		return field[pos.row][pos.col];
	}


	Checkers::Checkers(Color first_move)
		:next_move{ first_move }
	{}

	std::vector<Move> Checkers::GetAvailableMoves()const
	{
		std::vector<Move> kills, mvs;

		for (Row r{ 0 }; r.valid(); ++r)
			for (Column c{ 0 }; c.valid(); ++c)
				if (brd[{r, c}])
					if (brd[{r, c}]->color == next_move)
						for (auto& m : brd.test_piece({ r, c }))
							if (m.front().IsKiller())
								kills.push_back(std::move(m));
							else mvs.push_back(std::move(m));
		if (kills.empty())
			return mvs;
		return kills;
	}

	bool Checkers::MakeJump(Jump const& j)
	{
		ASSERT(brd[j.From()]);	//	have piece
		ASSERT(!brd[j.To()]);	//	empty
		return brd.MakeJump(j);
	}

	Color Checkers::SwitchPlayer()
	{
		return next_move = next_move == Color::White ? Color::Black : Color::White;;
	}

	std::vector<Move> Board::test_piece(game::Position const pos)const
	{
		auto p{ (*this)[pos] };
		ASSERT(p);
		std::vector<Move> kills, moves;

		if (p->color == Color::White)
		{
			if (p->rank == Rank::Pawn)
			{
				auto pawn_diagonal = [this, pos, &kills, &moves](bool bTestMove, Position to1, Position to2)
					{
						if (to2 && !(*this)[to2] && (*this)[to1] && (*this)[to1]->color != (*this)[pos]->color)
						{//ready to kill!
							Jump root{ pos, to2, to1 };
							auto brd{ *this };
							brd.MakeJump(root);
							auto mvs{ brd.test_piece(to2) };
							if (mvs.empty())
								kills.push_back({ root });
							else for (auto& mv : mvs)
								if (mv.front().IsKiller())
								{
									mv.insert(mv.begin(), root);
									kills.push_back(mv);
								}
						}
						else if (bTestMove && to1 && !(*this)[to1])	//	move to left
							moves.push_back({ {pos, to1} });
					};

				//	kill to left
				auto to1{ pos }, to2{ pos };
				--to1.row,
					--to1.col;
				to2.row -= 2,
					to2.col -= 2;
				pawn_diagonal(true, to1, to2);

				//	kill to right
				to1 = to2 = pos;
				--to1.row,
					++to1.col;
				to2.row -= 2,
					to2.col += 2;
				pawn_diagonal(true, to1, to2);

			}
			else		//	queen
			{
			}
		}
		else
		{
			if (p->rank == Rank::Pawn)
			{
			}
			else
			{
			}
		}

		if (kills.empty())
			return moves;
		else return kills;
	}

	bool Board::MakeJump(Jump j)
	{
		auto pos{ FindKill(j) };
		bool doKill{ pos != Position{} };
		if (doKill)
			Kill(pos);
		(*this)[j.To()] = (*this)[j.From()];
		(*this)[j.From()] = nullptr;
		return doKill;
	}

	std::vector<Piece*> Board::GetPieces(Color col) const
	{
		std::vector<Piece*> ret;
		ret.reserve(12);

		for (size_t y = 0; y < 8; ++y)
			for (size_t x = 0; x < 8; ++x)
				if (field[y][x] && field[y][x]->color == col)
					ret.push_back(field[y][x]);

		return ret;
	}

	Position Position::operator+(Position pos) const
	{
		auto ret{ *this };
		ret += pos;
		return ret;
	}

	void Position::operator+=(Position pos)
	{
		row += pos.row;
		col += pos.col;
	}

	bool Position::operator==(Position oth)
	{
		return row == oth.row && col == oth.col;
	}

	bool Position::is_white() const
	{
		if (row % 2)
			return col % 2;
		else
			return !(col % 2);
	}

	Line& Line::operator++()
	{
		++index;
		return *this;
	}
	Line& Line::operator--()
	{
		--index;
		return *this;
	}
	void Line::operator-=(int val)
	{
		index -= val;
	}
	Jump::Jump(Position f, Position t, Position k)
		:from{ f }
		, to{ t }
		, kill{ k }
	{}
}