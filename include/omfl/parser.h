#pragma once

#include "omfl/value.h"

namespace omfl {

const size_t kAllocationsForString = 64;

class Config {
public:
    Config();

    bool valid() const;

    void SetValid(bool valid);
    Value& GetRoot();

    Value Get(const std::string& path) const;
private:
    bool valid_;
    Value root_;
};

Config parse(const std::string& str);
Config parse(const std::string& str, bool file);

} //namespace omfl