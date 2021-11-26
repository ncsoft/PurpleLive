#pragma once

#ifdef _MSC_VER
#pragma warning(disable : 4152) /* casting func ptr to void */
#endif

#include <stdint.h>
#include <stdbool.h>
#include <windows.h>

#define LOWER_HALFBYTE(x) ((x)&0xF)
#define UPPER_HALFBYTE(x) (((x) >> 4) & 0xF)

static void decrypt_str(char *str, uint64_t val)
{
	uint8_t *dec_val = (uint8_t *)&val;
	int i = 0;

	while (*str != 0) {
		int pos = i / 2;
		bool bottom = (i % 2) == 0;
		uint8_t *ch = (uint8_t *)str;
		uint8_t _xor = bottom ? LOWER_HALFBYTE(dec_val[pos]) : UPPER_HALFBYTE(dec_val[pos]);

		*ch ^= _xor;

		if (++i == sizeof(uint64_t) * 2)
			i = 0;

		str++;
	}
}

static void* get_encrypted_func(HMODULE module, const char *str, uint64_t val)
{
	char new_name[128];
	strcpy(new_name, str);
	decrypt_str(new_name, val);
	return GetProcAddress(module, new_name);
}
