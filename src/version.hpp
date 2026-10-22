#pragma once

// Overridden by the build (-DWF_VERSION=...) when building a release.
#if !defined(WF_VERSION)
	#define WF_VERSION "1.0.0"
#endif

// MAJOR * 10000 + MINOR * 100 + PATCH, returned by WF_GetVersion
#if !defined(WF_VERSION_NUMBER)
	#define WF_VERSION_NUMBER 10000
#endif
