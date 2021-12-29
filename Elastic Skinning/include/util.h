#pragma once

#include <cstdio>

template <typename VALUE_TYPE, typename STATUS_TYPE>
struct Retval {
	VALUE_TYPE value;
	STATUS_TYPE status;
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