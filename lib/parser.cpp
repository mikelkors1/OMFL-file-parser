#include <omfl/parser.h>

#include <fstream>
#include <sstream>

namespace omfl {

static bool IsSpace(char symbol) {
    return symbol == ' ';
}

static std::string Trim(const std::string& raw_line) {
    size_t left = 0;
    while (left < raw_line.size() && IsSpace(raw_line[left])) {
        ++left;
    }

    size_t right = raw_line.size();
    while (right > left && IsSpace(raw_line[right - 1])) {
        --right;
    }
    return raw_line.substr(left, right - left);
}

static std::string RemoveComment(const std::string& raw_line) {
    bool in_string = false;
    for (size_t i = 0; i < raw_line.size(); ++i) {
        char c = raw_line[i];
        if (c == '"') {
            in_string = !in_string;
        } else if (c == '#' && !in_string) {
            return raw_line.substr(0,i);
        }
    }
    return raw_line;
}

static std::vector<std::string> SplitLines(const std::string& lines) {
    std::vector<std::string> out;
    std::string curr;
    curr.reserve(kAllocationsForString);
    for (char c : lines) {
        if (c == '\n') {
            out.push_back(curr);
            curr.clear();
        } else {
            curr.push_back(c);
        }
    }
    out.push_back(curr);
    return out;
}

static std::vector<std::string> SplitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::string curr;

    for (char c : path) {
        if (c == '.') {
            if (!curr.empty()) {
                parts.push_back(curr);
                curr.clear();
            }
        } else {
            curr += c;
        }
    }

    if (!curr.empty()) {
        parts.push_back(curr);
    }

    return parts;
}

static bool IsSectionLine(const std::string& line) {
    return line.size() >= 2 && line.front() == '[' && line.back() == ']';
}

static bool IsKeyValueLine(const std::string& line) {
    return line.find('=') != std::string::npos; 
}


static std::string ExtractSectionName(const std::string& line) {
    if (!IsSectionLine(line)) {
        return "";
    }
    return line.substr(1,line.size()-2);
}

static bool IsValidKey(const std::string& key) {
    if (key.empty()) {
        return false;
    }

    for (char c: key) {
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
               c == '-' || c == '_')) {
                return false;
        }
    }

    return true;
}

static Value ParseValue(const std::string& value_str);

static bool IsBoolValue(const std::string& value_str) {
    return value_str == "true" || value_str == "false";
}

static bool IsStringValue(const std::string& value_str) {
    if (value_str.size() < 2 || value_str.front() != '"' || value_str.back() != '"') {
        return false;
    }

    size_t pos = value_str.find('"',1);
    if (pos != std::string::npos && pos != value_str.size() - 1) {
        return false;
    }

    return true;
}

static bool IsArrayValue(const std::string& value_str) {
    return value_str.size() >= 2 && value_str.front() == '[' && value_str.back() == ']';
}

static bool IsIntValue(const std::string& value_str) {
    if (value_str.empty()) {
        return false;
    }

    size_t start = 0;
    if (value_str[0] == '+' || value_str[0] == '-') {
        start = 1;
        if (value_str.size() == 1) {
            return false;
        }
    }

    for (size_t i = start; i < value_str.size(); ++i) {
        if (value_str[i] < '0' || value_str[i] > '9') {
            return false;
        }
    }
    return true;
}

static bool IsFloatValue(const std::string& value_str) {
    if (value_str.empty()) {
        return false;
    }

    size_t start = 0;
    if (value_str[0] == '+' || value_str[0] == '-') {
        start = 1;
        if (value_str.size() == 1) {
            return false;
        }
    }

    size_t dot_pos = value_str.find('.', start);
    if (dot_pos == std::string::npos || dot_pos == start) {
        return false;
    }

    if (dot_pos == value_str.size() - 1) {
        return false;
    }

    for (size_t i = start; i < dot_pos; ++i) {
        if (value_str[i] < '0' || value_str[i] > '9') {
            return false;
        }
    }

    for (size_t i = dot_pos + 1;i < value_str.size(); ++i) {
        if (value_str[i] < '0' || value_str[i] > '9') {
            return false;
        }
    }

    return true;
}

