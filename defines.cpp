#include "pch.h"
#include "defines.h"

namespace game
{
	//void Board::Kill(Position pos)
	//{
	//	ASSERT(fld.has(pos.index()));
	//	fld.kill(pos.index());
	//	//delete field[pos.row][pos.col];
	//	//field[pos.row][pos.col] = nullptr;
	//}

	/*Piece Board::operator[](Position pos)
	{
		ASSERT(pos);
		return field[pos.row][pos.col];
	}

	Piece Board::operator[](Position pos) const
	{
		ASSERT(pos);
		return field[pos.row][pos.col];
	}*/

	Checkers::Checkers(Color first_move)
		:next_move{ first_move }
	{
		brd.init();
	}

	std::vector<Move> Checkers::GetAvailableMoves()const
	{
		std::vector<Move> ret;

		for (auto const& [p, pos] : brd.GetPieces(next_move))
			ret += brd.available_moves(pos);

		return ret;
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

	void Checkers::SetZipID(id::zip64 z)
	{
		brd.SetZipID(z);
	}

	bool Board::operator[](Position pos) const
	{
		return fld.has(pos);
	}

	Piece Board::at(Position pos) const
	{
		return Piece{ fld.is_white(pos) ? Color::White : Color::Black, fld.is_queen(pos) ? Rank::Queen : Rank::Pawn };
	}

	std::vector<Move> Board::available_moves(game::Position const pos)const
	{
		ASSERT((*this)[pos]);
		auto p{ at(pos) };
		std::vector<Move> kills, moves;

		auto test_kill = [this](Jump const& j)->std::vector<Move>
			{
				std::vector<Move> ret;
				if (j.To() && !(*this)[j.To()] && (*this)[j.Kill()])
					if (at(j.Kill()).color != at(j.From()).color)
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

		if (p.rank == Rank::Pawn)
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
				auto src{ test_move({ pos, pos + Position{ Row{p.color == Color::White ? -1 : 1}, Column{-1} } }) };
				moves.insert(moves.end(), std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()));
				src = test_move({ pos, pos + Position{ Row{p.color == Color::White ? -1 : 1}, Column{+1} } });
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
							if (fld.is_white(pos2) == fld.is_white(pos))
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
		fld.move(j.To(), j.From());

		if (j.IsKiller())
			fld.kill(j.Kill());

		if (!fld.is_queen(j.To()))
			if (j.To().row == (fld.is_white(j.To()) ? 0 : 7))
				fld.set_rank(j.To(), true);
	}

	std::vector<Board::location> Board::GetPieces(Color col) const
	{
		std::vector<location> ret;
		ret.reserve(12);
		bool const myWhite{ col == Color::White };

		for (size_t i = 0; i < 64; ++i)
			if (fld.has(i) && fld.is_white(i) == myWhite)
				ret.push_back({ Piece{col,fld.is_queen(i) ? Rank::Queen : Rank::Pawn}, Position{ Row{int(i) / 8}, Column{int(i) % 8} } });

		return ret;
	}

	void Board::init()
	{
		fld = {};
		auto fill_row = [this](Position pos)
			{
				fld.set(pos.index(), pos.row > 4, false);
				pos.col += 2;
				fld.set(pos.index(), pos.row > 4, false);
				pos.col += 2;
				fld.set(pos.index(), pos.row > 4, false);
				pos.col += 2; fld.set(pos.index(), pos.row > 4, false);
			};

		fill_row({ Row{0}, Column{1} });
		fill_row({ Row{1}, Column{0} });
		fill_row({ Row{2}, Column{1} });

		fill_row({ Row{5}, Column{0} });
		fill_row({ Row{6}, Column{1} });
		fill_row({ Row{7}, Column{0} });
	}

	void Board::SetPieces(std::vector<location> const& v)
	{
		for (auto& [p, pos] : v)
			fld.set(pos, p.color == Color::White, p.rank == Rank::Queen);
	}

	void Board::Clear()
	{
		fld.clear();
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
		if (!src.empty())
			if (dst.empty())
				dst = std::move(src);
			else if (is_kills(dst) == is_kills(src))
				dst.insert(dst.end(), src.begin(), src.end());
			else if (is_kills(src))
				dst = src;
	}

	Color operator!(Color col)
	{
		return col == Color::White ? Color::Black : Color::White;
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
			ret += _T(" -> ");
			if (j.IsKiller())
			{
				ret.AppendChar(_T('['));
				ret += CString(j.Kill());
				ret += _T("] -> ");
			}
			ret += CString(j.To());
		}

		return ret;
	}
}