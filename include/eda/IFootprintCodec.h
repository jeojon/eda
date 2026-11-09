#pragma once

#include "FootprintTypes.h"

#include <memory>
#include <string>

namespace eda
{

class IFootprintCodec
{
public:
    virtual ~IFootprintCodec() = default;

    virtual std::string Serialize(const Footprint& footprint) const = 0;
    virtual bool Deserialize(const std::string& text, Footprint& footprint, std::string* error = nullptr) const = 0;
};

} // namespace eda

