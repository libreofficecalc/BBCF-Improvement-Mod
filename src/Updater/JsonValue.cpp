#include "JsonValue.h"

#include <cctype>
#include <cstring>
#include <cstdlib>

namespace Updater
{
	namespace
	{
		const std::string EmptyString;
		const std::vector<JsonValue> EmptyArray;
		const std::map<std::string, JsonValue> EmptyObject;

		class JsonParser
		{
		public:
			JsonParser(const std::string& text)
				: m_text(text), m_pos(0)
			{
			}

			bool Parse(JsonValue& outValue, std::string& error)
			{
				SkipWhitespace();
				if (!ParseValue(outValue, error))
					return false;

				SkipWhitespace();
				if (m_pos != m_text.size())
				{
					error = "Unexpected trailing JSON data.";
					return false;
				}

				return true;
			}

		private:
			bool ParseValue(JsonValue& outValue, std::string& error)
			{
				SkipWhitespace();
				if (m_pos >= m_text.size())
				{
					error = "Unexpected end of JSON.";
					return false;
				}

				const char c = m_text[m_pos];
				if (c == 'n')
					return ParseLiteral("null", JsonValue(), outValue, error);
				if (c == 't')
					return ParseLiteral("true", JsonValue::MakeBool(true), outValue, error);
				if (c == 'f')
					return ParseLiteral("false", JsonValue::MakeBool(false), outValue, error);
				if (c == '"')
					return ParseStringValue(outValue, error);
				if (c == '[')
					return ParseArray(outValue, error);
				if (c == '{')
					return ParseObject(outValue, error);
				if (c == '-' || IsDigit(c))
					return ParseNumber(outValue, error);

				error = "Unexpected JSON value.";
				return false;
			}

			bool ParseLiteral(const char* literal, const JsonValue& value, JsonValue& outValue, std::string& error)
			{
				const size_t len = std::strlen(literal);
				if (m_text.compare(m_pos, len, literal) != 0)
				{
					error = "Invalid JSON literal.";
					return false;
				}

				m_pos += len;
				outValue = value;
				return true;
			}

			bool ParseStringValue(JsonValue& outValue, std::string& error)
			{
				std::string value;
				if (!ParseString(value, error))
					return false;

				outValue = JsonValue::MakeString(value);
				return true;
			}

			bool ParseString(std::string& outString, std::string& error)
			{
				if (!Consume('"'))
				{
					error = "Expected JSON string.";
					return false;
				}

				outString.clear();
				while (m_pos < m_text.size())
				{
					const char c = m_text[m_pos++];
					if (c == '"')
						return true;

					if (static_cast<unsigned char>(c) < 0x20)
					{
						error = "Control character in JSON string.";
						return false;
					}

					if (c != '\\')
					{
						outString.push_back(c);
						continue;
					}

					if (m_pos >= m_text.size())
					{
						error = "Unterminated JSON string escape.";
						return false;
					}

					const char escaped = m_text[m_pos++];
					switch (escaped)
					{
					case '"': outString.push_back('"'); break;
					case '\\': outString.push_back('\\'); break;
					case '/': outString.push_back('/'); break;
					case 'b': outString.push_back('\b'); break;
					case 'f': outString.push_back('\f'); break;
					case 'n': outString.push_back('\n'); break;
					case 'r': outString.push_back('\r'); break;
					case 't': outString.push_back('\t'); break;
					case 'u':
						if (!ParseUnicodeEscape(outString, error))
							return false;
						break;
					default:
						error = "Invalid JSON string escape.";
						return false;
					}
				}

				error = "Unterminated JSON string.";
				return false;
			}

			bool ParseUnicodeEscape(std::string& outString, std::string& error)
			{
				if (m_pos + 4 > m_text.size())
				{
					error = "Short JSON unicode escape.";
					return false;
				}

				unsigned int codePoint = 0;
				for (int i = 0; i < 4; ++i)
				{
					const int digit = HexDigit(m_text[m_pos++]);
					if (digit < 0)
					{
						error = "Invalid JSON unicode escape.";
						return false;
					}
					codePoint = (codePoint << 4) | static_cast<unsigned int>(digit);
				}

				AppendUtf8(codePoint, outString);
				return true;
			}

			bool ParseArray(JsonValue& outValue, std::string& error)
			{
				if (!Consume('['))
				{
					error = "Expected JSON array.";
					return false;
				}

				outValue = JsonValue::MakeArray();
				SkipWhitespace();
				if (Consume(']'))
					return true;

				for (;;)
				{
					JsonValue item;
					if (!ParseValue(item, error))
						return false;

					outValue.MutableArray().push_back(item);
					SkipWhitespace();

					if (Consume(']'))
						return true;
					if (!Consume(','))
					{
						error = "Expected comma or array end.";
						return false;
					}
				}
			}

			bool ParseObject(JsonValue& outValue, std::string& error)
			{
				if (!Consume('{'))
				{
					error = "Expected JSON object.";
					return false;
				}

				outValue = JsonValue::MakeObject();
				SkipWhitespace();
				if (Consume('}'))
					return true;

				for (;;)
				{
					std::string key;
					if (!ParseString(key, error))
						return false;

					SkipWhitespace();
					if (!Consume(':'))
					{
						error = "Expected object key separator.";
						return false;
					}

					JsonValue value;
					if (!ParseValue(value, error))
						return false;

					outValue.MutableObject()[key] = value;
					SkipWhitespace();

					if (Consume('}'))
						return true;
					if (!Consume(','))
					{
						error = "Expected comma or object end.";
						return false;
					}
					SkipWhitespace();
				}
			}

