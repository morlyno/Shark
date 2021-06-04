#pragma once

#include "Shark/Render/Material.h"
#include "Shark/Render/ConstantBuffer.h"

namespace Shark {

	struct ConstBuffer
	{
		std::string DebugName;
		std::vector<byte> TempBuffer;
		Ref<ConstantBuffer> ConstantBuffer;
	};

	struct BufferValue
	{
		uint32_t Size;
		uint32_t Offset;
		uint32_t BufferIndex;
	};

	struct Resource
	{
		uint32_t BindPoint;
		Ref<Texture2D> Texture;
	};


	class DirectXMaterial : public Material
	{
	public:
		DirectXMaterial(const Ref<Shaders>& shader, const std::string& name);
		virtual ~DirectXMaterial();

		virtual Ref<Shaders> GetShaders() const override { return m_Shaders; }

		virtual const std::string& GetName() const override { return m_Name; }
		virtual const MaterialDescriptor& GetDescriptor() const override { return m_Descriptor; }

		virtual void SetFlag(MaterialFlag flag, bool enabled) override { m_Flags = m_Flags & ~flag | flag & -(int)enabled; }
		virtual bool IsFalgSet(MaterialFlag flag) override { return m_Flags & flag; }

		virtual void Set(const std::string& name, const void* data, uint32_t size) override;
		virtual void Set(const std::string& name, const Ref<Texture2D>& texture) override;

		virtual void Bind() override;

	private:
		void Reflect();
		
	private:
		Ref<Shaders> m_Shaders;
		std::string m_Name;

		std::vector<ConstBuffer> m_ConstantBuffers;
		std::unordered_map<std::string, BufferValue> m_Values;
		std::unordered_map<std::string, Resource> m_Textures;

		uint32_t m_Flags = 0;

		MaterialDescriptor m_Descriptor;

	};

}
