#pragma once

#include <array>
#include <string_view>
#include <cstdint>

namespace CRC {
	template <typename T>
	T crc_polynomial;

	template<>
	uint64_t crc_polynomial<uint64_t> = 0x42F0E1EBA9EA3693;

	template<>
	uint32_t crc_polynomial<uint32_t> = 0x04C11DB7;

	template<>
	uint16_t crc_polynomial<uint16_t> = 0x2F15;

	template <typename T>
	T high_bit = T(1) << ((sizeof(T) * 8) - 1);

	template <typename T>
	constexpr T bit_reflect(T in) {
		T out = 0;
		std::size_t num_bits = sizeof(T) * 8;

		for (size_t i = 0; i < num_bits; i++) {
			if (in & (T(1) << i)) {
				out |= high_bit<T> >> i;
			}
		}

		return out;
	}

	template <typename T>
	constexpr std::array<T, 256> compute_crc_table() {
		std::array<T, 256> table;

		for (T i = 0; i < 256; i++) {
			table[i] = bit_reflect(i);

			for (std::size_t b = 0; b < 8; b++) {
				if (table[i] & high_bit<T>) {
					table[i] = ((table[i] << 1) ^ crc_polynomial<uint32_t>);
				}
				else {
					table[i] = table[i] << 1;
				}
			}

			table[i] = bit_reflect(table[i]);
		}

		return table;
	}

	template <typename T>
	constexpr T crcN(std::string_view s) {
		auto table = compute_crc_table<T>();

		T crc = ~T(0);

		for (size_t i = 0; i < s.length(); i++) {
			const T remainder = (crc ^ s[i]) & 0xFF;
			crc = (crc >> 8) ^ table[remainder];
		}

		crc = ~crc;

		return crc;
	}

	constexpr uint64_t crc64(std::string_view s) {
		return crcN<uint64_t>(s);
	}

	constexpr uint32_t crc32(std::string_view s) {
		return crcN<uint32_t>(s);
	}

	constexpr uint16_t crc16(std::string_view s) {
		return crcN<uint16_t>(s);
	}
}
