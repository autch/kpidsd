
#pragma once

#define MAKE_SEMVER(major, minor, patch)	(((major) << 16) | ((minor) << 8) | (patch))
#ifdef _DEBUG
#define DEBUG_MSG		" [DEBUG]"
#else
#define DEBUG_MSG		""
#endif

#define KPI_VERSION		MAKE_SEMVER(1, 0, 0)
#define KPI_DESC		"DSD over PCM plugin for KbMedia Player" DEBUG_MSG
#define KPI_COPYRIGHT	"Copyright(c) 2015, Autch.net"
