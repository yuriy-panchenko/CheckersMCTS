#pragma once
#include <vector>
#include "zip_id.h"

namespace game
{
	enum class Color :bool { White, Black, };
	enum class Rank :bool { Pawn, Queen, };
	enum class Wind { NW, NE, SW, SE, };

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
		explicit Row(int i)
			:Line{ i } {}

		wchar_t to_wchar()const { return L'8' - index; }
		char to_char()const { return '8' - index; }
		bool top_half()const { return index < 4; }
	};

	//	a-0, b-1, .., h-7
	class Column
		:public Line
	{
	public:
		Column() = default;
		explicit Column(int i)
			:Line{ i } {}
		wchar_t to_wchar()const { return L'A' + index; }
		char to_char()const { return 'A' + index; }
	};

	struct Position
	{
		Row row{ -1 };
		Column col{ -1 };

		explicit operator bool()const { return row.valid() && col.valid(); }
		operator size_t()const { return index(); }
		int index()const { return (*this) ? row * 8 + col : -1; }
		Position operator+(Position)const;
		void operator+=(Position);
		bool operator==(Position);
		bool is_white()const;
		operator CString()const;
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
		Wind Direction()const;
		size_t Distance()const;
		size_t to_policy_index(bool isWhite)const;
		bool operator==(Jump const&)const;
	};

	using Move = std::vector<Jump>;
	void operator+=(std::vector<Move>&, std::vector<Move>&&);
	Color operator!(Color);
	bool is_kills(std::vector<Move> const&);
	CString ToString(Move const&);
	//std::vector<Move> operator+(std::vector<Move>&&, std::vector<Move>&&);

	class Board
	{
		id::zip64 fld;

	public:
		using location = std::pair<Piece, Position>;
	public:
		//void Kill(Position);
		//Position FindKill(Jump const&)const;
		bool operator[](Position)const;
		Piece at(Position)const;
		/*
		Piece operator[](Position)const;*/
		std::vector<Move> available_moves(game::Position)const;
		void Make(Jump);
		std::vector<location> GetPieces(Color)const;
		void init();
		void SetPieces(std::vector<location> const&);
		void Clear();
		auto GetZipID()const { return fld; }
		void SetZipID(id::zip64 z) { fld = z; }
		
		std::vector<double> encode_board(bool isWhite)const;
	};

	//class Player {};

	class Checkers
	{
		Board brd;
		Color next_move;
		Position forced;

	public:
		Checkers(Color first_move);

		std::vector<Move> GetAvailableMoves()const;
		//std::vector<Move> GetAvailableMoves(Position)const;
		std::vector<Move> GetContinuation(Position)const;
		void Do(Move const&);
		bool Do(Jump const& j);
		Color SwitchPlayer();
		Color WhoMakesTurn()const { return next_move; }
		auto& GetBoard()const { return brd; }
		auto& GetBoard() { return brd; }
		void SetZipID(id::zip64);
		bool IsWhiteTurn()const;

		std::vector<double> encode_board()const;
		std::vector<size_t> encode_legal_moves()const;
	
	private:
		//void Do(Jump const&);
	};
}

using Moves = std::vector<game::Move>;