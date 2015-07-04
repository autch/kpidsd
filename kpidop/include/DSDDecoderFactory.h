#pragma once

#include "CAbstractKpi.h"

CAbstractKpi* CreateKpiDecoderInstance(LPCSTR szFileName);
BOOL GetDSDTagInfo(LPCSTR cszFileName, IKmpTagInfo* pInfo);
