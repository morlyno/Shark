#pragma once

#include <memory>
#include <utility>
#include <functional>
#include <algorithm>
#include <math.h>

#include <vector>
#include <unordered_map>
#include <string>


#ifdef SK_PLATFORM_WINDOWS
	#ifndef SK_FULL_WINDOWS
		#define SK_RESTRICTED_WINDOWS

		#define WIN32_LEAN_AND_MEAN
		#define NOGDICAPMASKS
		#define NOSYSMETRICS
		#define NOMENUS
		#define NOICONS
		#define NOSYSCOMMANDS
		#define NORASTEROPS
		#define OEMRESOURCE
		#define NOATOM
		#define NOCLIPBOARD
		#define NOCOLOR
		#define NOCTLMGR
		#define NODRAWTEXT
		#define NOKERNEL
		#define NONLS
		#define NOMEMMGR
		#define NOMETAFILE
		#define NOOPENFILE
		#define NOSCROLL
		#define NOSERVICE
		#define NOSOUND
		#define NOTEXTMETRIC
		#define NOWH
		#define NOCOMM
		#define NOKANJI
		#define NOHELP
		#define NOPROFILER
		#define NODEFERWINDOWPOS
		#define NOMCX
		#define NORPC
		#define NOPROXYSTUB
		#define NOIMAGE
		#define NOTAPE
		#define NOMINMAX
	#endif
#include <Windows.h>
#endif