#include "FileBrowser.h"

/* ImGui */
#include "../Externals/imgui/imgui.h"

/* Utils */
#include "../CommonUtilities/CU/Containers/Dictionary.h"

/* STL */
#include <filesystem>
#include <LevelState\LevelState.h>
#include <FileBrowser\FilesystemCache.h>
namespace f = std::filesystem;

CU::Dictionary<int, Filebrowser::FBResult> fbDict;

void TreeNodeCommon(bool isCwd, FixedWString256& cwd, const FixedWString256& pstr)
{
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
	{
		cwd = pstr;
	}

	if (isCwd)
	{
		ImGui::PopStyleColor();
	}
}

void DirListIt(FixedWString256 dir, FixedWString256& cwd, Filebrowser::FBResult* res)
{
	Filebrowser::FilesystemCache* cache = Filebrowser::caches[res->root];
	size_t dirIndex = *cache->dirLookup[dir];
	Filebrowser::FilesystemEntry* e = &cache->entries[dirIndex];
	size_t dirMin = e->childrenIndex;
	size_t dirMax = e->childrenIndex + e->childCount;

	for (size_t file = dirMin; file < dirMax; ++file)
	{
		Filebrowser::FilesystemEntry* entry = &cache->entries[file];

		if (entry->flags.Test(Filebrowser::FilesystemEntry::Flags_IsDirectory))
		{
			bool containsSubDirs = entry->childDirCount > 0;

			bool isCwd = (wcscmp(cwd, entry->path) == 0);
			if (isCwd)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, 0x7c7c7c7c);
			}

			FixedString256 name = FixedString256::Format(
				isCwd ? ICON_FA_FOLDER_OPEN " %S###%S" :
				ICON_FA_FOLDER " %S###%S"
				, entry->filename + entry->ext, entry->path);
			if (ImGui::TreeNodeEx(name, (!containsSubDirs ? ImGuiTreeNodeFlags_Leaf : 0)))
			{
				TreeNodeCommon(isCwd, cwd, entry->path);

				DirListIt(entry->path, cwd, res);

				ImGui::TreePop();
			}
			else
			{
				TreeNodeCommon(isCwd, cwd, entry->path);
			}
		}
	}
}

namespace Filebrowser
{
	FBResult Update(int id, FixedWString256 cwd, const VictorOnCrack<FixedWString256, 16, size_t>& filter)
	{
		FixedString256 strid = FixedString256::Format("fb child %i", id);
		ImVec2 wndSize;

		if (!fbDict.Contains(id))
		{
			fbDict.Insert(id, { L"", L"", cwd, L"", false, cwd });
		}
		cwd = fbDict[id]->cwd;
		{
			Filebrowser::FilesystemCache* cache = Filebrowser::caches.Get(fbDict[id]->root);
			if (!cache)
			{
				BuildFilesystemCache(fbDict[id]->root);
			}
		}

		FBResult* dictEntry = fbDict[id];
		dictEntry->doubleClicked = L"";

		wndSize = ImGui::GetWindowSize();
		wndSize.x *= .2f;
		wndSize.y -= 125.0f;

		/* Controls */
		{
			if (ImGui::Button(ICON_FA_SYNC))
			{
				BuildFilesystemCache(fbDict[id]->root);
			}
			ImGui::SameLine();

			bool isCwd = dictEntry->cwd == dictEntry->root;
			if (isCwd) ImGui::BeginDisabled();
			if (ImGui::Button(ICON_FA_ARROW_UP))
			{
				cwd = f::path((const wchar_t*)cwd).parent_path().c_str();
			}
			if (isCwd) ImGui::EndDisabled();
		}

		/* Directory Hierarchy*/
		{
			ImGui::BeginChild(strid, wndSize, true, ImGuiWindowFlags_NoTitleBar);

			ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
			const FixedWString256& root = fbDict[id]->root;
			bool isCwd = cwd == root;
			if (isCwd)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, 0x7c7c7c7c);
			}
			if (ImGui::TreeNode(FixedString256::Format(ICON_FA_FOLDER " %S", f::path((const wchar_t*)root).filename().c_str())))
			{
				TreeNodeCommon(isCwd, cwd, root);
				DirListIt(root, cwd, dictEntry);

				ImGui::TreePop();
			}
			else TreeNodeCommon(isCwd, cwd, root);

