#pragma once

#include "Shark/Render/Material.h"

#include "Platform/DirectX11/DirectXShader.h"
#include "Platform/DirectX11/DirectXConstantBuffer.h"
#include "Platform/DirectX11/DirectXTexture.h"

#include "Shark/Core/Buffer.h"

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


		virtual void SetFloat(const std::string& name, float val) override { SetBytes(name, (byte*)&val, sizeof(val)); }
		virtual void SetFloat2(const std::string& name, const glm::vec2& vec2) override { SetBytes(name, (byte*)&vec2, sizeof(vec2)); }
		virtual void SetFloat3(const std::string& name, const glm::vec3& vec3) override { SetBytes(name, (byte*)&vec3, sizeof(vec3)); }
		virtual void SetFloat4(const std::string& name, const glm::vec4& vec4) override { SetBytes(name, (byte*)&vec4, sizeof(vec4)); }

		virtual void SetInt(const std::string& name, int val) override { SetBytes(name, (byte*)&val, sizeof(val)); }
		virtual void SetInt2(const std::string& name, const glm::ivec2& vec2) override { SetBytes(name, (byte*)&vec2, sizeof(vec2)); }
		virtual void SetInt3(const std::string& name, const glm::ivec3& vec3) override { SetBytes(name, (byte*)&vec3, sizeof(vec3)); }
		virtual void SetInt4(const std::string& name, const glm::ivec4& vec4) override { SetBytes(name, (byte*)&vec4, sizeof(vec4)); }

		virtual void SetBool(const std::string& name, bool val) override { SetBytes(name, (byte*)&val, sizeof(val)); }

		virtual void SetMat3(const std::string& name, const glm::mat3& mat3) override { SetBytes(name, (byte*)&mat3, sizeof(mat3)); }
		virtual void SetMat4(const std::string& name, const glm::mat4& mat4) override { SetBytes(name, (byte*)&mat4, sizeof(mat4)); }


		virtual float GetFloat(const std::string& name) const override { return *(float*)GetBytes(name); }
		virtual const glm::vec2& GetFloat2(const std::string& name) const override { return *(glm::vec2*)GetBytes(name); }
		virtual const glm::vec3& GetFloat3(const std::string& name) const override { return *(glm::vec3*)GetBytes(name); }
		virtual const glm::vec4& GetFloat4(const std::string& name) const override { return *(glm::vec4*)GetBytes(name); }

		virtual int GetInt(const std::string& name, int val) override { return *(int*)GetBytes(name); }
		virtual const glm::ivec2& GetInt2(const std::string& name) const override { return *(glm::ivec2*)GetBytes(name); }
		virtual const glm::ivec3& GetInt3(const std::string& name) const override { return *(glm::ivec3*)GetBytes(name); }
		virtual const glm::ivec4& GetInt4(const std::string& name) const override { return *(glm::ivec4*)GetBytes(name); }

		virtual bool GetBool(const std::string& name) const override { return *(bool*)GetBytes(name); }

		virtual const glm::mat3& GetMat3(const std::string& name) const override { return *(glm::mat3*)GetBytes(name); }
		virtual const glm::mat4& GetMat4(const std::string& name) const override { return *(glm::mat4*)GetBytes(name); }


		virtual Ref<Texture2D> GetTexture(const std::string& name) const override { return m_ResourceMap.at(name)->Get(0); }
		virtual Ref<Texture2D> GetTexture(const std::string& name, uint32_t index) const override { return m_ResourceMap.at(name)->Get(index); }
		virtual Ref<Texture2DArray> GetTextureArray(const std::string& name) const override { return m_ResourceMap.at(name); }

	private:
		void SetBytes(const std::string& name, byte* data, uint32_t size);
		byte* GetBytes(const std::string& name) const;

	private:
		void Reflect();

	private:
		Ref<DirectXShader> m_Shader;

		Ref<DirectXConstantBufferSet> m_ConstnatBufferSet;
		std::unordered_map<uint32_t, ScopedBuffer> m_ConstantBufferData;
		std::unordered_map<std::string, CBVar> m_VariableMap;

		std::unordered_map<std::string, Ref<DirectXTexture2DArray>> m_ResourceMap;

		friend class DirectXRenderer;
	};

}
