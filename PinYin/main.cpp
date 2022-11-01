#include <io.h>
#include <fcntl.h>
#include <Windows.h>
//#include <stringapiset.h>

#include <iostream>
#include <vector>
#include <string>

#include "PinYin.hpp"
#include "utils.hpp"

int main()
{
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);

	do
	{
		std::wstring in;
		std::wcout << L"输入：";
		std::wcin >> in;

		Escape(in);
		std::vector<UINT> utf32 = CCodePageU16ToU32(in);

		std::wstring out = GetPinYin(utf32);
		std::wcout << out << std::endl;

		system("pause");
		system("cls");
	} while (true);
	
#if 0
	CUnihanReadings uni;
	uni.LoadUnihanReadingsData(L"Unihan_Readings_15.txt");
	while (!uni.readings.empty())
	{
		wchar_t in;
		std::wcout << L"输入：";
		std::wcin >> in;
		const CUnihanReadings::ReadingsStruct* fi = uni.Query(in);
		if (fi != 0)
		{
			typedef CUnihanReadings::ReadingsStruct::KeyEnum KeyEnum;

			if (fi->key[KeyEnum::kCantonese] != 0) std::wcout << L"粤语拼音: " << *fi->key[KeyEnum::kCantonese] << std::endl;
			if (fi->key[KeyEnum::kDefinition] != 0) std::wcout << L"定义: " << *fi->key[KeyEnum::kDefinition] << std::endl;
			if (fi->key[KeyEnum::kHangul] != 0) std::wcout << L"韩语发音: " << *fi->key[KeyEnum::kHangul] << std::endl;
			if (fi->key[KeyEnum::kHanyuPinlu] != 0) std::wcout << L"《現代漢語頻率詞典》: " << *fi->key[KeyEnum::kHanyuPinlu] << std::endl;
			if (fi->key[KeyEnum::kHanyuPinyin] != 0) std::wcout << L"《漢語大字典》: " << *fi->key[KeyEnum::kHanyuPinyin] << std::endl;
			if (fi->key[KeyEnum::kJapaneseKun] != 0) std::wcout << L"日语发音: " << *fi->key[KeyEnum::kJapaneseKun] << std::endl;
			if (fi->key[KeyEnum::kJapaneseOn] != 0) std::wcout << L"中日发音: " << *fi->key[KeyEnum::kJapaneseOn] << std::endl;
			if (fi->key[KeyEnum::kKorean] != 0) std::wcout << L"韩语发音(不推荐): " << *fi->key[KeyEnum::kKorean] << std::endl;
			if (fi->key[KeyEnum::kMandarin] != 0) std::wcout << L"常用拼音: " << *fi->key[KeyEnum::kMandarin] << std::endl;
			if (fi->key[KeyEnum::kTang] != 0) std::wcout << L"唐代发音: " << *fi->key[KeyEnum::kTang] << std::endl;
			if (fi->key[KeyEnum::kTGHZ2013] != 0) std::wcout << L"《通用规范汉字字典》: " << *fi->key[KeyEnum::kTGHZ2013] << std::endl;
			if (fi->key[KeyEnum::kVietnamese] != 0) std::wcout << L"越南语: " << *fi->key[KeyEnum::kVietnamese] << std::endl;
			if (fi->key[KeyEnum::kXHC1983] != 0) std::wcout << L"《现代汉语词典》: " << *fi->key[KeyEnum::kXHC1983] << std::endl;
		}
		system("pause");
		system("cls");
	}
#endif // 0

	return 0;
}