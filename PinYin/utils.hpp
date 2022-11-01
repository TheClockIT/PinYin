#include <cassert>

/// <summary>
/// UTF8/ANSI -> UTF16
/// </summary>
/// <param name="src">多字节字符串</param>
/// <param name="CodePage">传入字符串代码页，默认CP_UTF8</param>
/// <returns>返回转换后的UTF-16字符串</returns>
inline std::wstring CCodePage(const std::string& src, const UINT& CodePage = CP_UTF8)
{
	if (src.empty())
		return std::wstring();
	const int len = MultiByteToWideChar(CodePage, 0, src.c_str(), -1, 0, 0);
	if (len > 0)
	{
		std::vector<wchar_t> vw(len);
		MultiByteToWideChar(CodePage, 0, src.c_str(), -1, &vw[0], len);
		return &vw[0];
	}
	return std::wstring();
}

/// <summary>
/// UTF16 -> UTF8/ANSI
/// </summary>
/// <param name="src">UTF16字符串</param>
/// <param name="CodePage">目标代码页，默认CP_UTF8</param>
/// <returns>返回转换后的多字节字符串</returns>
inline std::string CCodePage(const std::wstring& src, const UINT& CodePage = CP_UTF8)
{
	if (src.empty())
		return std::string();
	const int len = WideCharToMultiByte(CodePage, 0, src.c_str(), -1, 0, 0, 0, 0);
	if (len > 0)
	{
		std::vector<char> vw(len);
		WideCharToMultiByte(CodePage, 0, src.c_str(), -1, &vw[0], len, 0, 0);
		return &vw[0];
	}
	return std::string();
}

inline std::vector<UINT> CCodePageU8ToU32(const std::string& src)
{
	std::vector<UINT> result;
	const size_t s = src.size();
	for (size_t i = 0; i < s; i++)
	{
		UINT numBytes;
		const UCHAR& ch = (UCHAR)src[i]; // 防止 char 类型变成负数

		if (ch > 0 && ch < 0x80)
		{
			result.push_back(src[i]);
			continue;
		}
		else if (ch < 0xC0 || ch > 0xFD)
			continue;
		else if (ch >= 0xC2 && ch < 0xE0)
			numBytes = 2;
		else if (ch >= 0xE0 && ch < 0xF0)
			numBytes = 3;
		else if (ch >= 0xF0 && ch < 0xF8)
			numBytes = 4;
		else if (ch >= 0xF8 && ch < 0xFC)
			numBytes = 5;
		else
			numBytes = 6;

		switch (numBytes)
		{
		case 2:
			result.push_back(((src[i++] & 0x1F) << 6) | (src[i] & 0x3F));
			break;
		case 3: case 4: case 5: case 6:
		{
			UINT disp = (numBytes - 1) * 6;
			UINT temp = ((src[i++] & 0x0F) << disp);
			while (disp > 6)
			{
				disp -= 6;
				temp |= (src[i++] & 0x3F) << disp;
			}
			temp |= src[i] & 0x3F;
			result.push_back(temp);
			break;
		}
		default:
			break;
		}
	}
	return result;
}

inline std::vector<UINT> CCodePageU16ToU32(const std::wstring src)
{
	std::vector<UINT> result;
	for (size_t i = 0; i < src.size(); i++)
	{
		const wchar_t& w1 = src[i];
		if (w1 >= 0xD800 && w1 <= 0xDFFF)
		{
			const size_t j = i + 1;
			if (j < src.size() && w1 < 0xDC00)
			{
				const wchar_t& w2 = src[j];
				if (w2 >= 0xDC00 && w2 <= 0xDFFF)
				{
					result.push_back(UINT(w2 & 0x03FF) + ((UINT(w1 & 0x03FF) + 0x40) << 10));
					continue;
				}
				assert(!"非法UTF16编码");
			}
			else
			{
				assert(!"非法UTF16编码");
			}
		}
		result.push_back((UINT)w1);
	}

	return result;
}

/// <summary>
/// UTF32 -> UTF16
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="src"></param>
/// <returns></returns>
inline std::wstring CCodePageU32ToU16(const UINT* src, const size_t& count)
{
	std::wstring result;
	if (src != 0)
	{
		for (size_t i = 0; i < count; i++)
		{
			if (src[i] <= 0xFFFF)
			{
				result += static_cast<wchar_t>(src[i]);
			}
			else if (src[i] <= 0x10FFFF)
			{
				result += static_cast<wchar_t>(0xD800 + (src[i] >> 10) - 0x40); // 高10位
				result += static_cast<wchar_t>(0xDC00 + (src[i] & 0x03FF));     // 低10位
			}
			else
			{
				assert(!"非法UTF32编码");
			}
		}
	}
	return result;
}

