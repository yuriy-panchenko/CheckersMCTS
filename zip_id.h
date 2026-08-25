#pragma once
#include <bitset>

namespace id
{
	class zip64;

	class zip32
	{
		std::bitset<32> location, color, rank;
	public:
		zip32() = default;
		zip32(zip32 const&) = default;
		zip32(zip32&&) = default;
		explicit zip32(zip64);

		zip32& operator=(zip32 const&) = default;
		zip32& operator=(zip32&&) = default;

		void set(size_t index/*0..63*/, bool isWhite, bool isQueen);
		void kill(size_t index) { location.set(to32(index), false); }

		bool has(size_t index/*0..63*/)const { return location.test(to32(index)); }
		bool is_white(size_t index/*0..63*/)const { return color.test(to32(index)); }
		bool is_queen(size_t index/*0..63*/)const { return rank.test(to32(index)); }

		static size_t to32(size_t);
	};

	class zip64
	{
		std::bitset<64> location, color, rank;
	public:
		zip64() = default;
		zip64(zip64 const&) = default;
		zip64(zip64&&) = default;
		explicit zip64(zip32);

		zip64& operator=(zip64 const&) = default;
		zip64& operator=(zip64&&) = default;

		void set(size_t index/*0..63*/, bool isWhite, bool isQueen);
		void kill(size_t index) { location.set(index, false); }

		bool has(size_t index/*0..63*/)const { return location.test(index); }
		bool is_white(size_t index/*0..63*/)const { return color.test(index); }
		bool is_queen(size_t index/*0..63*/)const { return rank.test(index); }
	};
}