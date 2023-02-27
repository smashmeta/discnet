/*
 *
 */

#include <sstream>
#include <array>
#include <boost/algorithm/string.hpp>
#include <boost/core/ignore_unused.hpp>
#include <openssl/sha.h>
#include <discnet/discnet.hpp>

namespace discnet
{
	std::string sha256_file(const std::string& filename)
	{
		FILE* file = nullptr;
		errno_t error = fopen_s(&file, filename.c_str(), "rb");
		if (error != 0)
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
		#pragma warning(suppress: 4996)
		SHA256_Init(&sha256);
		
		// generate sha256 from file content
		size_t bytesRead = fread(buffer, 1, bufSize, file);
		while(bytesRead > 0)
		{
			#pragma warning(suppress: 4996)
			SHA256_Update(&sha256, buffer, bytesRead);

			bytesRead = fread(buffer, 1, bufSize, file);
		}

		#pragma warning(suppress: 4996)
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