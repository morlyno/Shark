#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/ShaderReflection.h"

namespace Shark {

	class Material : public RefCount
	{
	public:
		virtual ~Material() = default;

		virtual void Prepare() = 0;
		virtual bool Validate() const = 0;

		virtual Ref<Shader> GetShader() const = 0;
		virtual const std::string& GetName() const = 0;
		virtual void SetName(const std::string& name) = 0;

		virtual void Set(const std::string& name, Ref<Texture2D> texture) = 0;
		virtual void Set(const std::string& name, Ref<TextureCube> textureCube) = 0;
		virtual void Set(const std::string& name, Ref<Image2D> image) = 0;

		virtual Ref<Texture2D> GetTexture(const std::string& name) const = 0;
		virtual Ref<TextureCube> GetTextureCube(const std::string& name) const = 0;
		virtual Ref<Image2D> GetImage(const std::string& name) const = 0;

		virtual void Set(const std::string& name, float val) = 0;
		virtual void Set(const std::string& name, const glm::vec2& vec2) = 0;
		virtual void Set(const std::string& name, const glm::vec3& vec3) = 0;
		virtual void Set(const std::string& name, const glm::vec4& vec4) = 0;

		virtual void Set(const std::string& name, int val) = 0;
		virtual void Set(const std::string& name, const glm::ivec2& vec2) = 0;
		virtual void Set(const std::string& name, const glm::ivec3& vec3) = 0;
		virtual void Set(const std::string& name, const glm::ivec4& vec4) = 0;

		virtual void Set(const std::string& name, bool val) = 0;

		virtual void Set(const std::string& name, const glm::mat3& mat3) = 0;
		virtual void Set(const std::string& name, const glm::mat4& mat4) = 0;
		
		virtual float& GetFloat(const std::string& name) = 0;
		virtual glm::vec2& GetVec2(const std::string& name) = 0;
		virtual glm::vec3& GetVec3(const std::string& name) = 0;
		virtual glm::vec4& GetVec4(const std::string& name) = 0;

		virtual int& GetInt(const std::string& name) = 0;
		virtual glm::ivec2& GetInt2(const std::string& name) = 0;
		virtual glm::ivec3& GetInt3(const std::string& name) = 0;
		virtual glm::ivec4& GetInt4(const std::string& name) = 0;

		virtual bool& GetBool(const std::string& name) = 0;

		virtual glm::mat3& GetMat3(const std::string& name) = 0;
		virtual glm::mat4& GetMat4(const std::string& name) = 0;

	public:
		static Ref<Material> Create(Ref<Shader> shader, const std::string& name = {});
	};

}
