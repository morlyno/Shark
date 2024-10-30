#pragma once

#include <imgui.h>

namespace Shark::UI {

	struct FontConfiguration
	{
		std::string Name;
		std::string Filepath;
		float Size;
		const ImWchar* GlythRanges = nullptr;
		bool MergeWithLast = false;
		bool Default = false;
	};

	class Fonts
	{
	public:
		static void Add(const FontConfiguration& config);
		static void PushDefault();
		static void Push(const std::string& name);
		static void Pop();
		static ImFont* Get(const std::string& name);
	};

}
