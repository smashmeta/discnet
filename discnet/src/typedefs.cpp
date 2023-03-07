/* 
 *
 */

#include <discnet/typedefs.hpp>

namespace discnet
{
    std::string to_string(const jumps_t& jumps)
    {
        std::string result = "{";
        for (const auto& jump : jumps)
        {
            if (result.size() > 1)
            {
                result += ", ";
            }

            result += std::to_string(jump); 
        }

        result += "}";
        return result;
    }
} // ! namespace discnet