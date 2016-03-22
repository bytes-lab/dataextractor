// MetaExtractor.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "DEFolder.h"
#include "Util.h"

int _tmain(int argc, _TCHAR* argv[])
{
	if(argc < 3) {
		_tprintf(L"Please specify a document folder and report folder paths.");
		exit(0);
	}

	TCHAR wszStartTime[50];
	TCHAR wszEndTime[50];

	JSONObject jsonRoot;
	JSONObject jsonSummary;
	JSONObject jsonSummaryFiles;
	JSONObject jsonSummaryFilesByType;
	JSONObject jsonSummaryTime;
	JSONValue *jsonReport, *jsonFolderReport;
	CDEFolder *eBookFolder = new CDEFolder();

	// store start time.
	GetCurrentTimeString(wszStartTime, 50);

	if(!eBookFolder->Load(argv[1])) {
		delete eBookFolder;
		exit(0);
	}
	jsonFolderReport = eBookFolder->Serialize();

	// store end time.
	GetCurrentTimeString(wszEndTime, 30);

#ifdef SUPPORT_DOC
	jsonSummaryFilesByType[L"doc"] = new JSONValue((double)eBookFolder->GetEBookCount(L".doc"));
#endif
#ifdef SUPPORT_XLS
	jsonSummaryFilesByType[L"xls"] = new JSONValue((double)eBookFolder->GetEBookCount(L".xls"));
#endif
#ifdef SUPPORT_PPT
	jsonSummaryFilesByType[L"ppt"] = new JSONValue((double)eBookFolder->GetEBookCount(L".ppt"));
#endif
#ifdef SUPPORT_DOCX
	jsonSummaryFilesByType[L"docx"] = new JSONValue((double)eBookFolder->GetEBookCount(L".docx"));
#endif
#ifdef SUPPORT_XLSX
	jsonSummaryFilesByType[L"xlsx"] = new JSONValue((double)eBookFolder->GetEBookCount(L".xlsx"));
#endif
#ifdef SUPPORT_PPTX
	jsonSummaryFilesByType[L"pptx"] = new JSONValue((double)eBookFolder->GetEBookCount(L".pptx"));
#endif
#ifdef SUPPORT_PDF
	jsonSummaryFilesByType[L"pdf"] = new JSONValue((double)eBookFolder->GetEBookCount(L".pdf"));
#endif
#ifdef SUPPORT_TXT
	jsonSummaryFilesByType[L"txt"] = new JSONValue((double)eBookFolder->GetEBookCount(L".txt"));
#endif
#ifdef SUPPORT_RTF
	jsonSummaryFilesByType[L"rtf"] = new JSONValue((double)eBookFolder->GetEBookCount(L".rtf"));
#endif
#ifdef SUPPORT_JPEG
	jsonSummaryFilesByType[L"jpeg"] = new JSONValue((double)eBookFolder->GetEBookCount(L".jpeg"));
#endif
#ifdef SUPPORT_TIFF
	jsonSummaryFilesByType[L"tiff"] = new JSONValue((double)eBookFolder->GetEBookCount(L".tiff"));
#endif
#ifdef SUPPORT_BMP
	jsonSummaryFilesByType[L"bmp"] = new JSONValue((double)eBookFolder->GetEBookCount(L".bmp"));
#endif

	jsonSummaryFiles[L"CRAWLED"] = new JSONValue((double)eBookFolder->GetEBookCount(NULL));
	jsonSummaryFiles[L"BY TYPE"] = new JSONValue(jsonSummaryFilesByType);

	jsonSummaryTime[L"STARTED"] = new JSONValue(wszStartTime);
	jsonSummaryTime[L"ENDED"] = new JSONValue(wszEndTime);

	jsonSummary[L"FILES"] = new JSONValue(jsonSummaryFiles);
	jsonSummary[L"TIME"] = new JSONValue(jsonSummaryTime);

	jsonRoot[L"CRAWLED FILES"] = jsonFolderReport;
	jsonRoot[L"SUMMARY"] = new JSONValue(jsonSummary);
	jsonRoot[L"TARGET DIRECTORY"] = new JSONValue(argv[1]);

	jsonReport = new JSONValue(jsonRoot);

	wstring wstrJsonReport;
	wstrJsonReport = jsonReport->Stringify().c_str();

	std::ofstream strmOutRep;
	strmOutRep.open(argv[2], std::ios::out | std::ios::binary);
	strmOutRep << ToUtf8(wstrJsonReport);
	strmOutRep.close();

	delete eBookFolder;
	delete jsonReport;

	return 0;
}