static std::vector<Value> ParseArrayElements(const std::string& value_str) {
    std::string content = value_str.substr(1, value_str.size() - 2);
    content = Trim(content);
    
    if (content.empty()) {
        return std::vector<Value>();
    }

    std::vector<Value> elements;
    std::string curr_element;
    int bracket_level = 0;
    bool in_string = false;

    for (size_t i = 0; i < content.size(); ++i) {
        char c = content[i];

        if(c == '"') {
            in_string = !in_string;
            curr_element += c;
        } else if (!in_string) {
            if (c == '[') {
                bracket_level++;
                curr_element += c;
            } else if (c == ']') {
                bracket_level--;
                curr_element += c;
            } else if (c == ',' && bracket_level == 0) {
                std::string elem = Trim(curr_element);
                if (!elem.empty()) {
                    Value parsed_elem = ParseValue(elem);
                    if (parsed_elem.IsInvalid()) {
                        return std::vector<Value>();
                    }
                    elements.push_back(parsed_elem);
                }
                curr_element.clear();
            } else {
                curr_element += c;
            }
        } else {
            curr_element += c;
        }
    }

    if (bracket_level != 0 || in_string) {
        return std::vector<Value>();
    }

    std::string elem = Trim(curr_element);
    if (!elem.empty()) {
        Value parsed_elem = ParseValue(elem);
        if (parsed_elem.IsInvalid()) {
            return std::vector<Value>();
        }
        elements.push_back(parsed_elem);
    }

    return elements;
}

static Value ParseValue(const std::string& value_str) {
    if (IsStringValue(value_str)) {
        return Value(value_str.substr(1, value_str.size() - 2));
    }
    
    if (IsBoolValue(value_str)) {
        return Value(value_str == "true");
    }
    
    if (IsArrayValue(value_str)) {
        std::vector<Value> elements = ParseArrayElements(value_str);
        if (elements.empty()) {
            std::string content = value_str.substr(1, value_str.size() - 2);
            content = Trim(content);
            if (content.empty()) {
                return Value(std::vector<Value>());
            }
            return Value();
        }
        return Value(elements);
    }
    
    if (IsFloatValue(value_str)) {
        return Value(std::stof(value_str));
    }
    
    if (IsIntValue(value_str)) {
        return Value(std::stoi(value_str));
    }
    
    return Value();
}

Config::Config() 
    : valid_(true) , root_(Value::CreateSection()) 
    {}

bool Config::valid() const {
    return valid_;
}

void Config::SetValid(bool valid) {
    valid_ = valid;
}

Value& Config::GetRoot() {
    return root_;
}

Value Config::Get(const std::string& path) const {
    if (!valid_) {
        return Value();
    }

    std::vector<std::string> parts = SplitPath(path);

    if (parts.empty()) {
        return Value();
    }

    const Value *current_section = &root_;
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        current_section = current_section->GetRef(parts[i]);
        if (current_section == nullptr || !current_section->IsSection()) {
            return Value();
        }
    }

    const Value *result = current_section->GetRef(parts.back());
    return result ? *result : Value();
}

Config parse(const std::string& str) {
    Config config;

    const auto lines = SplitLines(str);

    std::vector<std::string> current_section_path;

    for (const auto& raw_line : lines) {
        std::string line = Trim(RemoveComment(raw_line));
        if (line.empty()) {
            continue;
        }

        if (IsSectionLine(line)) {
            std::string section_name = ExtractSectionName(line);
            
            if (section_name.empty()) {
                config.SetValid(false);
                return config;
            }
            
            if (section_name.front() == '.' || section_name.back() == '.') {
                config.SetValid(false);
                return config;
            }

            std::vector<std::string> section_parts = SplitPath(section_name);

            for (const auto& part : section_parts) {
                if (!IsValidKey(part)) {
                    config.SetValid(false);
                    return config;
                }
            }

            Value *current_section = &config.GetRoot();
            for (const auto& part : section_parts) {
                current_section = &current_section->GetOrCreateSection(part);
            }

            current_section_path = section_parts;
            continue;
        }

        if (IsKeyValueLine(line)) {
            size_t index = line.find('=');

            std::string key = Trim(line.substr(0, index));
            std::string value = Trim(line.substr(index + 1));

            if (key.empty() || !IsValidKey(key)) {
                config.SetValid(false);
                return config;
            }

            Value *target_section = &config.GetRoot();
            for (const auto& part : current_section_path) {
                target_section = &target_section->GetOrCreateSection(part);
            }

            const Value *existing = target_section->GetRef(key);
            if (existing != nullptr && !existing->IsSection()) {
                config.SetValid(false);
                return config;
            }

            Value parsed_value = ParseValue(value);

            if (parsed_value.IsInvalid()) {
                config.SetValid(false);
                return config;
            }

            target_section->SetKeyValue(key, parsed_value);
            continue;
        }

        config.SetValid(false);
        return config;
    }

    return config;
}

Config parse(const std::string& str, bool file) {
    if (!file) {
        return parse(str);
    }

    std::ifstream in(str);
    if (!in.is_open()) {
        Config cfg;
        cfg.SetValid(false);
        return cfg;
    }
    std::stringstream buff;
    buff << in.rdbuf();
    std::string line = buff.str();

    Config root = parse(line);

    return root;
}

} // namespace omfl
