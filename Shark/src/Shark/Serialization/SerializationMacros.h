#pragma once

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
	SK_DESERIALIZE_PROPERTY(_yamlNode, _name, value);\
	return value;\
}()

#define SK_SERIALIZE_PROPERTY(_yamlNode, _name, _value)\
	_yamlNode << YAML::Key << _name << YAML::Value << _value

#define SK_BEGIN_GROUP(_out, _name) _out << YAML::Key << _name << YAML::Value << YAML::BeginMap
#define SK_END_GROUP(_out) _out << YAML::EndMap
