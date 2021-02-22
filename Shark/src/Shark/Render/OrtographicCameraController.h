#pragma once

#include "Shark/Render/OrtographicCamera.h"

#include "Shark/Core/TimeStep.h"
#include "Shark/Event/Event.h"
#include "Shark/Event/MouseEvent.h"
#include "Shark/Event/WindowEvent.h"

namespace Shark {

	class OrtographicCameraController
	{
	public:
		OrtographicCameraController(float aspectratio, bool doRotation = false);
		OrtographicCamera& GetCamera() { return m_Camera; }

		void OnResize(float width, float height);

		void OnUpdate(TimeStep ts);
		void OnEvent(Event& e);
	private:
		bool OnMouseScrolled(MouseScrolledEvent& event);
		bool OnWindowResized(WindowResizeEvent& event);
	private:
		float m_Aspectratio;
		bool m_DoRotation;

		DirectX::XMFLOAT3 m_Position = { 0.0f, 0.0f, 0.0f };
		float m_MoveSpeed = 1.0f;

		float m_Rotation = 0.0f;
		float m_RotationSpeed = 1.0f;

		float m_ZoomLevel = 1.0f;

		OrtographicCamera m_Camera;
	};

}