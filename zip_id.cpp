#include "pch.h"
#include "zip_id.h"
namespace id
{
	void zip32::set(size_t index, bool isWhite, bool isQueen)
	{
		location.set(to32(index));
		color.set(to32(index), isWhite);
		rank.set(to32(index), isQueen);
	}

	size_t zip32::to32(size_t index)
	{
#ifdef DEBUG
		auto const row = index / 8;
		auto const col = index % 8;
		return row * 4 + col / 2;
#else
		return (index + (index % 8)) >> 1;
#endif // DEBUG
	}

	void zip64::set(size_t index, bool isWhite, bool isQueen)
	{
		location.set(index);
		color.set(index, isWhite);
		rank.set(index, isQueen);
	}
}