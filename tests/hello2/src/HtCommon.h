#pragma once

typedef char Char_t;

struct Host_t {
	short b;
	uint16_t pad1;
	int a;
	char c[5];
	uint8_t pad2[3];
	uint32_t d;
};

typedef Host_t Host2_t;
