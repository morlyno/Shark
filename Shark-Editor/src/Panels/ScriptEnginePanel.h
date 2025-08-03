#pragma once

#include "Shark/UI/TextFilter.h"
#include "Panel.h"
#include <Coral/Assembly.hpp>

namespace Shark {

	class ScriptEnginePanel : public Panel
	{
	public:
		ScriptEnginePanel();

		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnEvent(Event& event) override;

		static const char* GetStaticID() { return "ScriptEnginePanel"; }
		virtual const char* GetPanelID() const override { return GetStaticID(); }
	private:
		void DrawInfo();
		void DrawSettings();
		void DrawScript();
		void ListScripts();

		void DrawTempPanel(bool& shown);
		void DrawAssemblyInfo(const char* internalName, Coral::ManagedAssembly* assembly) const;
	private:
		enum class Context
		{
			Info, Settings, Scripts
		};

	private:
		bool m_Focused = false;

		Context m_Context = Context::Info;
		uint64_t m_SelectedScriptID = 0;
		UI::TextFilter m_Filter;
		bool m_StartSearch = false;

	};

}
