#include "pch.h"
#include "zip_id.h"
namespace id
{
	void zip64::set(size_t index, bool isWhite, bool isQueen)
	{
		location.set(index);
		color.set(index, isWhite);
		rank.set(index, isQueen);
	}
}