// kpidop / kpid2p の各ソースが最初にインクルードするヘッダ

#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
// WIN32_LEAN_AND_MEAN は定義しない (kpi.h が IUnknown を使う)
#include <windows.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "kmp_pi.h"
#include "kpi.h"
#include "kpi_decoder.h"
#include "kpi_impl.h"
#include "kpi_mingw_uuid.h"	// MinGW では __uuidof の GUID を補う
