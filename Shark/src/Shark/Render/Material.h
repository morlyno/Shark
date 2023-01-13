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
		virtual void SetFloat2(const std::string& name, const glm::vec2& vec2) = 0;
		virtual void SetFloat3(const std::string& name, const glm::vec3& vec3) = 0;
		virtual void SetFloat4(const std::string& name, const glm::vec4& vec4) = 0;

		virtual void SetInt(const std::string& name, int val) = 0;
		virtual void SetInt2(const std::string& name, const glm::ivec2& vec2) = 0;
		virtual void SetInt3(const std::string& name, const glm::ivec3& vec3) = 0;
		virtual void SetInt4(const std::string& name, const glm::ivec4& vec4) = 0;

		virtual void SetBool(const std::string& name, bool val) = 0;

		virtual void SetMat3(const std::string& name, const glm::mat3& mat3) = 0;
		virtual void SetMat4(const std::string& name, const glm::mat4& mat4) = 0;

		virtual void SetTexture(const std::string& name, Ref<Texture2D> texture) = 0;
		virtual void SetTexture(const std::string& name, Ref<Texture2D> texture, uint32_t index) = 0;
		virtual void SetTextureArray(const std::string& name, Ref<Texture2DArray> textureArray) = 0;

		virtual void SetImage(const std::string& name, Ref<Image2D> image, uint32_t binding) = 0;

		virtual float GetFloat(const std::string& name) const = 0;
		virtual const glm::vec2& GetFloat2(const std::string& name) const = 0;
		virtual const glm::vec3& GetFloat3(const std::string& name) const = 0;
		virtual const glm::vec4& GetFloat4(const std::string& name) const = 0;

		virtual int GetInt(const std::string& name, int val) = 0;
		virtual const glm::ivec2& GetInt2(const std::string& name) const = 0;
		virtual const glm::ivec3& GetInt3(const std::string& name) const = 0;
		virtual const glm::ivec4& GetInt4(const std::string& name) const = 0;

		virtual bool GetBool(const std::string& name) const = 0;

		virtual const glm::mat3& GetMat3(const std::string& name) const = 0;
		virtual const glm::mat4& GetMat4(const std::string& name) const = 0;

		virtual Ref<Texture2D> GetTexture(const std::string& name) const = 0;
		virtual Ref<Texture2D> GetTexture(const std::string& name, uint32_t index) const = 0;
		virtual Ref<Texture2DArray> GetTextureArray(const std::string& name) const = 0;

	public:
		static Ref<Material> Create(Ref<Shader> shader);
	};

}
