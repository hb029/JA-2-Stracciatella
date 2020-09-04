#include "Debug_Pages.h"
#include "Font.h"
#include "Font_Control.h"

#include <string_theory/format>
#include <string_theory/string>


void MPageHeader(const ST::string& header)
{
	SetFont(DEBUG_PAGE_FONT);
	SetFontColors(DEBUG_PAGE_HEADER_COLOR);
	MPrint(DEBUG_PAGE_FIRST_COLUMN, DEBUG_PAGE_SCREEN_OFFSET_Y, header);
	SetFontColors(DEBUG_PAGE_TEXT_COLOR);
}


void MHeader(INT32 x, INT32 y, const ST::string& header)
{
	SetFontColors(DEBUG_PAGE_HEADER_COLOR);
	MPrint(x, y, header);
	SetFontColors(DEBUG_PAGE_TEXT_COLOR);
}


void MPrintStat(INT32 x, INT32 y, const ST::string& header, INT32 val)
{
	MHeader(x, y, header);
	MPrint(x+DEBUG_PAGE_LABEL_WIDTH, y, ST::format("{}", val));
}


void MPrintStat(INT32 x, INT32 y, const ST::string& header, const ST::string& val)
{
	MHeader(x, y, header);
	MPrint(x+DEBUG_PAGE_LABEL_WIDTH, y, ST::format("{}", val));
}


void MPrintStat(INT32 x, INT32 y, const ST::string& header, const void* val)
{
	MHeader(x, y, header);
	MPrint(x+DEBUG_PAGE_LABEL_WIDTH, y, ST::format("{#x}", reinterpret_cast<uintptr_t>(val)));
}


void MPrintStat(INT32 x, INT32 y, const ST::string& header, INT32 const val, INT32 const effective_val)
{
	MHeader(x, y, header);
	MPrint(x+DEBUG_PAGE_LABEL_WIDTH, y, ST::format("{} ( {} )", val, effective_val));
}
