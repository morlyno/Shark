#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/Texture.h"

namespace Shark {

	class Material : public RefCount
	{
	public:
		virtual ~Material() = default;

		virtual void SetFloat(const std::string& name, float val) = 0;
		virtual void SetFloat2(const std::string& name, const DirectX::XMFLOAT2& vec2) = 0;
		virtual void SetFloat3(const std::string& name, const DirectX::XMFLOAT3& vec3) = 0;
		virtual void SetFloat4(const std::string& name, const DirectX::XMFLOAT4& vec4) = 0;

		virtual void SetInt(const std::string& name, int val) = 0;
		virtual void SetInt2(const std::string& name, const DirectX::XMINT2& vec2) = 0;
		virtual void SetInt3(const std::string& name, const DirectX::XMINT3& vec3) = 0;
		virtual void SetInt4(const std::string& name, const DirectX::XMINT4& vec4) = 0;

		virtual void SetBool(const std::string& name, bool val) = 0;

		virtual void SetMat3(const std::string& name, const DirectX::XMFLOAT3X3& mat3) = 0;
		virtual void SetMat4(const std::string& name, const DirectX::XMFLOAT4X4& mat4) = 0;
		virtual void SetMat4(const std::string& name, const DirectX::XMMATRIX& mat4) = 0;

		virtual void SetTexture(const std::string& name, Ref<Texture2D> texture) = 0;
		virtual void SetTexture(const std::string& name, Ref<Texture2D> texture, uint32_t index) = 0;
		virtual void SetTextureArray(const std::string& name, Ref<Texture2DArray> textureArray) = 0;

	public:
		static Ref<Material> Create(Ref<Shader> shader);
	};

}