			bool ParseNumber(JsonValue& outValue, std::string& error)
			{
				const size_t start = m_pos;
				if (Consume('-') && m_pos >= m_text.size())
				{
					error = "Invalid JSON number.";
					return false;
				}

				if (Consume('0'))
				{
				}
				else if (m_pos < m_text.size() && IsDigitOneToNine(m_text[m_pos]))
				{
					while (m_pos < m_text.size() && IsDigit(m_text[m_pos]))
						++m_pos;
				}
				else
				{
					error = "Invalid JSON number.";
					return false;
				}

				if (Consume('.'))
				{
					if (m_pos >= m_text.size() || !IsDigit(m_text[m_pos]))
					{
						error = "Invalid JSON fraction.";
						return false;
					}
					while (m_pos < m_text.size() && IsDigit(m_text[m_pos]))
						++m_pos;
				}

				if (m_pos < m_text.size() && (m_text[m_pos] == 'e' || m_text[m_pos] == 'E'))
				{
					++m_pos;
					if (m_pos < m_text.size() && (m_text[m_pos] == '+' || m_text[m_pos] == '-'))
						++m_pos;
					if (m_pos >= m_text.size() || !IsDigit(m_text[m_pos]))
					{
						error = "Invalid JSON exponent.";
						return false;
					}
					while (m_pos < m_text.size() && IsDigit(m_text[m_pos]))
						++m_pos;
				}

				const std::string numberText = m_text.substr(start, m_pos - start);
				outValue = JsonValue::MakeNumber(std::strtod(numberText.c_str(), nullptr));
				return true;
			}

			void SkipWhitespace()
			{
				while (m_pos < m_text.size())
				{
					const unsigned char c = static_cast<unsigned char>(m_text[m_pos]);
					if (!std::isspace(c))
						break;
					++m_pos;
				}
			}

			bool Consume(char expected)
			{
				if (m_pos < m_text.size() && m_text[m_pos] == expected)
				{
					++m_pos;
					return true;
				}
				return false;
			}

			static bool IsDigit(char c)
			{
				return c >= '0' && c <= '9';
			}

			static bool IsDigitOneToNine(char c)
			{
				return c >= '1' && c <= '9';
			}

			static int HexDigit(char c)
			{
				if (c >= '0' && c <= '9')
					return c - '0';
				if (c >= 'a' && c <= 'f')
					return c - 'a' + 10;
				if (c >= 'A' && c <= 'F')
					return c - 'A' + 10;
				return -1;
			}

			static void AppendUtf8(unsigned int codePoint, std::string& outString)
			{
				if (codePoint <= 0x7F)
				{
					outString.push_back(static_cast<char>(codePoint));
				}
				else if (codePoint <= 0x7FF)
				{
					outString.push_back(static_cast<char>(0xC0 | ((codePoint >> 6) & 0x1F)));
					outString.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
				}
				else
				{
					outString.push_back(static_cast<char>(0xE0 | ((codePoint >> 12) & 0x0F)));
					outString.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
					outString.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
				}
			}

			const std::string& m_text;
			size_t m_pos;
		};
	}

	JsonValue::JsonValue()
		: m_type(TypeNull), m_boolValue(false), m_numberValue(0.0)
	{
	}

	JsonValue JsonValue::MakeBool(bool value)
	{
		JsonValue result;
		result.m_type = TypeBool;
		result.m_boolValue = value;
		return result;
	}

	JsonValue JsonValue::MakeNumber(double value)
	{
		JsonValue result;
		result.m_type = TypeNumber;
		result.m_numberValue = value;
		return result;
	}

	JsonValue JsonValue::MakeString(const std::string& value)
	{
		JsonValue result;
		result.m_type = TypeString;
		result.m_stringValue = value;
		return result;
	}

	JsonValue JsonValue::MakeArray()
	{
		JsonValue result;
		result.m_type = TypeArray;
		return result;
	}

	JsonValue JsonValue::MakeObject()
	{
		JsonValue result;
		result.m_type = TypeObject;
		return result;
	}

	JsonValue::Type JsonValue::GetType() const { return m_type; }
	bool JsonValue::IsNull() const { return m_type == TypeNull; }
	bool JsonValue::IsBool() const { return m_type == TypeBool; }
	bool JsonValue::IsNumber() const { return m_type == TypeNumber; }
	bool JsonValue::IsString() const { return m_type == TypeString; }
	bool JsonValue::IsArray() const { return m_type == TypeArray; }
	bool JsonValue::IsObject() const { return m_type == TypeObject; }

	bool JsonValue::AsBool(bool fallback) const { return IsBool() ? m_boolValue : fallback; }
	double JsonValue::AsNumber(double fallback) const { return IsNumber() ? m_numberValue : fallback; }
	const std::string& JsonValue::AsString() const { return IsString() ? m_stringValue : EmptyString; }
	const std::vector<JsonValue>& JsonValue::AsArray() const { return IsArray() ? m_arrayValue : EmptyArray; }
	const std::map<std::string, JsonValue>& JsonValue::AsObject() const { return IsObject() ? m_objectValue : EmptyObject; }

	std::vector<JsonValue>& JsonValue::MutableArray()
	{
		return m_arrayValue;
	}

	std::map<std::string, JsonValue>& JsonValue::MutableObject()
	{
		return m_objectValue;
	}

	const JsonValue* JsonValue::Find(const std::string& key) const
	{
		if (!IsObject())
			return nullptr;

		const std::map<std::string, JsonValue>::const_iterator found = m_objectValue.find(key);
		return found == m_objectValue.end() ? nullptr : &found->second;
	}

	bool ParseJson(const std::string& text, JsonValue& outValue, std::string& error)
	{
		error.clear();
		JsonParser parser(text);
		return parser.Parse(outValue, error);
	}
}
