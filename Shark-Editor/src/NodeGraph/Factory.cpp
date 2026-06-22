#include "skpch.h"
#include "Factory.h"

namespace Shark {

	void GraphEditor::Factory::AddNode(const std::string& category, const std::string& type, std::function<Node*()> spawner)
	{
		auto& cat = m_Registry[category];
		SK_CORE_VERIFY(cat.contains(type) == false);

		cat[type] = std::move(spawner);
	}

	GraphEditor::Node* GraphEditor::Factory::SpawnNode(std::string_view category, std::string_view type)
	{
		const auto cat = m_Registry.find(category);
		if (cat != m_Registry.end())
		{
			const auto spawner = cat->second.find(type);
			if (spawner != cat->second.end())
			{
				return spawner->second();
			}
		}
		return nullptr;
	}

}
