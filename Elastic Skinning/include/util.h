#pragma once

#include "crc.h"

#include <initializer_list>
#include <vector>
#include <array>
#include <string_view>
#include <algorithm>
#include <iterator>
#include <concepts>
#include <type_traits>
#include <utility>
#include <cstdio>
#include <cstdint>

template <typename VALUE_TYPE, typename STATUS_TYPE>
struct Retval {
	VALUE_TYPE value;
	STATUS_TYPE status;
};

using BinaryBlob = std::vector<uint8_t>;

struct BinaryBlobView {
	const BinaryBlob* source{ nullptr };
	size_t offset{ 0 };
	size_t size{ 0 };
	size_t stride{ 0 };

	const bool empty() {
		return (source == nullptr) || (size == 0);
	}

	const BinaryBlob::value_type& operator[](size_t idx) {
		size_t actual_stride = (stride == 0) ? 1 : stride;
		return source->at(offset + (actual_stride * idx));
	}

	const bool index_in_bounds(size_t idx) {
		size_t actual_stride = (stride == 0) ? 1 : stride;
		return (idx < size) && ((offset + (actual_stride * idx)) < source->size());
	}
};

template <typename T>
inline constexpr T convert_binary_to_type(const std::array<uint8_t, sizeof(T)>& data) {
	union {
		T out;
		std::array<uint8_t, sizeof(T)> raw;
	};

	raw = data;

	return out;
}

template <typename T>
struct BinaryBlobAccessor : public BinaryBlobView {
	size_t element_offset{ 0 };
	size_t element_count{ 0 };

	const bool empty() {
		return (source == nullptr) || (size == 0) || (element_count == 0);
	}

	T operator[](size_t idx) {
		size_t actual_stride = (stride == 0) ? sizeof(T) : stride;
		size_t base_idx = (offset + (actual_stride * idx));
		std::array<uint8_t, sizeof(T)> data;

		for (size_t i = 0; i < sizeof(T); i++) {
			data[i] = source->at(base_idx + element_offset + i);
		}

		return convert_binary_to_type<T>(data);
	}
};

template <typename T>
concept Hash = std::same_as<T, uint64_t>;

static const uint64_t NULL_HASH = 0;

using StringHash = uint64_t;

template <Hash... Ts>
inline constexpr uint64_t hash_combine(Ts... hashes) {
	std::array<uint64_t, sizeof...(Ts)> data{ hashes... };

	if constexpr (sizeof...(Ts) == 0) {
		return 0;
	}
	else if constexpr (sizeof...(Ts) == 1) {
		return data[0];
	}

	uint64_t ret = 0;

	for (size_t i = 0; i < data.size(); i++) {
		ret ^= data[i] + 0x9e3779b9 + (ret << 6) + (ret >> 2);
	}

	return ret;
}

template <size_t N>
struct StringLiteral {
	constexpr StringLiteral(const char(&str)[N]) {
		std::copy(str, str + N, value);
	}

	char value[N];
};

template <typename T>
concept ArrayType =
	requires (T a) {
		{a.size()} -> std::integral;
		{a[0]} -> std::same_as<typename T::reference>;
	};

template <typename T>
concept BooleanTestableImpl =
std::convertible_to<T, bool>;

template <typename T>
concept BooleanTestable =
	BooleanTestableImpl<T> &&
	requires (T && b) {
		{!std::forward<T>(b)} ->  BooleanTestableImpl;
	};
	

#define LOG(format, ...) \
	fprintf(stdout, "\33[38;5;75m"); \
	fprintf(stdout, format, __VA_ARGS__); \
	fprintf(stdout, "\33[0m");

#define LOG_ERROR(format, ...) \
	fprintf(stderr, "\n\n"); \
	fprintf(stderr, "\33[38;5;196m"); \
	fprintf(stderr, "Line: %d\n", __LINE__); \
	fprintf(stderr, "File: %s\n\n", __FILE__); \
	fprintf(stderr, format, __VA_ARGS__); \
	fprintf(stderr, "\n\n"); \
	fprintf(stderr, "\33[0m");