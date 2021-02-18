#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Layer/Layer.h"

namespace Shark {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void OnAttach() override;
		void OnDetach() override;
		void Begin();
		void End();

		void SetDockSpace(bool enabeld) { m_DockSpace = enabeld; }
		bool DockSpaceEnabeld() const { return m_DockSpace; }
	private:
		bool m_DockSpace = true;
	};

}