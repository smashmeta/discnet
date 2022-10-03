/*
 *
 */

#include <sstream>
#include <boost/algorithm/string.hpp>
#include <discnet_lib/discnet_lib.hpp>

namespace discnet
{
    std::string bytes_to_hex_string(const std::span<std::byte>& buffer)
    {
		std::stringstream result;

		for (size_t i = 0; i < buffer.size(); ++i)
		{
			const std::byte& byte = buffer[i];
			if (i > 0)
			{
				result << " ";
			}

			const int first_nibble = ((unsigned char)byte >> 4) & 0xf;
			const int second_nibble = (unsigned char)byte & 0xf;
			result << std::hex << first_nibble << std::hex << second_nibble;
		}

		return boost::to_upper_copy(result.str());
    }
} // !namespace discnet