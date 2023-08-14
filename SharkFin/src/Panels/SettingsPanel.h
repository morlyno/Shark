
#include "Shark/Editor/Panel.h"

namespace Shark {

	class SettingsPanel : public Panel
	{
	public:
		SettingsPanel(const std::string& panelName);
		virtual ~SettingsPanel();

		virtual void OnImGuiRender(bool& shown) override;

		template<typename TFunc>
		void AddNode(const TFunc& func)
		{
			m_Nodes.emplace_back(func);
		}

		static Ref<SettingsPanel> Get();

	private:
		std::vector<std::function<void()>> m_Nodes;
		//std::map<std::string, std::function<void()>> m_Nodes;
	};

}
