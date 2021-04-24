
#include <Shark/Scean/NativeScriptFactory.h>
#include <Shark/Scean/Components/Components.h>
#include <Shark/Core/Input.h>
#include <Shark/Core/Random.h>
#include <Shark/Event/KeyEvent.h>

SK_SCRIPT_CLASS(TestScript)
{
public:
	virtual void OnCreate() override
	{
		SK_CORE_TRACE("Test Script OnCreate");
	}

	virtual void OnDestroy() override
	{
		SK_CORE_TRACE("Test Script OnDestroy");
	}

	virtual void OnUpdate(Shark::TimeStep ts)
	{
		SK_CORE_TRACE("Test Script OnUpdate");
	}

};

SK_SCRIPT_CLASS(PlayerScript)
{
public:
	float m_MoveForce;
	float m_JumpForce;

	bool m_CameraAtached = false;
	DirectX::XMFLOAT4 m_Color;

public:
	virtual void OnCreate() override
	{
		if (!m_Entity.HasComponent<Shark::TransformComponent>())
			m_Entity.AddComponent<Shark::TransformComponent>();
		if (!m_Entity.HasComponent<Shark::RigidBodyComponent>())
		{
			auto& body = m_Entity.AddComponent<Shark::RigidBodyComponent>().Body;
			body.SetType(Shark::BodyType::Dynamic);
		}
		if (!m_Entity.HasComponent<Shark::BoxColliderComponent>())
		{
			auto& collider = m_Entity.AddComponent<Shark::BoxColliderComponent>().Collider;
			Shark::ColliderSpecs cs = collider.GetCurrentState();
			cs.Friction = 0.3f;
			cs.Density = cs.Width * cs.Height;
			collider.SetState(cs);
		}

		m_MoveForce = 25.0f;
		m_JumpForce = 500.0f;

		m_Color = m_Entity.GetComponent<Shark::SpriteRendererComponent>().Color;
	}

	virtual void OnUpdate(Shark::TimeStep ts) override
	{
		auto& rb = m_Entity.GetComponent<Shark::RigidBodyComponent>();
		auto& collider = m_Entity.GetComponent<Shark::BoxColliderComponent>().Collider;
		auto& sr = m_Entity.GetComponent<Shark::SpriteRendererComponent>();

		DirectX::XMFLOAT2 dir = { 0.0f, 0.0f };
		if (Shark::Input::KeyPressed(Shark::Key::A))
			dir.x -= m_MoveForce;
		if (Shark::Input::KeyPressed(Shark::Key::D))
			dir.x += m_MoveForce;

		if (dir.x != 0 || dir.y != 0)
			rb.Body.AplyForce(dir, true);

		sr.Color = m_Color;
		if (collider.IsColliding())
			sr.Color = { 1.0f, 1.0f, 0.0f, 1.0f };

		if (m_CameraAtached)
		{
			Shark::Entity camera = m_Scean->GetActiveCamera();
			auto& tf = camera.GetComponent<Shark::TransformComponent>();
			auto [x, y] = rb.Body.GetPosition();
			tf.Position.x = x;
			tf.Position.y = y;
		}

	}

	virtual void OnEvent(Shark::Event& event) override
	{
		Shark::EventDispacher dispacher(event);
		dispacher.DispachEvent<Shark::KeyPressedEvent>(SK_BIND_EVENT_FN(PlayerScript::OnKeyPressed));
	}

	bool OnKeyPressed(Shark::KeyPressedEvent& event)
	{
		switch (event.GetKeyCode())
		{
			case Shark::Key::Space:
			{
				auto& rb = m_Entity.GetComponent<Shark::RigidBodyComponent>();
				rb.Body.AplyForce({ 0.0f, m_JumpForce }, true);
				return true;
			}
			case Shark::Key::C:
			{
				m_CameraAtached = !m_CameraAtached;
				return true;
			}
		}
		return false;
	}

};


SK_SCRIPT_CLASS(CameraScript)
{
public:
	float m_MoveSpeed;
	float m_CameraSpeed;


public:
	virtual void OnCreate() override
	{
		m_MoveSpeed = 4.0f;
		m_CameraSpeed = 1.0f;

		m_Scean->SetActiveCamera(m_Entity);

	}

	virtual void OnUpdate(Shark::TimeStep ts) override
	{
		Shark::TransformComponent& tf = m_Entity.GetComponent<Shark::TransformComponent>();

		if (Shark::Input::KeyPressed(Shark::Key::W))
			tf.Position.z += m_MoveSpeed * ts;
		if (Shark::Input::KeyPressed(Shark::Key::S))
			tf.Position.z -= m_MoveSpeed * ts;
		if (Shark::Input::KeyPressed(Shark::Key::A))
			tf.Position.x -= m_MoveSpeed * ts;
		if (Shark::Input::KeyPressed(Shark::Key::D))
			tf.Position.x += m_MoveSpeed * ts;
		if (Shark::Input::KeyPressed(Shark::Key::LeftShift))
			tf.Position.y -= m_MoveSpeed * ts;
		if (Shark::Input::KeyPressed(Shark::Key::Space))
			tf.Position.y += m_MoveSpeed * ts;

		if (Shark::Input::KeyPressed(Shark::Key::Up))
			tf.Rotation.x -= m_CameraSpeed * ts;
		if (Shark::Input::KeyPressed(Shark::Key::Down))
			tf.Rotation.x += m_CameraSpeed * ts;
		if (Shark::Input::KeyPressed(Shark::Key::Left))
			tf.Rotation.y -= m_CameraSpeed * ts;
		if (Shark::Input::KeyPressed(Shark::Key::Right))
			tf.Rotation.y += m_CameraSpeed * ts;


	}

	virtual void OnEvent(Shark::Event& event) override
	{
		if (event.GetEventType() == Shark::EventTypes::KeyPressed)
		{
			auto& kpe = static_cast<Shark::KeyPressedEvent&>(event);
			if (kpe.GetKeyCode() == Shark::Key::N)
			{
				Shark::Entity entity = m_Entity.GetScean()->CreateEntity();
				auto& sr = entity.AddComponent<Shark::SpriteRendererComponent>();
				sr.Color = { Shark::Random::Float(), Shark::Random::Float(), Shark::Random::Float(), 1.0f };
				auto& tf = entity.GetComponent<Shark::TransformComponent>();
				tf.Position = { Shark::Random::Float() * 20 - 10, Shark::Random::Float() * 20 - 10, Shark::Random::Float() * 20 - 10 };
			}
		}
	}

};
