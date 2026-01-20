/*
 *
 */

#include <sstream>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <boost/algorithm/string.hpp>
#include <boost/core/ignore_unused.hpp>
#include <openssl/sha.h>
#include <discnet/discnet.hpp>

namespace discnet
{

#ifdef _WIN32
	#pragma warning( push )
	#pragma warning( disable : 4996 )
#elif defined(__GNUC__) && !defined(__clang__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#else
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

	std::string sha256_file(const std::string& filename)
	{
		FILE* file = std::fopen(filename.c_str(), "rb");
		if (!file)
		{
			return "";
		} 

		const int bufSize = 32768;
		char* buffer =  static_cast<char*>(malloc(bufSize));
		if(buffer == nullptr)
		{
			return "";
		} 

		unsigned char hash[SHA256_DIGEST_LENGTH];
		SHA256_CTX sha256;
		
		// using depricated sha256 interface (suppressing warning)
		SHA256_Init(&sha256);
		
		// generate sha256 from file content
		size_t bytesRead = fread(buffer, 1, bufSize, file);
		while(bytesRead > 0)
		{
			SHA256_Update(&sha256, buffer, bytesRead);

			bytesRead = fread(buffer, 1, bufSize, file);
		}

		SHA256_Final(hash, &sha256);
		
		fclose(file);
		free(buffer);

		// converting sha256 number to string
		std::string result(64, '\0');
		for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i)
		{
			// {:02x} => write byte as hex and fill first char with 0
			// if the hex number does not expand to 2 characters.
			std::format_to(result.begin()+(i*2), "{:02x}", hash[i]);
		}

		return result;
	}
#ifdef _WIN32
	#pragma warning( pop )
#elif defined(__GNUC__) && !defined(__clang__)
	#pragma GCC diagnostic pop
#else
	#pragma clang diagnostic pop
#endif

	std::string bytes_to_hex_string(const std::span<const std::byte>& buffer)
    {
		// safety check
		if (buffer.size() <= 0)
		{
			return "";
		}

		size_t result_size = buffer.size() == 1 ? 2 : (buffer.size()*3) - 1;
		std::string result(result_size, ' ');
		for (size_t i = 0; i < buffer.size(); ++i)
		{
			std::format_to(result.begin()+((i)*3), "{:02X}", (unsigned char)buffer[i]);
		}

		return result;
	}

	std::string bytes_to_hex_string(const std::span<const discnet::byte_t>& buffer)
	{
		return bytes_to_hex_string((const std::span<std::byte>&)buffer);
	}
} // !namespace discnet