			ImGui::EndChild();
		}

		/* Files */
		{
			ImGui::SameLine();
			ImGui::BeginChild(strid + "files", { 0, wndSize.y }, true, ImGuiWindowFlags_NoTitleBar);

			ImGui::Columns(CU::Max(1, int((ImGui::GetContentRegionAvail().x) / 76.0f)), 0, false);

			Filebrowser::FilesystemCache* cache = Filebrowser::caches.Get(fbDict[id]->root);
			size_t* dirIndexPtr = cache->dirLookup.Get(cwd);
			assert(dirIndexPtr);
			size_t dirIndex;
			if (dirIndexPtr)
				dirIndex = *dirIndexPtr;
			else dirIndex = 0;
			size_t dirChildMax = cache->entries[dirIndex].childrenIndex + cache->entries[dirIndex].childCount;

			for (size_t entry = cache->entries[dirIndex].childrenIndex; entry < dirChildMax; ++entry)
			{
				Filebrowser::FilesystemEntry* file = &cache->entries[entry];

				void* srv;

				if (file->flags.Test(Filebrowser::FilesystemEntry::Flags_IsDirectory))
				{
					srv = levelState.folderSolidSRV;
				}
				else
				{
					if (filter.Size())
					{
						bool isInFilters = false;
						for (size_t i = 0U; i < filter.Size(); ++i)
						{
							if (wcscmp(file->ext, filter[i]) == 0)
							{
								isInFilters = true;
								break;
							}
						}
						if (!isInFilters)
							continue;
					}

					auto& ext = file->ext;
					if (ext == FixedWString256(L".dds"))
					{
						srv = EngineInterface::GetTextureManager()
							->GetTextureObject(ShortString(FixedString256::Format("%S", file->path)));
					}
					else if (ext == FixedWString256(L".grat"))
					{
						srv = levelState.artstationBrandsSRV;
					}
					else if (ext == FixedWString256(L".gratmotorik"))
					{
						srv = levelState.runningSolidSRV;
					}
					else
					{
						srv = levelState.fileSolidSRV;
					}
				}

				ImVec4 hoverColor = { 0.482352f,0.7019607f, .97f, 1.0f };
				const auto wpath = (FixedWString256)file->path;
				bool hovered = dictEntry->hovered == wpath;
				const ImVec4 selColor = { 0.1054f, 0.4804f, 0.972f, 1.0f };
				const bool selected = dictEntry->selected == wpath;
				bool dclicked = false;

				if (selected)
				{
					hovered = selected;
					hoverColor = selColor;
				}

				ImGui::Image(ImTextureID(srv), { 64.0f, 64.0f }, { 0,0 }, { 1,1 }, (hovered ? hoverColor : ImVec4{ 1,1,1,1 }));
				if (ImGui::IsItemHovered())
					dictEntry->hovered = wpath;
				if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
				{
					dictEntry->selected = wpath;
					dclicked |= ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
				}
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
				{
					if (file->ext == FixedWString256(L".grat"))
					{
						ImGui::SetDragDropPayload("MODELID", file->filename, sizeof(FixedWString256));
						ImGui::Image(ImTextureID(srv), { 16.0f, 16.0f });
					}
					else if (file->ext == FixedWString256(L".gratmotorik"))
					{
						ImGui::SetDragDropPayload("ANIMMODELID", file->filename, sizeof(FixedWString256));
						ImGui::Image(ImTextureID(srv), { 16.0f, 16.0f });
					}
					else if (file->ext == FixedWString256(L".gratplat"))
					{
						ImGui::SetDragDropPayload("MATERIALID", file->filename, sizeof(FixedWString256));
						ImGui::Image(ImTextureID(srv), { 16.0f, 16.0f });
					}
					else if (file->ext == FixedWString256(L".gratsprut"))
					{
						ImGui::SetDragDropPayload("PARTICLEID", file->filename, sizeof(FixedWString256));
						ImGui::Image(ImTextureID(srv), { 16.0f, 16.0f });
					}
					else if (file->ext == FixedWString256(L".dds"))
					{
						ImGui::SetDragDropPayload("DDS", file->path, sizeof(FixedWString256));
						ImGui::Image(ImTextureID(srv), { 16.0f, 16.0f });
					}
					ImGui::EndDragDropSource();
				}

				if (hovered)
					ImGui::PushStyleColor(ImGuiCol_Text, hoverColor);
				ImGui::TextWrapped(FixedString256::Format("%S", file->filename + file->ext));
				if (hovered)
					ImGui::PopStyleColor();
				if (ImGui::IsItemHovered())
					dictEntry->hovered = wpath;
				if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
				{
					dictEntry->selected = wpath;
					dclicked |= ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
				}

				if (dclicked)
				{
					if (file->flags.Test(Filebrowser::FilesystemEntry::Flags_IsDirectory))
					{
						cwd = wpath;
					}
					else
					{
						dictEntry->doubleClicked = wpath;
					}
				}

				ImGui::NextColumn();
			}

			ImGui::EndChild();
		}

		fbDict[id]->cwd = cwd;
		return *fbDict[id];
	}
	FBResult UpdateModal(FixedString256 title, int id, FixedWString256 cwd,
		const VictorOnCrack<FixedWString256, 16, size_t>& filter, const SIMDString32& okayLabel)
	{
		bool open = true;
		FBResult returnValue, res;

		ImGui::PushID(id);

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, { .5f, .5f });
		ImGui::SetNextWindowSize({ 1250.0f, 450.0f }, ImGuiCond_Appearing);
		if (ImGui::BeginPopupModal(title, &open))
		{
			res = Update(id, cwd, filter);

			open = !res.close;

			returnValue.selected = res.selected;
			returnValue.doubleClicked = res.doubleClicked;
			returnValue.cwd = res.cwd;

			ImGui::Separator();

			if (ImGui::Button(okayLabel))
			{
				returnValue.doubleClicked = res.selected;
				open = false;
			}
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_WINDOW_CLOSE " Cancel"))
			{
				open = false;
			}

			if (returnValue.doubleClicked.Data()[0])
			{
				open = false;
			}

			if (!open)
			{
				fbDict.Remove(id);
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();

		return { returnValue.selected, returnValue.doubleClicked, returnValue.cwd, returnValue.hovered, !open };
	}
	void PushModal(FixedString256 title, int id)
	{
		ImGui::PushID(id);

		ImGui::OpenPopup(title);

		ImGui::PopID();
	}
}