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

	/*Position Board::FindKill(Jump const& j) const
	{
		Position const step_pos{
			Row{j.To().row > j.From().row ? +1 : -1},
			Column{j.To().col > j.From().col ? +1 : -1} };

		for (auto pos{ j.From() + step_pos }; pos != j.To(); pos += step_pos)
			if ((*this)[pos])
				return pos;

		return {};
	}*/

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

		for (auto const& [p, pos] : brd.GetPieces(next_move))
		{
			auto ms{ brd.available_moves(pos) };
			if (!ms.empty())
				if (ms.front().front().IsKiller())
					kills.insert(kills.end(), std::make_move_iterator(ms.begin()), std::make_move_iterator(ms.end()));
				else
					mvs.insert(mvs.end(), std::make_move_iterator(ms.begin()), std::make_move_iterator(ms.end()));
		}

		if (kills.empty())
			return mvs;
		else return kills;
	}

	void Checkers::Do(Jump const& j)
	{
		ASSERT(brd[j.From()]);	//	have piece
		ASSERT(!brd[j.To()]);	//	empty
		//return
		brd.MakeJump(j);
	}

	std::vector<Move> Checkers::Do(Move const& m)
	{
		for (auto& j : m)
			Do(j);
		SwitchPlayer();
		return GetAvailableMoves();
	}

	Color Checkers::SwitchPlayer()
	{
		return next_move = next_move == Color::White ? Color::Black : Color::White;;
	}

	std::vector<Move> Board::available_moves(game::Position const pos)const
	{
		auto p{ (*this)[pos] };
		ASSERT(p);
		std::vector<Move> kills, moves;

		auto test_kill = [this](Jump const& j)->std::vector<Move>
			{
				std::vector<Move> ret;
				if (j.To() && !(*this)[j.To()] && (*this)[j.Kill()] && (*this)[j.Kill()]->color != (*this)[j.From()]->color)
				{
					auto brd{ *this };
					brd.MakeJump(j);
					auto mvs{ brd.available_moves(j.To()) };
					if (mvs.empty())
						ret.push_back({ j });
					else if (mvs.front().front().IsKiller())
						for (auto& mv : mvs)
						{
							mv.insert(mv.begin(), j);
							ret.push_back(std::move(mv));
						}
					else ret.push_back({ j });
				}
				return ret;
			};

		auto test_move = [this](Jump const& j)->std::vector<Move>
			{
				if (j.To() && !(*this)[j.To()])	//	move to left
					return { { j } };
				else return {};
			};

		auto make_pawn_kill_jump = [pos](bool left, bool up)->Jump
			{
				auto const
					y{ up ? -1 : +1 },
					x{ left ? -1 : +1 };
				return Jump{ pos, pos + Position{Row{2 * y}, Column{2 * x}}, pos + Position{Row{y}, Column{x}} };
			};

		if (p->rank == Rank::Pawn)
		{
			auto murder{ test_kill(make_pawn_kill_jump(true,true)) };
			auto src{ test_kill(make_pawn_kill_jump(true, false)) };
			murder.insert(murder.end(), std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()));

			src = test_kill(make_pawn_kill_jump(false, true));
			murder.insert(murder.end(), std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()));
			src = test_kill(make_pawn_kill_jump(false, false));
			murder.insert(murder.end(), std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()));


			if (murder.empty())
			{
				auto src{ test_move({ pos, pos + Position{ Row{p->color == Color::White ? -1 : 1}, Column{-1} } }) };
				moves.insert(moves.end(), std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()));
				src = test_move({ pos, pos + Position{ Row{p->color == Color::White ? -1 : 1}, Column{+1} } });
				moves.insert(moves.end(), std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()));
			}
			else
				kills.insert(kills.end(), std::make_move_iterator(murder.begin()), std::make_move_iterator(murder.end()));
		}
		else
		{
			auto test_queen_diagonal = [this, pos](Position dir)->std::vector<Move>
				{
					assert(pos);
					std::vector<Move> mvs;

					auto pos2{ pos + dir };

					for (; pos2; pos2 += dir)
						if ((*this)[pos2])
							if ((*this)[pos2]->color == (*this)[pos]->color)
								return mvs;
							else break;
						else mvs.push_back({ {pos, pos2} });

					if (pos2)
					{
						std::vector<Move> kills;
						auto kill_pos{ pos2 };

						for (pos2 += dir; pos2; pos2 += dir)
							if ((*this)[pos2])
								break;
							else
							{
								Jump const j{ pos, pos2, kill_pos };
								auto brd{ *this };
								brd.MakeJump(j);
								auto possible{ brd.available_moves(pos2) };

								if (possible.empty() || !is_kills(possible))
									kills.push_back({ j });
								else for (auto& mv : possible)
								{
									mv.insert(mv.begin(), j);
									kills.push_back(std::move(mv));
								}
							}
						mvs += std::move(kills);
					}

					return mvs;
				};

			auto qMoves{ test_queen_diagonal({ Row{-1}, Column{-1} }) };
			qMoves += test_queen_diagonal({ Row{-1}, Column{+1} });
			qMoves += test_queen_diagonal({ Row{+1}, Column{-1} });
			qMoves += test_queen_diagonal({ Row{+1}, Column{+1} });

			auto& dst{ is_kills(qMoves) ? kills : moves };
			dst.insert(dst.end(), std::make_move_iterator(qMoves.begin()), std::make_move_iterator(qMoves.end()));
		}

		return kills.empty() ? moves : kills;
	}

	void Board::MakeJump(Jump j)
	{
		if (j.IsKiller())
			Kill(j.Kill());
		(*this)[j.To()] = (*this)[j.From()];
		(*this)[j.From()] = nullptr;

		if ((*this)[j.To()]->rank == Rank::Pawn)
			if (j.To().row == ((*this)[j.To()]->color == Color::White ? 0 : 7))
				(*this)[j.To()]->rank = Rank::Queen;
	}

	std::vector<Board::location> Board::GetPieces(Color col) const
	{
		std::vector<location> ret;
		ret.reserve(12);

		for (int y = 0; y < 8; ++y)
			for (int x = 0; x < 8; ++x)
				if (field[y][x] && field[y][x]->color == col)
					ret.push_back({ field[y][x], Position{ Row{y}, Column{x} } });

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

	Position::operator CString() const
	{
		wchar_t const ret[] = { col.to_wchar(), row.to_wchar(), L'\0', };
		return ret;
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

	void operator+=(std::vector<Move>& dst, std::vector<Move>&& src)
	{
		if (src.empty())
			return;

		if (dst.empty())
		{
			dst = std::move(src);
			return;
		}

		if (is_kills(dst) == is_kills(src))
			dst.insert(dst.end(), std::make_move_iterator(src.begin()), std::make_move_iterator(src.begin()));
		else if (is_kills(src))
			dst = src;
	}

	bool is_kills(std::vector<Move> const& v)
	{
		if (v.empty())
			return false;
		return v.front().front().IsKiller();
	}

	/*std::vector<Move> operator+(std::vector<Move>&& v1, std::vector<Move>&& v2)
	{
		v1 += std::move(v2);
		return v1;
	}*/

	CString ToString(Move const& m)
	{
		if (m.empty())
			return _T("");

		CString ret{ m.front().From() };

		for (auto& j : m)
		{
			ret += _T("->");
			if (j.IsKiller())
			{
				ret.AppendChar(_T('{'));
				ret += CString(j.Kill());
				ret += _T("}->");
			}
			ret += CString(j.To());
		}

		return ret;
	}
}