class CUnihanReadings
{
public:
	~CUnihanReadings()
	{
		for (auto& i : readings)
		{
			i.DeleteKey();
		}
	}
	struct ReadingsStruct
	{
		enum KeyEnum
		{
			kCantonese = 0,
			kDefinition,
			kHangul,
			kHanyuPinlu,
			kHanyuPinyin,
			kJapaneseKun,
			kJapaneseOn,
			kKorean,
			kMandarin,
			kTang,
			kTGHZ2013,
			kVietnamese,
			kXHC1983,
			kCount
		};
		UINT code = 0;
		std::wstring* key[KeyEnum::kCount] = { 0 };
		void DeleteKey()
		{
			for (size_t i = 0; i < KeyEnum::kCount; i++)
			{
				if (key[i] != 0)
				{
					delete key[i];
					key[i] = 0;
				}
			}
		}

		// 重载 == 运算符，主要给 std::find() 函数用
		bool operator == (const UINT& n) { return code == n; }
	};

	std::vector<ReadingsStruct> readings;

	// 加载 Unihan_Readings.txt 的数据
	void LoadUnihanReadingsData(wchar_t const* _FileName = L"Unihan_Readings.txt")
	{
		typedef ReadingsStruct::KeyEnum KeyEnum;

		FILE* pFile = _wfsopen(_FileName, L"r, ccs=UTF-8", _SH_DENYWR);
		if (pFile == NULL)
			return;

		wchar_t Buffer[255] = { 0 };

		std::vector<std::wstring> UnihanReadings;

		while (!feof(pFile))
		{
			fgetws(Buffer, 255, pFile);
			if (Buffer[0] != L'U')
				continue;
			UnihanReadings.push_back(Buffer);
		};

		fclose(pFile);

		if (!UnihanReadings.empty())
		{
			bool isContinue = false;
			wchar_t* EndPtr = 0;

			const wchar_t* findStr[KeyEnum::kCount] = {
				L"kCantonese",
				L"kDefinition",
				L"kHangul",
				L"kHanyuPinlu",
				L"kHanyuPinyin",
				L"kJapaneseKun",
				L"kJapaneseOn",
				L"kKorean",
				L"kMandarin",
				L"kTang",
				L"kTGHZ2013",
				L"kVietnamese",
				L"kXHC1983",
			};
			const size_t lens[KeyEnum::kCount] = {
				11,12,8,12,13,13,12,8,10,6,10,12,9
			};

			for (size_t i = 0; i < UnihanReadings.size(); i++)
			{
				const std::wstring& item = UnihanReadings[i];
				size_t fi = (size_t)wcstol(item.c_str() + 2, &EndPtr, 16);

				if (readings.empty() || (UINT)fi != readings.back().code)
				{
					readings.push_back(ReadingsStruct());
					readings.back().code = (UINT)fi;
				}

				ReadingsStruct& uni = readings.back();
				for (size_t j = 0; j < KeyEnum::kCount; j++)
				{
					fi = item.find(findStr[j], 7);
					if (fi != (size_t)-1)
					{
						const size_t& len1 = lens[j];
						const size_t& len2 = fi + len1 + 1;
						assert(uni.key[j] == 0);
						uni.key[j] = new std::wstring(UnihanReadings[i].substr(fi + len1, UnihanReadings[i].size() - len2));
						break;
					}
				}
			}
		}
	}

