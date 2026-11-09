#include "eda/IFootprintLibrary.h"

namespace eda
{
namespace
{

class FilesystemFootprintLibrary final : public IFootprintLibrary
{
public:
    std::wstring DefaultDirectory() const override
    {
        return L"footprints";
    }

    // TODO(student): scan the library directory for *.fp files.
    bool Reload(std::wstring*) override
    {
        entries_.clear();
        return false;
    }

    const std::vector<LibraryEntry>& Entries() const override
    {
        return entries_;
    }

    // TODO(student): read one footprint through IFootprintCodec.
    bool Read(const std::wstring&, Footprint&, std::wstring* error) const override
    {
        if (error != nullptr)
        {
            *error = L"FilesystemFootprintLibrary::Read is not implemented.";
        }
        return false;
    }

    // TODO(student): write one footprint through IFootprintCodec.
    bool Write(const std::wstring&, const Footprint&, std::wstring* error) const override
    {
        if (error != nullptr)
        {
            *error = L"FilesystemFootprintLibrary::Write is not implemented.";
        }
        return false;
    }

    // TODO(student): delete one library file safely.
    bool Delete(const std::wstring&, std::wstring* error) const override
    {
        if (error != nullptr)
        {
            *error = L"FilesystemFootprintLibrary::Delete is not implemented.";
        }
        return false;
    }

private:
    std::vector<LibraryEntry> entries_;
};

} // namespace

std::shared_ptr<IFootprintLibrary> CreateFootprintLibrary()
{
    return std::make_shared<FilesystemFootprintLibrary>();
}

} // namespace eda
