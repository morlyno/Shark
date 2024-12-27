#pragma once

#include "Shark/Core/Base.h"
#include <Coral/ManagedObject.hpp>

namespace Shark {

	enum class ManagedFieldType : uint16_t
	{
		None = 0,

		Bool, // Coral::Bool32
		Byte,
		SByte,
		Short,
		UShort,
		Int,
		UInt,
		Long,
		ULong,
		Float,
		Double,

		Vector2,
		Vector3,
		Vector4,

		String, // Coral::String, Coral::ScopedString / NativeString
		Entity, // UUID, uint64_t
		Prefab, // AssetHandle
	};

	inline uint64_t GetDataTypeSize(ManagedFieldType type)
	{
		switch (type)
		{
			case ManagedFieldType::Bool: return sizeof(Coral::Bool32);
			case ManagedFieldType::Byte: return sizeof(int8_t);
			case ManagedFieldType::SByte: return sizeof(uint8_t);
			case ManagedFieldType::Short: return sizeof(int16_t);
			case ManagedFieldType::UShort: return sizeof(uint16_t);
			case ManagedFieldType::Int: return sizeof(int32_t);
			case ManagedFieldType::UInt: return sizeof(uint32_t);
			case ManagedFieldType::Long: return sizeof(int64_t);
			case ManagedFieldType::ULong: return sizeof(uint64_t);
			case ManagedFieldType::Float: return sizeof(float);
			case ManagedFieldType::Double: return sizeof(double);
			case ManagedFieldType::Vector2: return sizeof(float) * 2;
			case ManagedFieldType::Vector3: return sizeof(float) * 3;
			case ManagedFieldType::Vector4: return sizeof(float) * 4;
			case ManagedFieldType::String: return sizeof(Coral::String);
			case ManagedFieldType::Entity: return sizeof(UUID);
		}
		return 0;
	}

	struct FieldMetadata
	{
		std::string Name;
		Coral::Type* Type;
		ManagedFieldType DataType = ManagedFieldType::None;

		Buffer DefaultValue;
	};

	struct ScriptMetadata
	{
		std::string FullName;
		Coral::Type* Type;

		std::unordered_map<uint64_t, FieldMetadata> Fields;
	};

	class FieldStorage
	{
	public:
		const std::string& GetName() const { return m_Name; }
		ManagedFieldType GetDataType() const { return m_DataType; }

		template<typename T>
		T GetValue() const
		{
			if (m_Instance)
				return m_Instance->GetFieldValue<T>(m_Name);
			else
				return m_ValueBuffer.Value<T>();
		}

		template<typename T>
		void SetValue(const T& value)
		{
			if (m_Instance)
				m_Instance->SetFieldValue(m_Name, value);
			else
				m_ValueBuffer.Write(&value, sizeof(value));
		}

		template<>
		std::string GetValue<std::string>() const
		{
			if (m_Instance)
				return m_Instance->GetFieldValue<std::string>(m_Name);
			else
				return std::string(m_ValueBuffer.As<const char>(), m_ValueBuffer.Size);
		}

		template<>
		void SetValue(const std::string& value)
		{
			if (m_Instance)
				m_Instance->SetFieldValue(m_Name, value);
			else
			{
				m_ValueBuffer.Allocate(value.size() + 1);
				m_ValueBuffer.Write(value.data(), value.size() + 1);
			}
		}

	public:
		FieldStorage() = default;
		~FieldStorage() { m_ValueBuffer.Release(); }

	private:
		std::string m_Name;
		ManagedFieldType m_DataType = ManagedFieldType::None;
		Coral::ManagedObject* m_Instance = nullptr;

		Buffer m_ValueBuffer;

		friend class ScriptEngine;
		friend class ScriptStorage;
		friend class SceneSerializer;
	};

}