	// 查询
	const ReadingsStruct* Query(const wchar_t& in)
	{
		const auto& fi = std::find(readings.begin(), readings.end(), (UINT)in);
		if (fi == readings.end())
			return 0;
		return &(*fi);
	}

#if 0
	// 对比 14.0.0 和 15.0.0
	// 15相对于14，在拼音上没有改变，代码暂时保留，以后新版本可能会用上
	void test()
	{
		CUnihanReadings unia;
		unia.LoadUnihanReadingsData(L"Unihan_Readings_14.txt");

		CUnihanReadings unib;
		unib.LoadUnihanReadingsData(L"Unihan_Readings_15.txt");

		typedef CUnihanReadings::ReadingsStruct::KeyEnum KeyEnum;
		for (auto i = unia.readings.begin(); i != unia.readings.end(); i += 1)
		{
			for (auto j = unib.readings.begin(); j != unib.readings.end(); j += 1)
			{
				if (i->code == j->code)
				{
					if (i->key[KeyEnum::kHanyuPinlu] != 0 && j->key[KeyEnum::kHanyuPinlu] != 0 && i->key[KeyEnum::kHanyuPinlu]->compare(*j->key[KeyEnum::kHanyuPinlu]) != 0)
					{
						std::wcout << L"0x" << std::hex << i->code
							<< L"\n\t14:kHanyuPinlu: " << *i->key[KeyEnum::kHanyuPinlu]
							<< L"\n\t15:kHanyuPinlu: " << *j->key[KeyEnum::kHanyuPinlu]
							<< std::endl;
					}
					if (i->key[KeyEnum::kHanyuPinyin] != 0 && j->key[KeyEnum::kHanyuPinyin] != 0 && i->key[KeyEnum::kHanyuPinyin]->compare(*j->key[KeyEnum::kHanyuPinyin]) != 0)
					{
						std::wcout << L"0x" << std::hex << i->code
							<< L"\n\t14:kHanyuPinyin: " << *i->key[KeyEnum::kHanyuPinyin]
							<< L"\n\t15:kHanyuPinyin: " << *j->key[KeyEnum::kHanyuPinyin]
							<< std::endl;
					}
					if (i->key[KeyEnum::kMandarin] != 0 && j->key[KeyEnum::kMandarin] != 0 && i->key[KeyEnum::kMandarin]->compare(*j->key[KeyEnum::kMandarin]) != 0)
					{
						std::wcout << L"0x" << std::hex << i->code
							<< L"\n\t14:kMandarin: " << *i->key[KeyEnum::kMandarin]
							<< L"\n\t15:kMandarin: " << *j->key[KeyEnum::kMandarin]
							<< std::endl;
					}
					if (i->key[KeyEnum::kTGHZ2013] != 0 && j->key[KeyEnum::kTGHZ2013] != 0 && i->key[KeyEnum::kTGHZ2013]->compare(*j->key[KeyEnum::kTGHZ2013]) != 0)
					{
						std::wcout << L"0x" << std::hex << i->code
							<< L"\n\t14:kTGHZ2013: " << *i->key[KeyEnum::kTGHZ2013]
							<< L"\n\t15:kTGHZ2013: " << *j->key[KeyEnum::kTGHZ2013]
							<< std::endl;
					}

					if (i->key[KeyEnum::kXHC1983] != 0 && j->key[KeyEnum::kXHC1983] != 0 && i->key[KeyEnum::kXHC1983]->compare(*j->key[KeyEnum::kXHC1983]) != 0)
					{
						std::wcout << L"0x" << std::hex << i->code
							<< L"\n\t14:kXHC1983: " << *i->key[KeyEnum::kXHC1983]
							<< L"\n\t15:kXHC1983: " << *j->key[KeyEnum::kXHC1983]
							<< std::endl;
					}

					j->DeleteKey();
					unib.readings.erase(j);
					break;
				}
			}
		}
	}
#endif // 0
};

// 字符转义功能
// \x 将\x后的16进制字符串(\x20-\x10FFFF)转为字符
// \d 将\x后的10进制字符串(\d32-\d1114111)转为字符
// \o 将\x后的8进制字符串(\o40-\o4177777)转为字符
// \b 将\x后的2进制字符串(\b100000-\b100001111111111111111)转为字符
static void Escape(std::wstring& in)
{
	size_t lxi = 0;
	do
	{
		enum class EscapeEnum : char
		{
			none = 0,
			x,
			d,
			o,
			b,
		};
		EscapeEnum flag = EscapeEnum::none;

		size_t tlxi = in.find(L"\\x", lxi);
		if (tlxi != -1)
			flag = EscapeEnum::x;
		else if ((tlxi = in.find(L"\\d", lxi)) != -1)
			flag = EscapeEnum::d;
		else if ((tlxi = in.find(L"\\o", lxi)) != -1)
			flag = EscapeEnum::o;
		else if ((tlxi = in.find(L"\\b", lxi)) != -1)
			flag = EscapeEnum::b;

		if (flag != EscapeEnum::none)
		{
			lxi = tlxi;
			const size_t lxi2 = lxi + 2;
			wchar_t* EndPtr = NULL;
			const wchar_t* StartPtr = in.c_str() + lxi2;

			switch (flag)
			{
			case EscapeEnum::x:
			case EscapeEnum::d:
			case EscapeEnum::o:
			case EscapeEnum::b:
			{
				const UINT l = (UINT)wcstol(StartPtr, &EndPtr, ((flag == EscapeEnum::x) ? 16 : (flag == EscapeEnum::d) ? 10 : (flag == EscapeEnum::o) ? 8 : 2));
				if ((l >= 0x20 || l == 0x09) && l <= 0x10FFFF)	// 0x09是TAB缩进字形, wchar_t/char16_t只支持到0x10FFFF，UTF32大于0x10FFFF的字形无法显示。
				{
					std::wstring UTF16 = CCodePageU32ToU16(&l, 1);

					const size_t count = EndPtr - StartPtr + 2;
					in.replace(lxi, count, UTF16);
				}
				else
					lxi += 2;
				break;
			}
			default:
				break;
			}

			continue;
		}

		break;
	} while (true);
};

std::wstring GetPinYin(std::vector<UINT> in)
{
	std::wstring result;
	if (!in.empty())
	{
		for (size_t i = 0; i < in.size(); i++)
		{
			if (i != 0)
				result += L" ";

			UINT j = in[i];
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
				goto PinYinOutSource;

			for (size_t k = 0; k < 7; k++)
			{
				if (PinYin[j][k][0] == 0)
					break;
				if (k != 0)
					result += L"/";

				result += CCodePage(PinYin[j][k]); // 将 PinYin[] 的类型改为wchar_t/char16_t可以节省这里的转换计算，但会牺牲更多内存
			}
		PinYinOutSource:
			result += CCodePageU32ToU16(&in[i], 1);
		}
	}
	return result;
}