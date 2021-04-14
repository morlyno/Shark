#pragma once

#include <box2d/box2d.h>
#include <DirectXMath.h>

namespace Shark {

	enum class ShapeType { Box };

	struct ColliderSpecs
	{
		ShapeType Shape = ShapeType::Box;

		float Friction = 0.0f;
		float Density = 0.0f;
		float Restitution = 0.0f;

		DirectX::XMFLOAT2 Center = { 0.0f, 0.0f };
		float Rotation = 0.0f;

		float Width = 0.0f, Height = 0.0f;
	};

	class BoxCollider
	{
	public:
		BoxCollider() = default;
		BoxCollider(b2Fixture* fixture, uint32_t index, const ColliderSpecs& specs);

		BoxCollider(const BoxCollider&) = default;
		BoxCollider& operator=(const BoxCollider&) = default;

		BoxCollider(BoxCollider&&) = default;
		BoxCollider& operator=(BoxCollider&&) = default;

		void Resize(float width, float height);
		DirectX::XMFLOAT2 GetSize() const { return { m_Width, m_Height }; }

		void SetTransform(const DirectX::XMFLOAT2& center, float rotation);
		void SetCenter(const DirectX::XMFLOAT2& center) { SetTransform(center, m_Rotation); }
		void SetRotation(float rotation) { SetTransform(m_Center, rotation); }
		DirectX::XMFLOAT2 GetCenter() const { return m_Center; }
		float GetRotation() const { return m_Rotation; }

		float GetFriction() const { return m_Fixture->GetFriction(); }
		float GetDensity() const { return m_Fixture->GetDensity(); }
		float GetRestituion() const { return m_Fixture->GetRestitution(); }

		void SetFriction(float friction) { m_Fixture->SetFriction(friction); }
		void SetDensity(float density) { m_Fixture->SetDensity(density); m_Fixture->GetBody()->ResetMassData(); }
		void SetRestitution(float restituion) { m_Fixture->SetRestitution(restituion); }

		ShapeType GetShape() const { return m_Shape; }
		void SetShape(...) { SK_CORE_ASSERT(false, "Not Implemented"); }

		ColliderSpecs GetCurrentState() const;
		void SetState(const ColliderSpecs& specs);

		operator b2Fixture* () const { return m_Fixture; }
	private:
		b2Fixture* m_Fixture = nullptr;

		ShapeType m_Shape;
		float m_Width, m_Height;
		DirectX::XMFLOAT2 m_Center;
		float m_Rotation;

		uint32_t m_ColliderIndex = 0;
	};

}
