#pragma once

#include <map>
#include <string>
#include <vector>

namespace Updater
{
	class JsonValue
	{
	public:
		enum Type
		{
			TypeNull,
			TypeBool,
			TypeNumber,
			TypeString,
			TypeArray,
			TypeObject
		};

		JsonValue();

		static JsonValue MakeBool(bool value);
		static JsonValue MakeNumber(double value);
		static JsonValue MakeString(const std::string& value);
		static JsonValue MakeArray();
		static JsonValue MakeObject();

		Type GetType() const;
		bool IsNull() const;
		bool IsBool() const;
		bool IsNumber() const;
		bool IsString() const;
		bool IsArray() const;
		bool IsObject() const;

		bool AsBool(bool fallback = false) const;
		double AsNumber(double fallback = 0.0) const;
		const std::string& AsString() const;
		const std::vector<JsonValue>& AsArray() const;
		const std::map<std::string, JsonValue>& AsObject() const;

		std::vector<JsonValue>& MutableArray();
		std::map<std::string, JsonValue>& MutableObject();

		const JsonValue* Find(const std::string& key) const;

	private:
		Type m_type;
		bool m_boolValue;
		double m_numberValue;
		std::string m_stringValue;
		std::vector<JsonValue> m_arrayValue;
		std::map<std::string, JsonValue> m_objectValue;
	};

	bool ParseJson(const std::string& text, JsonValue& outValue, std::string& error);
}
