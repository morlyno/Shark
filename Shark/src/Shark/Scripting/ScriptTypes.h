#pragma once

#include "Shark/Core/Base.h"

extern "C" {
	typedef struct _MonoAssembly MonoAssembly;
	typedef struct _MonoImage MonoImage;
	typedef struct _MonoType MonoType;
	typedef struct _MonoClass MonoClass;
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoString MonoString;
	typedef struct _MonoClassField MonoClassField;
}

namespace Shark {

	class Entity;

	struct Accessibility
	{
		enum Type : uint16_t
		{
			None = 0,
			Private = BIT(0),
			Internal = BIT(1),
			Protected = BIT(2),
			Public = BIT(3)
		};
		using Flags = std::underlying_type_t<Type>;
	};

	enum class ManagedFieldType : uint16_t
	{
		None = 0,

		Bool,   // => bool
		Char,   // => wchar_t
		Byte,   // => uint8_t
		SByte,  // => int8_t
		Short,  // => int16_t
		UShort, // => uint16_t
		Int,    // => int32_t
		UInt,   // => uint32_t
		Long,   // => int64_t
		ULong,  // => uint64_t
		Float,  // => float
		Double, // => double
		String, // => MonoString*

		Entity, // => UUID
		Component, // => UUID (same as entity)
		Vector2,
		Vector3,
		Vector4


		// TODO(moro):
		//  - Array
		//  - Component
	};

	inline const char* ToString(ManagedFieldType type)
	{
		switch (type)
		{
			case ManagedFieldType::None: return "None";
			case ManagedFieldType::Bool: return "Bool";
			case ManagedFieldType::Char: return "Char";
			case ManagedFieldType::Byte: return "Byte";
			case ManagedFieldType::SByte: return "SByte";
			case ManagedFieldType::Short: return "Short";
			case ManagedFieldType::UShort: return "UShort";
			case ManagedFieldType::Int: return "Int";
			case ManagedFieldType::UInt: return "UInt";
			case ManagedFieldType::Long: return "Long";
			case ManagedFieldType::ULong: return "ULong";
			case ManagedFieldType::Float: return "Float";
			case ManagedFieldType::Double: return "Double";
			case ManagedFieldType::String: return "String";
			case ManagedFieldType::Entity: return "Entity";
			case ManagedFieldType::Vector2: return "Vector2";
			case ManagedFieldType::Vector3: return "Vector3";
			case ManagedFieldType::Vector4: return "Vector4";
		}
		SK_CORE_ASSERT(false, "Unkown ManagedFieldType");
		return "Unkown";
	}

	inline ManagedFieldType ToManagedFieldType(const std::string& type)
	{
		if (type == "None") return ManagedFieldType::None;
		if (type == "Bool") return ManagedFieldType::Bool;
		if (type == "Char") return ManagedFieldType::Char;
		if (type == "Byte") return ManagedFieldType::Byte;
		if (type == "SByte") return ManagedFieldType::SByte;
		if (type == "Short") return ManagedFieldType::Short;
		if (type == "UShort") return ManagedFieldType::UShort;
		if (type == "Int") return ManagedFieldType::Int;
		if (type == "UInt") return ManagedFieldType::UInt;
		if (type == "Long") return ManagedFieldType::Long;
		if (type == "ULong") return ManagedFieldType::ULong;
		if (type == "Float") return ManagedFieldType::Float;
		if (type == "Double") return ManagedFieldType::Double;
		if (type == "String") return ManagedFieldType::String;
		if (type == "Entity") return ManagedFieldType::Entity;
		if (type == "Vector2") return ManagedFieldType::Vector2;
		if (type == "Vector3") return ManagedFieldType::Vector3;
		if (type == "Vector4") return ManagedFieldType::Vector4;

		SK_CORE_ASSERT(false, "Unkown ManagedFieldType string");
		return ManagedFieldType::None;
	}

