#include <omfl/value.h>

namespace omfl {

Value::Value() 
    : data_(std::nullopt)
    {}

Value::Value(int32_t value) 
    : data_(value)
    {}

Value::Value(float value) 
    : data_(value)
    {}

Value::Value(bool value)
    : data_(value)
    {}

Value::Value(const std::string& value) 
    : data_(value)
    {} 

Value::Value(const std::vector<Value>& value)
    : data_(value)
    {}

Value Value::CreateSection() {
    Value v;
    v.data_ = std::map<std::string, Value>();
    return v;
}

bool Value::IsInt() const {
    return data_.has_value() && std::holds_alternative<int32_t>(*data_);
}

bool Value::IsBool() const {
    return data_.has_value() && std::holds_alternative<bool>(*data_);
}

bool Value::IsFloat() const {
    return data_.has_value() && std::holds_alternative<float>(*data_);
}


bool Value::IsString() const {
    return data_.has_value() && std::holds_alternative<std::string>(*data_);
}

bool Value::IsArray() const {
    return data_.has_value() && std::holds_alternative<std::vector<Value>>(*data_);
}

bool Value::IsSection() const {
    return data_.has_value() && std::holds_alternative<std::map<std::string, Value>>(*data_);
}

bool Value::IsInvalid() const {
    return !data_.has_value();
}

int32_t Value::AsInt() const {
    if (IsInt()) {
        return std::get<int32_t>(*data_);
    }
    return 0;
}

float Value::AsFloat() const {
    if (IsFloat()) {
        return std::get<float>(*data_);
    }

    return 0.0f;
}

bool Value::AsBool() const {
    if (IsBool()) {
        return std::get<bool>(*data_);
    }

    return false;
}

const std::string& Value::AsString() const {
    if (IsString()) {
        return std::get<std::string>(*data_);
    }

    static const std::string empty;
    return empty;
}

const std::vector<Value>& Value::AsArray() const {
    if (IsArray()) {
        return std::get<std::vector<Value>>(*data_);
    }

    static const std::vector<Value> empty;
    return empty;
} 

int32_t Value::AsIntOrDefault(int32_t default_value) const {
    if (IsInt()) {
        return AsInt();
    }

    return default_value;
}

float Value::AsFloatOrDefault(float default_value) const {
    if (IsFloat()) {
        return AsFloat();
    }

    return default_value;
}

bool Value::AsBoolOrDefault(bool default_value) const {
    if (IsBool()) {
        return AsBool();
    }

    return default_value;
}

std::string Value::AsStringOrDefault(const std::string& default_value) const {
    if (IsString()) {
        return AsString();
    }

    return default_value;
}

Value Value::operator[](size_t index) const {
    if (IsArray()) {
        const auto& arr = std::get<std::vector<Value>>(*data_);
        if (index < arr.size()) {
            return arr[index];
        }
    }

    return Value();
}

const Value *Value::GetRef(const std::string& key) const {
    if (IsSection()) {
        const auto& section = std::get<std::map<std::string,Value>>(*data_);
        auto it = section.find(key);
        if (it != section.end()) {
            return &it->second;
        }
    }

    return nullptr;
}

Value Value::Get(const std::string& key) const {
    const Value *ref = GetRef(key);
    if (ref) {
        return *ref;
    } else {
        return Value();
    }
}

void Value::SetKeyValue(const std::string& key, const Value& value) {
    if(!IsSection()) {
        data_ = std::map<std::string, Value>();
    }

    std::get<std::map<std::string, Value>>(*data_)[key] = value;
}

Value& Value::GetOrCreateSection(const std::string& key) {
    if (!IsSection()) {
        data_ = std::map<std::string, Value>();
    }

    auto& section = std::get<std::map<std::string, Value>>(*data_);
    if (section.find(key) == section.end()) {
        section[key] = Value::CreateSection();
    }

    return section[key];
}

} //namespace omfl