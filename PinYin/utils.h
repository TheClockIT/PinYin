#pragma once

unsigned int UTF32ToUTF16(char32_t UTF32, std::wstring& UTF16)
{
	if (UTF32 <= 0xFFFF)
	{
		UTF16 += static_cast<wchar_t>(UTF32);
		return 1;
	}
	else if (UTF32 <= 0x10FFFF)
	{
		UTF16 += static_cast<wchar_t>(0xD800 + (UTF32 >> 10) - 0x40); // 高10位
		UTF16 += static_cast<wchar_t>(0xDC00 + (UTF32 & 0x03FF));     // 低10位
		return 2;
	}
	else
		return 0;
}
unsigned int UTF32ToUTF8(char32_t UTF32, char8_t* pUTF8)
{
	constexpr unsigned char Prefix[] = { 0, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
	constexpr unsigned int  CodeUp[] = {
			0x80,           // U+00000000 ～ U+0000007F  
			0x800,          // U+00000080 ～ U+000007FF  
			0x10000,        // U+00000800 ～ U+0000FFFF  
			0x200000,       // U+00010000 ～ U+001FFFFF  
			0x4000000,      // U+00200000 ～ U+03FFFFFF  
			0x80000000      // U+04000000 ～ U+7FFFFFFF  
	};

	unsigned int i = 0;

	// 根据UCS4编码范围确定对应的UTF-8编码字节数  
	unsigned int iLen = (sizeof(CodeUp) / sizeof(*CodeUp));
	for (i = 0; i < iLen; i++)
	{
		if (UTF32 < CodeUp[i])
			break;
	}

	if (i == iLen)
		return 0;    // 无效的UCS4编码  

	iLen = i + 1;   // UTF-8编码字节数  
	if (pUTF8 != NULL)
	{
		// 转换为UTF-8编码  
		for (; i > 0; i--)
		{
			pUTF8[i] = static_cast<char8_t>((UTF32 & 0x3F) | 0x80);
			UTF32 >>= 6;
		}
		pUTF8[0] = static_cast<char8_t>(UTF32 | Prefix[iLen - 1]);
	}

	return iLen;
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
std::wstring UTF8ToUTF16(const std::u8string UTF8)
{
	std::wstring UTF16;

	std::u32string UTF32 = UTF8ToUTF32(&UTF8[0]);
	for (size_t i = 0; i < UTF32.size(); i++)
	{
		if (UTF32ToUTF16(UTF32[i], UTF16) == 0)
			break;
	}

	return UTF16;
}

#include <wininet.h>
#pragma comment(lib, "wininet.lib")
//模拟浏览器发送HTTP请求函数
std::string HttpRequest(const wchar_t* pHostName, const short sPort, const wchar_t* pUrl, const wchar_t* pMethod, char* pPostData, const int nPostDataLen)
{
	std::string strResponse;

	HINTERNET hInternet = InternetOpenW(L"User-Agent", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (!hInternet)
		return strResponse;

	HINTERNET hConnect = InternetConnectW(hInternet, pHostName, sPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	if (!hConnect)
	{
		InternetCloseHandle(hInternet);
		return strResponse;
	}

	HINTERNET hRequest = HttpOpenRequestW(hConnect, pMethod, pUrl, NULL, NULL, NULL, INTERNET_FLAG_RELOAD, 0);
	if (!hRequest)
	{
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);
		return strResponse;
	}

	BOOL bRet = HttpSendRequestW(hRequest, NULL, 0, pPostData, nPostDataLen);
	while (TRUE)
	{
		char cReadBuffer[10000] = { 0 };
		unsigned long lNumberOfBytesRead;
		bRet = InternetReadFile(hRequest, cReadBuffer, sizeof(cReadBuffer) - 1, &lNumberOfBytesRead);
		if (!bRet || !lNumberOfBytesRead)
			break;
		cReadBuffer[lNumberOfBytesRead] = 0;
		strResponse += cReadBuffer;	// 最终响应结果
	}

	InternetCloseHandle(hRequest);
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInternet);

	return strResponse;
}

void Reptile()
{
	static const unsigned int ranges[] =
	{
		0x3400, 0x4DBF,   // CJK Unified Ideographs Extension A
		0x4E00, 0x9FFF,   // CJK Unified Ideographs
		0xF900, 0xFAFF,   // CJK Compatibility Ideographs
		0x20000, 0x2A6DF, // CJK Unified Ideographs Extension B
		0x2A700, 0x2EBEF, // CJK Unified Ideographs Extension C-F
		0x2F800, 0x2FA1F, // CJK Compatibility Ideographs Supplement
		0x30000, 0x3134F, // CJK Unified Ideographs Extension G
	};
	constexpr size_t rangesSize = sizeof(ranges) / sizeof(*ranges);

	FILE* pFile = _wfsopen(L"Reptile.txt", L"w, ccs=UNICODE", _SH_DENYWR);
	if (pFile == NULL)
		return;

	for (size_t i = 0; i < rangesSize; i += 2)
	{
		for (size_t j = ranges[i]; j <= ranges[i + 1]; j++)
		{
			size_t pyIndex = j;
			if (j >= 0x3400 && j <= 0x4DBF)			// 0x00000 - 0x019BF
				pyIndex -= 0x3400;
			else if (j >= 0x4E00 && j <= 0x9FFF)	// 0x019C0 - 0x06BBF
				pyIndex -= 0x3440;
			else if (j >= 0xF900 && j <= 0xFAFF)	// 0x06BC0 - 0x06DBF
				pyIndex -= 0x8D40;
			else if (j >= 0x20000 && j <= 0x2A6DF)	// 0x06DC0 - 0x1149F
				pyIndex -= 0x19240;
			else if (j >= 0x2A700 && j <= 0x2EBEF)	// 0x114A0 - 0x1598F
				pyIndex -= 0x19260;
			else if (j >= 0x2F800 && j <= 0x2FA1F)	// 0x15990 - 0x15BAF
				pyIndex -= 0x19E70;
			else if (j >= 0x30000 && j <= 0x3134F)	// 0x15BB0 - 0x16EFF
				pyIndex -= 0x1A450;
			else
				break;

			if (pyIndex >= PinYinSize)
				break;

			if (pyIndex <= 0x6DBF && PinYin[pyIndex][0][0] != 0)
			{
				fwprintf_s(pFile, L"{ ");
				for (size_t k = 0; k < 7; k++)
				{
					if (PinYin[pyIndex][k][0] == 0)
						break;
					fwprintf_s(pFile, L"u8\"%s\", ", (wchar_t*)PinYin[pyIndex][k]);
				}
				fwprintf_s(pFile, L"},\n");
				fflush(pFile);
			}
			else
			{
				//_sleep(5000);//延时5秒 
				const unsigned int len = UTF32ToUTF8((char32_t)j, NULL) + 1;
				char8_t* pUtf8 = new char8_t[len];
				memset(pUtf8, 0, len);
				UTF32ToUTF8((char32_t)j, pUtf8);

				std::wstring url = L"/hans/";
				for (size_t k = 0; k < (len - 1); k++)
				{
					wchar_t buffer[3];
					_itow_s(pUtf8[k], buffer, 16);
					url += L"%";
					url += buffer;
				}

				delete[] pUtf8;
				pUtf8 = NULL;

				std::string strResponse = HttpRequest(L"www.zdic.net", 80, url.c_str(), L"GET", NULL, 0);
				size_t index = strResponse.find((char*)u8"<span class=\"z_ts2\">拼音</span>");
				wchar_t zi = 0;
				if (index != -1)
				{
					std::vector<std::wstring> yin;

					index = 0;
					do
					{
						index = strResponse.find("z_d song", index);
						if (index != -1)
						{
							index += 10;
							size_t NextIndex = strResponse.find("<", index);
							std::wstring val = UTF8ToUTF16((char8_t*)strResponse.substr(index, NextIndex - index).c_str());
							if (!val.empty() && val[0] < 0x2000)
							{
								yin.push_back(val.c_str());
							}
							else
								break;
							index = NextIndex;
						}
						else
							break;
					} while (true);

					fwprintf_s(pFile, L"{ ");
					for (size_t k = 0; k < yin.size(); k++)
						fwprintf_s(pFile, L"u8\"%s\", ", yin[k].c_str());
					fwprintf_s(pFile, L"},\n");
				}
				else
					fwprintf_s(pFile, L"{ },\n");
				fflush(pFile);
			}
		}
	}

	fclose(pFile);
}

// 删除掉重复拼音
void DeleteDuplicate(char8_t _PinYin[][7][8], size_t _Size)
{
	FILE* pFile = _wfsopen(L"DeleteDuplicate.txt", L"w, ccs=UNICODE", _SH_DENYWR);
	if (pFile == NULL)
		return;

	for (size_t i = 0; i < _Size; i++)
	{
		fwprintf_s(pFile, L"{ ");

		std::vector<std::u8string> val;
		for (size_t j = 0; j < 7; j++)
		{
			if (_PinYin[i][j][0] == '\0')
				break;

			bool isAdd = true;
			for (size_t k = 0; k < val.size(); k++)
			{
				if (val[k] == _PinYin[i][j])
				{
					isAdd = false;
					break;
				}
			}

			if (isAdd)
			{
				std::wstring wval = UTF8ToUTF16(_PinYin[i][j]);
				fwprintf_s(pFile, L"u8\"%s\", ", wval.c_str());
				val.push_back(_PinYin[i][j]);
			}
		}

		fwprintf_s(pFile, L"},\n");
		fflush(pFile);
	}

	fclose(pFile);
}