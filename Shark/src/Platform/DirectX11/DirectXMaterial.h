#pragma once

#include "Shark/Render/Material.h"

#include "Platform/DirectX11/DirectXShader.h"
#include "Platform/DirectX11/DirectXConstantBuffer.h"
#include "Platform/DirectX11/DirectXTexture.h"

//#include "Shark/Core/Buffer.h"

namespace Shark {

	class DirectXMaterial : public Material
	{
		struct CBVar
		{
			uint32_t BufferSlot;
			uint32_t Offset;
			uint32_t Size;
		};

	public:
		DirectXMaterial(Ref<Shader> shader);
		virtual ~DirectXMaterial();

		virtual void SetTexture(const std::string& name, Ref<Texture2D> texture) override;
		virtual void SetTexture(const std::string& name, Ref<Texture2D> texture, uint32_t index) override;
		virtual void SetTextureArray(const std::string& name, Ref<Texture2DArray> textureArray) override;


		void SetFloat(const std::string& name, float val) override;
		void SetFloat2(const std::string& name, const DirectX::XMFLOAT2& vec2) override;
		void SetFloat3(const std::string& name, const DirectX::XMFLOAT3& vec3) override;
		void SetFloat4(const std::string& name, const DirectX::XMFLOAT4& vec4) override;

		void SetInt(const std::string& name, int val) override;
		void SetInt2(const std::string& name, const DirectX::XMINT2& vec2) override;
		void SetInt3(const std::string& name, const DirectX::XMINT3& vec3) override;
		void SetInt4(const std::string& name, const DirectX::XMINT4& vec4) override;

		void SetBool(const std::string& name, bool val) override;

		void SetMat3(const std::string& name, const DirectX::XMFLOAT3X3& mat3) override;
		void SetMat4(const std::string& name, const DirectX::XMFLOAT4X4& mat4) override;
		void SetMat4(const std::string& name, const DirectX::XMMATRIX& mat4) override;

	private:
		void SetBytes(const std::string& name, byte* data, uint32_t size);

	private:
		void Reflect();

	private:
		Ref<DirectXShader> m_Shader;

		Ref<DirectXConstantBufferSet> m_ConstnatBufferSet;
		//std::unordered_map<uint32_t, Buffer> m_ConstantBuffers;
		std::unordered_map<std::string, CBVar> m_VariableMap;

		std::unordered_map<std::string, Ref<DirectXTexture2DArray>> m_ResourceMap;

		friend class DirectXRenderer;
	};

}
