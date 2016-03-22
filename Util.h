#pragma once

#define MAX_MEMORY_FILE_SIZE 1024 * 1024 * 256

namespace file {
	bool Exists(const TCHAR *filePath);
	size_t GetSize(const TCHAR *filePath);
	bool ReadAll(const TCHAR *filePath, char *buffer, size_t bufferLen);
	bool WriteAll(const TCHAR *filePath, void *data, size_t dataLen);
	bool Delete(const TCHAR *filePath);
	FILETIME GetModificationTime(const TCHAR *filePath);
}

void GetCurrentTimeString( TCHAR *wszTime, UINT nLen );
void DumpPropVariant(PROPVARIANT *pPropVar, TCHAR* wszVar, INT nLen);
std::string ToUtf8(const std::wstring& str);
