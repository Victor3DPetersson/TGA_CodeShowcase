#pragma once
#include "..\CommonUtilities\CU\Containers\GrowingArray.hpp"
#include <filesystem>

namespace Utility
{
	class DragNDropHandler
	{
	public:
		DragNDropHandler()
		{
			myEvents.Init(10);
		}
		void Subscribe(std::function<void(std::filesystem::path)> aFunction) { myEvents.Add(aFunction); }
		void Callback(std::filesystem::path aValue)
		{
			for (std::function<void(std::filesystem::path)>& func : myEvents)
			{
				func(aValue);
			}
		}

	private:
		CU::GrowingArray<std::function<void(std::filesystem::path)>> myEvents;
	};

}
