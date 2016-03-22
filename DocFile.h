#pragma once

#include "officefile.h"

class CDocFile :
	public COfficeFile
{
public:
	CDocFile(void);
	~CDocFile(void);

public:
	virtual BOOL Load(TCHAR* wszFilePath);
};
