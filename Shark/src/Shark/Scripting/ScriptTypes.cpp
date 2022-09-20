#include "skpch.h"
#include "ScriptTypes.h"

#include "Shark/Scene/Entity.h"

#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/Scripting/GCManager.h"

#include <mono/metadata/class.h>
#include "mono/metadata/object.h"
#include "mono/metadata/attrdefs.h"
#include "mono/metadata/debug-helpers.h"

namespace Shark {

	namespace utils {

		static ManagedFieldType MonoFieldGetManagedFieldType(MonoClassField* field)
		{
			MonoType* monoType = mono_field_get_type(field);
			int type = mono_type_get_type(monoType);
			switch (type)
			{
				case MONO_TYPE_BOOLEAN: return ManagedFieldType::Bool;
				case MONO_TYPE_CHAR:    return ManagedFieldType::Char;
				case MONO_TYPE_I1:      return ManagedFieldType::SByte;
				case MONO_TYPE_U1:      return ManagedFieldType::Byte;
				case MONO_TYPE_I2:      return ManagedFieldType::Short;
				case MONO_TYPE_U2:      return ManagedFieldType::UShort;
				case MONO_TYPE_I4:      return ManagedFieldType::Int;
				case MONO_TYPE_U4:      return ManagedFieldType::UInt;
				case MONO_TYPE_I8:      return ManagedFieldType::Long;
				case MONO_TYPE_U8:      return ManagedFieldType::ULong;
				case MONO_TYPE_R4:      return ManagedFieldType::Float;
				case MONO_TYPE_R8:      return ManagedFieldType::Double;
				case MONO_TYPE_STRING:  return ManagedFieldType::String;
				case MONO_TYPE_CLASS:
				{
					MonoType* monoType = mono_field_get_type(field);
					MonoClass* klass = mono_type_get_class(monoType);
					//MonoClass* klass = mono_field_get_parent(field);
					if (mono_class_is_subclass_of(ScriptEngine::GetEntityClass(), klass, false))
						return ManagedFieldType::Entity;
					return ManagedFieldType::None;
				}
				case MONO_TYPE_VALUETYPE:
				{
					MonoType* monoType = mono_field_get_type(field);
					char* typeName = mono_type_full_name(monoType);
					ManagedFieldType result = ManagedFieldType::None;

					if (typeName == "Shark.Vector2"sv) result = ManagedFieldType::Vector2;
					if (typeName == "Shark.Vector3"sv) result = ManagedFieldType::Vector3;
					if (typeName == "Shark.Vector4"sv) result = ManagedFieldType::Vector4;

					mono_free(typeName);
					return result;
				}
				case MONO_TYPE_I: SK_CORE_ASSERT(false, "don't know what type this is"); return ManagedFieldType::None;
				case MONO_TYPE_U: SK_CORE_ASSERT(false, "don't know what type this is"); return ManagedFieldType::None;
			}

			return ManagedFieldType::None;
		}

		static Accessibility::Flags GetFieldAccess(MonoClassField* field)
		{
			Accessibility::Flags accessFlags = Accessibility::None;
			uint32_t access = mono_field_get_flags(field) & MONO_FIELD_ATTR_FIELD_ACCESS_MASK;
			switch (access)
			{
				case MONO_FIELD_ATTR_PRIVATE:       return Accessibility::Private;
				case MONO_FIELD_ATTR_FAM_AND_ASSEM: return Accessibility::Protected | Accessibility::Internal;
				case MONO_FIELD_ATTR_ASSEMBLY:      return Accessibility::Internal;
				case MONO_FIELD_ATTR_FAMILY:        return Accessibility::Protected;
				case MONO_FIELD_ATTR_FAM_OR_ASSEM:  return Accessibility::Private | Accessibility::Protected;
				case MONO_FIELD_ATTR_PUBLIC:        return Accessibility::Public;
			}
			SK_CORE_ASSERT(false, "Unkown Access");
			return Accessibility::None;
		}

	}

	int ManagedType::GetSize() const
	{
		int alignment;
		return mono_type_size(Type, &alignment);
	}