	inline std::string ToString(Accessibility::Flags access)
	{
		std::string result;
		if (access & Accessibility::Private) result += "Private | ";
		if (access & Accessibility::Internal) result += "Internal | ";
		if (access & Accessibility::Protected) result += "Protected | ";
		if (access & Accessibility::Public) result += "Public | ";

		return result.empty() ? std::string{} : result.substr(0, result.size() - 3);
	}

	using GCHandle = uint32_t;

	struct AssemblyInfo
	{
		MonoAssembly* Assembly = nullptr;
		MonoImage* Image = nullptr;
		std::filesystem::path FilePath;
	};

	class ManagedType
	{
	public:
		MonoType* Type = nullptr;

	public:
		ManagedType(MonoType* type)
			: Type(type)
		{}

		operator MonoType* () const { return Type; }

		int GetSize() const;
		int GetAlignment() const;
	};

	class ManagedField
	{
	public:
		ManagedFieldType Type = ManagedFieldType::None;
		Accessibility::Flags Access = Accessibility::None;
		MonoClassField* Field = nullptr;

	public:
		operator MonoClassField* () const { return Field; }
		ManagedType GetManagedType() const;

		template<typename T>
		void SetValue(GCHandle handle, const T& value)
		{
			SetValueInternal(handle, &value);
		}

		template<typename T>
		auto GetValue(GCHandle handle) const
		{
			T value;
			GetValueInternal(handle, &value);
			return value;
		}

		template<>
		void SetValue<std::string>(GCHandle handle, const std::string& value)
		{
			SetString(handle, value);
		}

		template<>
		auto GetValue<std::string>(GCHandle handle) const
		{
			return GetString(handle);
		}

		UUID GetEntity(GCHandle handle) const;
		void SetEntity(GCHandle handle, Entity entity);

		void SetComponent(GCHandle handle, Entity entity);
		UUID GetComponent(GCHandle handle);

	private:
		void SetValueInternal(GCHandle handle, const void* value);
		void GetValueInternal(GCHandle handle, void* value) const;

		void SetString(GCHandle handle, const std::string& value);
		std::string GetString(GCHandle handle) const;
	};

	class FieldStorage : public RefCount
	{
	public:
		std::string Name;
		ManagedFieldType Type = ManagedFieldType::None;

		template<typename T>
		void SetValue(const T& value)
		{
			static_assert(!std::is_same_v<T, Entity>, "Use SetValue<UUID> instead");
			static_assert(sizeof(T) <= sizeof(m_Buffer));
			memcpy(m_Buffer, &value, sizeof(T));
		}

		template<typename T>
		auto GetValue() const
		{
			static_assert(!std::is_same_v<T, Entity>, "Use GetValue<UUID> instead");
			static_assert(sizeof(T) <= sizeof(m_Buffer));
			return *(T*)m_Buffer;
		}
		
		template<>
		void SetValue(const std::string& value)
		{
			m_String = value;
		}

		template<>
		auto GetValue<std::string>() const
		{
			return m_String;
		}

	public:
		static Ref<FieldStorage> FromManagedField(const ManagedField& field);

	private:
		uint8_t m_Buffer[16]{};
		std::string m_String;

		friend class ScriptEngine;
	};

	class ScriptClass : public RefCount
	{
	public:
		ScriptClass(const std::string& nameSpace, const std::string& name);
		ScriptClass(const AssemblyInfo& assembly, const std::string& nameSpace, const std::string& name);

		uint64_t GetID() const { return m_ID; }
		const std::string& GetName() const { return m_Name; }
		std::map<std::string, ManagedField>& GetFields() { return m_Fields; }

		bool HasField(const std::string& fieldName) const { return m_Fields.find(fieldName) != m_Fields.end(); }
		ManagedField& GetField(const std::string& fieldName) { return m_Fields.at(fieldName); }
		const ManagedField& GetField(const std::string& fieldName) const { return m_Fields.at(fieldName); }

	private:
		std::string m_Name;
		uint64_t m_ID;
		MonoClass* m_Class;

		std::map<std::string, ManagedField> m_Fields;

		friend class ScriptEngine;
	};

}
