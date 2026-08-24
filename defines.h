#pragma once
#include <vector>

namespace game
{
	enum class Color :bool { White, Black, };

	enum class Rank :bool { Pawn, Queen, };

	struct Piece
	{
		Color color;
		Rank rank;
	};

	class Line abstract
	{
	protected:
		char index;
	public:
		Line(int i) :index{ char(i) } {}
		operator int()const noexcept { return index; }
		bool valid()const { return index >= 0 && index <= 7; }
		void operator+=(int i) { index += char(i); }
		Line& operator++();
		Line& operator--();
		void operator-=(int);
	};

	//	8-0, 7-1, .., 1-7
	class Row
		:public Line
	{
	public:
		Row() = default;
		explicit Row(char);
		explicit Row(int i)
			:Line{ i } {}

		bool top_half()const { return index < 4; }
	};

	//	a-0, b-1, .., h-7
	class Column
		:public Line
	{
	public:
		Column() = default;
		explicit Column(char);
		explicit Column(int i)
			:Line{ i } {}

	};

	struct Position
	{
		Row row{ -1 };
		Column col{ -1 };

		operator bool()const { return row.valid() && col.valid(); }
		Position operator+(Position)const;
		void operator+=(Position);
		bool operator==(Position);
		bool is_white()const;
	};

	class Jump
	{
		Position from, to, kill;
	public:
		Jump(Position f, Position t, Position k = {});
		Position From()const { return from; }
		Position To()const { return to; }
		Position Kill()const { return kill; }
		bool IsKiller()const { return kill != Position{}; }
	};

	using Move = std::vector<Jump>;

	class Board
	{
		Piece* field[8][8];

	public:
		Board();
		Board(Board const&);
		Board(Board&);
		~Board();

		Board& operator=(Board const&);
		Board& operator=(Board&&);

		void Kill(Position);
		Position FindKill(Jump const&)const;
		Piece*& operator[](Position);
		Piece const* operator[](Position)const;
		std::vector<Move> test_piece(game::Position)const;
		bool MakeJump(Jump);
		std::vector<Piece*> GetPieces(Color)const;
	};

	//class Player {};

	class Checkers
	{
		Board brd;
		Color next_move;

	public:
		Checkers(Color first_move = Color::White);

		std::vector<Move> GetAvailableMoves()const;
		bool MakeJump(Jump const&);
		Color SwitchPlayer();
		Color WhoMakesTurn()const { return next_move; }
		auto& GetBoard()const { return brd; }
	};
}