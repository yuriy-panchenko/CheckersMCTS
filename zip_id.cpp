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

	void zip32::clear()
	{
		location.reset();
		color.reset();
		rank.reset();
	}

	size_t zip32::to32(size_t index)
	{
#ifdef DEBUG
		auto const row = index / 8;
		auto const col = index % 8;
		return row * 4 + col / 2;
#else
		return index / 8 * 4 + (index % 8) / 2;
#endif // DEBUG
	}

	bool zip64::operator==(zip64 const& oth) const
	{
		return location == oth.location
			&& get_color() == oth.get_color()
			&& get_rank() == oth.get_rank();
	}

	bool zip64::operator<(zip64 const& oth) const
	{
		if (location.to_ullong() != oth.location.to_ullong())
			return location.to_ullong() < oth.location.to_ullong();

		auto const myColor = get_color().to_ullong();
		auto const othColor = oth.get_color().to_ullong();
		if (myColor != othColor)
			return myColor < othColor;

		return get_rank().to_ullong() < oth.get_rank().to_ullong();
	}

	/*bool zip64::operator<(zip64 const& oth) const
	{
		if (location != oth.location)
			return location.to_ullong() != oth.location.to_ullong();
		if (get_color() != oth.get_color())
			return get_color().to_ullong() < oth.get_color().to_ullong();
		return get_rank().to_ullong() < oth.get_rank().to_ullong();
	}*/

	void zip64::set(size_t index, bool isWhite, bool isQueen)
	{
		location.set(index);
		color.set(index, isWhite);
		rank.set(index, isQueen);
	}

	void zip64::clear()
	{
		*this = {};
	}

	void zip64::move(size_t dstIndex, size_t srcIndex)
	{
		location.set(dstIndex);
		color.set(dstIndex, color.test(srcIndex));
		rank.set(dstIndex, rank.test(srcIndex));

		location.set(srcIndex, false);
		//color.set(srcIndex, false);
		//rank.set(srcIndex, false);
	}

	void zip64::set_rank(size_t index, bool isQueen)
	{
		rank.set(index, isQueen);
	}
}