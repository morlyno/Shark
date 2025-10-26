#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/ShaderInputManager.h"

namespace Shark {

	class Material : public RefCount
	{
	public:
		static Ref<Material> Create(Ref<Shader> shader, const std::string& name = {}) { return Ref<Material>::Create(shader, name); }

	public:
		void Prepare();
		bool Validate() const;

		Ref<Shader> GetShader() const { return m_Shader; }
		const std::string& GetName() const { return m_Name; }
		void SetName(const std::string& name) { m_Name = name; }
		const ShaderInputManager& GetInputManager() const { return m_InputManager; }

		void Set(const std::string& name, Ref<Image2D> image, uint32_t arrayIndex = 0);
		void Set(const std::string& name, Ref<Texture2D> texture, uint32_t arrayIndex = 0);
		void Set(const std::string& name, Ref<TextureCube> textureCube, uint32_t arrayIndex = 0);
		void Set(const std::string& name, Ref<Sampler> sampler, uint32_t arrayIndex = 0);

		void Set(const std::string& name, const Buffer data);

	public:
		Material(Ref<Shader> shader, const std::string& name);
		~Material();

	private:
		Ref<Shader> m_Shader;
		ShaderInputManager m_InputManager;
		std::unordered_map<std::string, Ref<ConstantBuffer>> m_ConstantBuffers;

		std::string m_Name;
	};

}
