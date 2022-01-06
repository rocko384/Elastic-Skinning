#pragma once

#include <concepts>
#include <type_traits>
#include <utility>
#include <cstdio>

template <typename VALUE_TYPE, typename STATUS_TYPE>
struct Retval {
	VALUE_TYPE value;
	STATUS_TYPE status;
};

using StringHash = size_t;

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