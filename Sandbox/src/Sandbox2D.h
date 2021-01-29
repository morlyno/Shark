#pragma once

#include <Shark.h>

class Sandbox2D : public Shark::Layer
{
public:
	Sandbox2D(const std::string& name);
	virtual ~Sandbox2D();

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	virtual void OnUpdate(Shark::TimeStep ts) override;
	virtual void OnEvent(Shark::Event& event) override;

	virtual void OnImGuiRender() override;
private:
	bool OnKeyPressedEvent(Shark::KeyPressedEvent event);
private:
	Shark::OrtographicCameraController m_CameraController;

	Shark::Ref<Shark::Texture2D> m_CheckerBoardTexture;
	float rotation = 0.0f;
};
