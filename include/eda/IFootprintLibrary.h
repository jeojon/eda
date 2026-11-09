#pragma once

#include "FootprintTypes.h"

#include <memory>
#include <string>
#include <vector>

namespace eda
{

class IFootprintLibrary
{
public:
    virtual ~IFootprintLibrary() = default;

    virtual std::wstring DefaultDirectory() const = 0;
    virtual bool Reload(std::wstring* error = nullptr) = 0;
    virtual const std::vector<LibraryEntry>& Entries() const = 0;

    virtual bool Read(const std::wstring& path, Footprint& footprint, std::wstring* error = nullptr) const = 0;
    virtual bool Write(const std::wstring& path, const Footprint& footprint, std::wstring* error = nullptr) const = 0;
    virtual bool Delete(const std::wstring& path, std::wstring* error = nullptr) const = 0;
};

} // namespace eda

