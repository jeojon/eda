#include "eda/IFootprintCodec.h"

namespace eda
{
namespace
{

class FootprintCodec final : public IFootprintCodec
{
public:
    // TODO(student): define the .fp file format and serialize all objects.
    std::string Serialize(const Footprint&) const override
    {
        return {};
    }

    // TODO(student): parse the .fp file and validate its version and fields.
    bool Deserialize(const std::string&, Footprint&, std::string* error) const override
    {
        if (error != nullptr)
        {
            *error = "FootprintCodec::Deserialize is not implemented.";
        }
        return false;
    }
};

} // namespace

std::shared_ptr<IFootprintCodec> CreateFootprintCodec()
{
    return std::make_shared<FootprintCodec>();
}

} // namespace eda

