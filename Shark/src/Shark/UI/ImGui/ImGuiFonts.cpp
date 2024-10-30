#include "skpch.h"
#include "ImGuiFonts.h"

#include "Shark/File/FileSystem.h"

namespace Shark::UI {

	static std::unordered_map<std::string, ImFont*> s_Fonts;

	void Fonts::Add(const FontConfiguration& config)
	{
		if (s_Fonts.contains(config.Name))
		{
			SK_CORE_ERROR_TAG("UI", "Tried to add Font with name {} but name is allready used", config.Name);
			return;
		}

		ImFontConfig imguiFontConfig;
		imguiFontConfig.MergeMode = config.MergeWithLast;
		ImGuiIO& io = ImGui::GetIO();
		ImFont* font = io.Fonts->AddFontFromFileTTF(config.Filepath.c_str(), config.Size, &imguiFontConfig, config.GlythRanges);
		SK_CORE_VERIFY(font, "Failed to load font! {}", config.Filepath);
		s_Fonts[config.Name] = font;

		if (config.Default)
			io.FontDefault = font;
	}

	void Fonts::PushDefault()
	{
		auto& io = ImGui::GetIO();
		ImGui::PushFont(io.FontDefault);
	}

	void Fonts::Push(const std::string& name)
	{
		if (!s_Fonts.contains(name))
		{
			auto& io = ImGui::GetIO();
			ImGui::PushFont(io.FontDefault);
			return;
		}

		ImGui::PushFont(s_Fonts.at(name));
	}

	void Fonts::Pop()
	{
		ImGui::PopFont();
	}

	ImFont* Fonts::Get(const std::string& name)
	{
		if (s_Fonts.contains(name))
			return s_Fonts.at(name);

		return ImGui::GetIO().FontDefault;
	}

}
