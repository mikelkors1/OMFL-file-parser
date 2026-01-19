#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <optional>
#include <cstdint>

namespace omfl {

class Value {
public:
    Value();
    Value(int32_t value);
    Value(float value);
    Value(const std::string& value);
    Value(bool value);
    Value(const std::vector<Value>& array);

    static Value CreateSection();

    Value(const Value& other) = default;

    Value& operator=(const Value& other) = default;

    ~Value() = default;
    
    bool IsInt() const;
    bool IsFloat() const;
    bool IsString() const;
    bool IsBool() const;
    bool IsArray() const;
    bool IsSection() const;
    bool IsInvalid() const;

    int32_t AsInt() const;
    float AsFloat() const;
    const std::string& AsString() const;
    bool AsBool() const;
    const std::vector<Value>& AsArray() const;

    int32_t AsIntOrDefault(int32_t default_value) const;
    float AsFloatOrDefault(float default_value) const;
    std::string AsStringOrDefault(const std::string& default_value) const;
    bool AsBoolOrDefault(bool default_value) const;
    
    Value operator[](size_t index) const;

    const Value* GetRef(const std::string& key) const;
    Value Get(const std::string& key) const;
    
    void SetKeyValue(const std::string& key, const Value& value);
    Value& GetOrCreateSection(const std::string& key);
private:
    using ValueVariant = std::variant<
        int32_t,
        float,
        bool,
        std::string,
        std::vector<Value>,
        std::map<std::string, Value>
    >;

    std::optional<ValueVariant> data_;
};

} // namespcae omfl