	int ManagedType::GetAlignment() const
	{
		int alignment;
		int size = mono_type_size(Type, &alignment);
		return alignment;
	}

	ManagedType ManagedField::GetManagedType() const
	{
		MonoType* monoType = mono_field_get_type(Field);
		return ManagedType(monoType);
	}

	UUID ManagedField::GetEntity(GCHandle handle) const
	{
		MonoObject* object = GCManager::GetManagedObject(handle);
		MonoObject* entityObject = mono_field_get_value_object(ScriptEngine::GetRuntimeDomain(), Field, object);
		if (!entityObject)
			return UUID::Null;

		MonoClass* klass = mono_object_get_class(entityObject);
		MonoClassField* idField = mono_class_get_field_from_name(klass, "ID");
		SK_CORE_ASSERT(idField, "Field Is not an entity");
		uint64_t id = 0;
		mono_field_get_value(entityObject, idField, &id);
		return id;
	}

	void ManagedField::SetEntity(GCHandle handle, Entity entity)
	{
		MonoObject* entityInstance;
		if (ScriptEngine::IsInstantiated(entity))
			entityInstance = ScriptEngine::GetInstanceObject(entity);
		else
			entityInstance = ScriptEngine::InstantiateBaseEntity(entity);

		MonoObject* object = GCManager::GetManagedObject(handle);
		mono_field_set_value(object, Field, entityInstance);
	}

	void ManagedField::SetValueInternal(GCHandle handle, const void* value)
	{
		MonoObject* obj = GCManager::GetManagedObject(handle);
		mono_field_set_value(obj, Field, (void*)value);
	}

	void ManagedField::GetValueInternal(GCHandle handle, void* value) const
	{
		MonoObject* obj = GCManager::GetManagedObject(handle);
		mono_field_get_value(obj, Field, value);
	}

	void ManagedField::SetString(GCHandle handle, const std::string& value)
	{
		MonoString* monoString = ScriptUtils::UTF8ToMonoString(value);
		MonoObject* obj = GCManager::GetManagedObject(handle);
		mono_field_set_value(obj, Field, monoString);
	}

	std::string ManagedField::GetString(GCHandle handle) const
	{
		MonoObject* obj = GCManager::GetManagedObject(handle);
		MonoObject* stringObj = mono_field_get_value_object(ScriptEngine::GetRuntimeDomain(), Field, obj);
		MonoString* monoString = mono_object_to_string(stringObj, nullptr);
		return ScriptUtils::MonoStringToUTF8(monoString);
	}

	Ref<FieldStorage> FieldStorage::FromManagedField(const ManagedField& field)
	{
		Ref<FieldStorage> storage = Ref<FieldStorage>::Create();
		storage->Type = field.Type;
		storage->Name = mono_field_full_name(field);
		return storage;
	}

	ScriptClass::ScriptClass(const std::string& nameSpace, const std::string& name)
		: ScriptClass(ScriptEngine::GetAppAssemblyInfo(), nameSpace, name)
	{

	}

	ScriptClass::ScriptClass(const AssemblyInfo& assembly, const std::string& nameSpace, const std::string& name)
	{
		m_Name = fmt::format("{}.{}", nameSpace, name);
		m_ID = Hash::GenerateFNV(m_Name);
		m_Class = mono_class_from_name(assembly.Image, nameSpace.c_str(), name.c_str());

		void* iter = nullptr;
		while (MonoClassField* field = mono_class_get_fields(m_Class, &iter))
		{
			if (mono_field_get_flags(field) & MONO_FIELD_ATTR_STATIC)
				continue;

			ManagedFieldType type = utils::MonoFieldGetManagedFieldType(field);
			Accessibility::Flags access = utils::GetFieldAccess(field);
			if (type == ManagedFieldType::None || !(access & Accessibility::Public))
				continue;

			const char* fieldName = mono_field_get_name(field);
			ManagedField& managedField = m_Fields[fieldName];
			managedField.Type = type;
			managedField.Access = access;
			managedField.Field = field;
		}
	}

}

