// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。
//

#pragma once

#include "targetver.h"

//#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーから使用されていない部分を除外します。
// Windows ヘッダー ファイル:
#include <windows.h>
#include <mbstring.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#include <string>
#include <vector>
#include <stack>

#ifdef _DEBUG
#define LEAK_CHECK
#endif
#ifdef LEAK_CHECK
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

// TODO: プログラムに必要な追加ヘッダーをここで参照してください。
#include "kpi.h"
#include "kpi_decoder.h"
#include "kmp_pi.h"
