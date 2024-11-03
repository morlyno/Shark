#pragma once

#define INTERNAL_DESERIALIZE_PROPERTY_FALLBACK0(...)
#define INTERNAL_DESERIALIZE_PROPERTY_FALLBACK1(_value, _fallback) _value = _fallback

#define SK_DESERIALIZE_PROPERTY(_yamlNode, _name, _value, ...)\
try\
{\
	_value = _yamlNode[_name].as<std::decay_t<decltype(_value)>>();\
}\
catch (const YAML::BadConversion& e)\
{\
	SK_CORE_ERROR_TAG("Serialization", "Failed to deserialize property!\n\tName: {}\n\tError: {}", _name, e.what());\
	__VA_OPT__(_value = __VA_ARGS__);\
}

#define SK_DESERIALIZE_VALUE(_yamlNode, _name, _type)\
[&]()\
{\
	_type value;\
	SK_DESERIALIZE_PROPERTY(_yamlNode, _name, value, _type{});\
	return value;\
}()

#define SK_SERIALIZE_PROPERTY(_yamlNode, _name, _value)\
	_yamlNode << YAML::Key << _name << YAML::Value << _value
