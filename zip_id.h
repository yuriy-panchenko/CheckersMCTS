#pragma once
#include <bitset>

namespace id
{
	class zip32
	{
		std::bitset<32> position, color, rank;
	public:
		void set(size_t index/*0..63*/, bool isWhite, bool isQueen);
		void kill(size_t index);

		bool has(size_t index/*0..63*/)const;
		bool is_white(size_t index/*0..63*/)const;
		bool is_queen(size_t index/*0..63*/)const;
	};

	class zip64
	{
		std::bitset<64> location, color, rank;
	public:
		void set(size_t index/*0..63*/, bool isWhite, bool isQueen);
		void kill(size_t index);

		bool has(size_t index/*0..63*/)const;
		bool is_white(size_t index/*0..63*/)const;
		bool is_queen(size_t index/*0..63*/)const;
	};
}