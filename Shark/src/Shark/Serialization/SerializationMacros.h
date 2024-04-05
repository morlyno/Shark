#pragma once

#define SK_DESERIALIZE_PROPERTY(_yamlNode, _name, _value, _fallback)\
try\
{\
	_value = _yamlNode[_name].as<std::decay_t<decltype(_value)>>();\
}\
catch (const YAML::BadConversion& e)\
{\
	SK_CORE_ERROR_TAG("Serialization", "Failed to deserialize property!\n\tName: {}\n\tError: {}", _name, e.what());\
	_value = _fallback;\
}\

#define SK_DESERIALIZE_VALUE(_yamlNode, _name, _type)\
[&]()\
{\
	_type value;\
	SK_DESERIALIZE_PROPERTY(_yamlNode, _name, value, _type{});\
	return value;\
}()

#define SK_SERIALIZE_PROPERTRY(_yamlNode, _name, _value)\
	_yamlNode << YAML::Key << _name << YAML::Value << _value
