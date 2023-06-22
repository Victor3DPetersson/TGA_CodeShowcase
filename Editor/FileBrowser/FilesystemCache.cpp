#include "FilesystemCache.h"

#include "../CommonUtilities/CU/Containers/Queue.hpp"
#include <filesystem>
namespace f = std::filesystem;

namespace Filebrowser
{
	int SearchDirectory(const f::path& root, FilesystemCache* result, CU::GrowingQueue<size_t>& directoryQueue, int* dirCountTarget);

	constexpr size_t DEFAULT_CACHE_CAPACITY = 128U;
	CU::Dictionary<FixedWString256, FilesystemCache> caches;

	Result BuildFilesystemCache(const FixedWString256& path)
	{
		f::path root = (const wchar_t*)path;

		if (!f::exists(root))
			return Result_Fail;

		CU::GrowingQueue<size_t> directoryQueue;

		/* Init cache */
		FilesystemCache* result = caches.Get(path);
		if (result)
		{
			result->dirLookup.Clear();
		}
		else
		{
			result = caches.Insert(path, { 0, 0, 0, { } });

			if (!result)
			{
				assert(!"Insertion failed ;)");
				return Result_Fail;
			}

			void* data = malloc(sizeof(FilesystemEntry) * DEFAULT_CACHE_CAPACITY);

			if (!data)
			{
				assert(!"Allocation failed");
				return Result_Fail;
			}

			result->entries = (FilesystemEntry*)data;
			result->capacity = DEFAULT_CACHE_CAPACITY;
		}

		FilesystemEntry* entry = &result->entries[0];
		wcscpy_s(entry->path, MAX_PATH, path);
		entry->filename = f::path(entry->path).filename().replace_extension().c_str();
		entry->flags = FilesystemEntry::Flags_IsDirectory;
		directoryQueue.Enqueue(0);

		result->dirLookup.Insert(path, 0);
		result->count = 1;

		/* Recursive search */
		int subdirCount = 0;
		if (SearchDirectory(root, result, directoryQueue, &subdirCount) == -1)
		{
			return Result_Fail;
		}
		result->entries[0].childDirCount = subdirCount;

		size_t index = size_t(-1);
		while (directoryQueue.GetSize())
		{
			index = directoryQueue.Dequeue();

			result->entries[index].childrenIndex = result->count;
			int rv = SearchDirectory(result->entries[index].path, result, directoryQueue, &subdirCount);
			if (rv == -1)
				return Result_Fail;
			result->entries[index].childCount = rv;
			result->entries[index].childDirCount = subdirCount;

			*result->dirLookup[result->entries[index].path] = index;
		}

		return Result_Success;
	}

	int SearchDirectory(const f::path& root, FilesystemCache* result, CU::GrowingQueue<size_t>& directoryQueue, int* dirCountTarget)
	{
		int childCount = 0;
		int dirCount = 0;

		for (const f::directory_entry& entry :
			f::directory_iterator(root, f::directory_options::skip_permission_denied))
		{
			std::error_code e;
			f::path entryPath = entry.path();

			{
				f::file_status status = entry.status(e);
				if (e)
					continue;
				f::file_type type = status.type();
				if (!(type == f::file_type::directory || type == f::file_type::regular))
				{
					continue;
				}

				if (entryPath.filename().c_str()[0] == L'.' || GetFileAttributesW(entryPath.c_str()) & FILE_ATTRIBUTE_HIDDEN)
					continue;
			}

			/* Grow */
			if (result->count >= result->capacity)
			{
				void* data = realloc(result->entries, result->capacity * 2 * sizeof(FilesystemEntry));
				if (!data)
				{
					assert(!"Allocation failed");
					return -1;
				}
				result->entries = (FilesystemEntry*)data;
				result->capacity *= 2;
			}

			FilesystemEntry* target = &result->entries[result->count++];

			wcscpy_s(target->path, entryPath.c_str());
			target->filename = entryPath.filename().replace_extension().c_str();
			// TODO: to lower
			target->ext = entryPath.extension().c_str();
			target->childCount = 0U;
			target->childrenIndex = size_t(-1);
			target->flags = FilesystemEntry::Flags_None;

			if (entry.is_directory(e) && !e)
			{
				directoryQueue.Enqueue(result->count - 1);

				target->flags.Set(FilesystemEntry::Flags_IsDirectory);
				++dirCount;
			}

			++childCount;
		}

		*dirCountTarget = dirCount;
		return childCount;
	}
}