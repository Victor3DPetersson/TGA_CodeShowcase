#pragma once

#include "../CommonUtilities/CU/Utility/FixedString.hpp"
#include "../CommonUtilities/CU/Containers/BitFlagpole.hpp"
#include "../CommonUtilities/CU/Containers/Dictionary.h"

#include <Windows.h>

namespace Filebrowser
{
	struct FilesystemEntry
	{
		wchar_t path[MAX_PATH];
		FixedWString256 filename;
		FixedWString256 ext;

		enum Flags
		{
			Flags_None = 0,
			Flags_IsDirectory = PO2(0)
		};
		CU::BitFlagpole8 flags;

		size_t childCount = 0u;
		size_t childDirCount = 0u;
		size_t childrenIndex = size_t(-1);
	};

	struct FilesystemCache
	{
		size_t count = 0U, capacity = 0U;
		FilesystemEntry* entries = 0;
		CU::Dictionary<FixedWString256, size_t> dirLookup;
	};

	extern CU::Dictionary<FixedWString256, FilesystemCache> caches;

	enum Result_
	{
		Result_Fail, Result_Success
	};
	typedef int Result;
	Result BuildFilesystemCache(const FixedWString256& path);
}