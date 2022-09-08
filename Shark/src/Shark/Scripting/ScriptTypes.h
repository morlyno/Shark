#pragma once

#include "Shark/Core/Base.h"

extern "C" {
	typedef struct _MonoAssembly MonoAssembly;
	typedef struct _MonoImage MonoImage;
	typedef struct _MonoClass MonoClass;
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoString MonoString;
	typedef struct _MonoClassField MonoClassField;
}

namespace Shark {

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
		Entity, // => MonoObject*

		// TODO(moro):
		//  - Array
		//  - Component
	};

	using GCHandle = uint32_t;

	struct AssemblyInfo
	{
		MonoAssembly* Assembly = nullptr;
		MonoImage* Image = nullptr;
		std::filesystem::path FilePath;
	};

	class ManagedField
	{
	public:
		ManagedFieldType Type = ManagedFieldType::None;
		Accessibility::Flags Access = Accessibility::None;
		MonoClassField* Field = nullptr;

	public:
		template<typename T>
		void SetValue(GCHandle handle, const T& value)
		{
			static_assert(sizeof(T) <= 8, "Type too large");

			SetValueInternal(handle, &value);
		}

		template<typename T>
		T GetValue(GCHandle handle)
		{
			static_assert(sizeof(T) <= 8, "Type too large");

			uint8_t buffer[8];
			GetValueInternal(handle, buffer);
			return *(T*)buffer;
		}

	private:
		void SetValueInternal(GCHandle handle, const void* value);
		void GetValueInternal(GCHandle handle, void* value);
	};

	class FieldStorage : public RefCount
	{
	public:
		FieldStorage(const ManagedField& field)
			: Field(field)
		{
			memset(m_Buffer, 0, sizeof(m_Buffer));
		}

	public:
		ManagedField Field;

		template<typename T>
		void SetValue(const T& value)
		{
			static_assert(sizeof(T) <= sizeof(m_Buffer));
			memcpy(m_Buffer, &value, sizeof(T));
		}

		template<typename T>
		T GetValue() const
		{
			static_assert(sizeof(T) <= sizeof(m_Buffer));
			return *(T*)m_Buffer;
		}

	private:
		uint8_t m_Buffer[8];

		friend class ScriptEngine;
	};

	class ScriptClass : public RefCount
	{
	public:
		ScriptClass(const std::string& nameSpace, const std::string& name);
		ScriptClass(const AssemblyInfo& assembly, const std::string& nameSpace, const std::string& name);

		const std::string& GetName() const { return m_Name; }
		std::unordered_map<std::string, ManagedField>& GetFields() { return m_Fields; }

	private:
		std::string m_Name;
		uint64_t m_ID;
		MonoClass* m_Class;

		std::unordered_map<std::string, ManagedField> m_Fields;

		friend class ScriptEngine;
	};

}
