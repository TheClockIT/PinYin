#include <io.h>
#include <fcntl.h>
#include <Windows.h>
//#include <stringapiset.h>

#include <iostream>
#include <vector>
#include <string>

#include "PinYin.h"

std::wstring UTF8ToUTF16(const std::string UTF8);
std::u32string UTF8ToUTF32(const char8_t* pUTF8);
constexpr char32_t UTF16ToUTF32(const char16_t* pUTF16);
unsigned int UTF32ToUTF16(char32_t UTF32, std::wstring& UTF16);

int main()
{
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);

	std::wstring in;
	std::wstring out;
	do
	{
		std::wcout << L"输入汉字：";
		std::wcin >> in;

		out.clear();
		if (!in.empty())
		{
			for (size_t i = 0; i < in.size(); i++)
			{
				if (i != 0)
					out += L" ";

				size_t j = UTF16ToUTF32((char16_t*)&in[i]);
				if (j > 0xFFFF)
					i++;

				// 0x2E80  - 0x2EFF  CJK Radicals Supplement 汉字部首 128个
				// 0x2F00  - 0x2FDF  Kangxi Radicals         康熙部首 224个
				// 
				// 0x3400  - 0x4DBF  CJK Unified Ideographs Extension A			6592个汉字
				// 0x4E00  - 0x9FFF  CJK Unified Ideographs						20992个汉字
				// 0xF900  - 0xFAFF  CJK Compatibility Ideographs				512个汉字
				// 0x20000 - 0x2A6DF CJK Unified Ideographs Extension B			42720个汉字
				// 0x2A700 - 0x2B73F CJK Unified Ideographs Extension C			4160个汉字
				// 0x2B740 - 0x2B81F CJK Unified Ideographs Extension D			224个汉字
				// 0x2B820 - 0x2CEAF CJK Unified Ideographs Extension E			5776个汉字
				// 0x2CEB0 - 0x2EBEF CJK Unified Ideographs Extension F			7488个汉字
				// 0x2F800 - 0x2FA1F CJK Compatibility Ideographs Supplement	544个汉字
				// 0x30000 - 0x3134F CJK Unified Ideographs Extension G			4944个汉字

				if (j >= 0x3400 && j <= 0x4DBF)			// 0x0000  - 0x19BF
					j -= 0x3400;								   
				else if (j >= 0x4E00 && j <= 0x9FFF)	// 0x19C0  - 0x6BBF
					j -= 0x3440;								   
				else if (j >= 0xF900 && j <= 0xFAFF)	// 0x6BC0  - 0x6DBF
					j -= 0x8D40;								   
				else if (j >= 0x20000 && j <= 0x2A6DF)	// 0x6DC0  - 0x1149F
					j -= 0x19240;
				else if (j >= 0x2A700 && j <= 0x2EBEF)	// 0x114A0 - 0x1598F
					j -= 0x19260;
				else if (j >= 0x2F800 && j <= 0x2FA1F)	// 0x15990 - 0x15BAF
					j -= 0x19E70;
				else if (j >= 0x30000 && j <= 0x3134F)	// 0x15BB0 - 0x16EFF
					j -= 0x1A450;
				else
				{
				PinYinOutSource:
					out += in[i];
					continue;
				}

				for (size_t k = 0; k < 7; k++)
				{
					if (PinYin[j][k][0] == 0)
						goto PinYinOutSource;
					if (k != 0)
						out += L"/";

					out += UTF8ToUTF16((char*)PinYin[j][k]); // 将 PinYin[] 的类型改为wchar_t/char16_t可以节省这里的转换计算，但会牺牲更多内存
				}
				goto PinYinOutSource;
			}
		}
		else
		{
			break;
		}

		std::wcout << out << std::endl;
	} while (true);
}

std::wstring UTF8ToUTF16(const std::string UTF8)
{
	std::wstring UTF16;

	std::u32string UTF32 = UTF8ToUTF32((char8_t*)&UTF8[0]);
	for (size_t i = 0; i < UTF32.size(); i++)
	{
		if (UTF32ToUTF16(UTF32[i], UTF16) == 0)
			break;
	}

	return UTF16;
}
std::u32string UTF8ToUTF32(const char8_t* pUTF8)
{
	std::u32string UTF32;
	if (pUTF8 == NULL)
		return UTF32;
	while (*pUTF8 != u8'\0')
	{
		char8_t b = *pUTF8++;
		if (b < 0x80)
		{
			UTF32 += char32_t(b);
			continue;
		}
		if (b < 0xC0 || b > 0xFD)
			return UTF32;    // 非法UTF8

		char32_t result = 0;
		unsigned int iLen = 0;
		if (b < 0xE0)
		{
			result = b & 0x1F;
			iLen = 2;
		}
		else if (b < 0xF0)
		{
			result = b & 0x0F;
			iLen = 3;
		}
		else if (b < 0xF8)
		{
			result = b & 7;
			iLen = 4;
		}
		else if (b < 0xFC)
		{
			result = b & 3;
			iLen = 5;
		}
		else
		{
			result = b & 1;
			iLen = 6;
		}

		for (unsigned int i = 1; i < iLen; i++)
		{
			b = *pUTF8++;
			if (b < 0x80 || b > 0xBF)
				return UTF32; // 非法UTF8

			result = (result << 6) + (b & 0x3F);
		}

		UTF32 += result;
	}

	return UTF32;
}
constexpr char32_t UTF16ToUTF32(const char16_t* pUTF16)
{
	if (pUTF16 == NULL)
		return 0;

	const char16_t& w1 = pUTF16[0];
	if (w1 >= 0xD800 && w1 <= 0xDFFF)
	{
		// 编码在替代区域（Surrogate Area）  
		if (w1 < 0xDC00)
		{
			const char16_t& w2 = pUTF16[1];
			if (w2 >= 0xDC00 && w2 <= 0xDFFF)
				return (w2 & 0x03FF) + (((w1 & 0x03FF) + 0x40) << 10);
		}

		return 0;   // 非法UTF16编码      
	}
	else
		return w1;
}
unsigned int UTF32ToUTF16(char32_t UTF32, std::wstring& UTF16)
{
	if (UTF32 <= 0xFFFF)
	{
		UTF16 += static_cast<wchar_t>(UTF32);
		return 1;
	}
	else if (UTF32 <= 0xEFFFF)
	{
		UTF16 += static_cast<wchar_t>(0xD800 + (UTF32 >> 10) - 0x40); // 高10位
		UTF16 += static_cast<wchar_t>(0xDC00 + (UTF32 & 0x03FF));     // 低10位
		return 2;
	}
	else
		return 0;
}