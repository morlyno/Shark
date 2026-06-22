#pragma once

#include "NodeGraph/EditorNodes.h"

namespace Shark {

	namespace GraphEditor {

		class Factory
		{
		public:
			void AddNode(const std::string& category, const std::string& type, std::function<Node*()> spawner);
			const auto& GetRegistry() const { return m_Registry; }

			Node* SpawnNode(std::string_view category, std::string_view type);

		private:
			std::map<std::string, std::map<std::string, std::function<Node* ()>, std::ranges::less>, std::ranges::less> m_Registry;
		};

	}

}
