/*******************************************************************************
module:   strutils
author:   digimokan
date:     31 JUL 2017 (created)
purpose:  various c-string utility functions
*******************************************************************************/

#ifndef STRUTILS_H
#define STRUTILS_H 1

// allow this header to be included from c++ source file
#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * SYSTEM INCLUDES
 ******************************************************************************/

#include <stdbool.h>    // ISOC: bool
#include <stddef.h>     // ISOC: size_t

/*******************************************************************************
 * PUBLIC TYPES / CONSTANTS / VARIABLES
 ******************************************************************************/

typedef enum {
  SUCCESS_STR_MALLOCED,
  SUCCESS_STR_CATED,
  SUCCESS_STR_COPIED,
  SUCCESS_OUT_OF_SPACE_IN_DEST,
  ERR_STR_MALLOC_FAIL,
  ERR_SRC_IS_NULL,
  ERR_DEST_IS_NULL,
  ERR_STR1_IS_NULL,
  ERR_STR2_IS_NULL,
  ERR_DEST_NOT_STRING,
  ERR_UNKNOWN_STRUTILS_ERR
} StrAction;

/*******************************************************************************
 * PUBLIC MACROS / FUNCTIONS
 ******************************************************************************/

// return char* to pos just after last non-space char in str
char* str_find_stripped_end (char* str);

// remove trailing spaces from str, return the same str (now rstripped)
char* str_rstrip (char* str);

// return true if str starts with starting_substr
bool str_startswith (const char* str, const char* starting_substr);

// return true if str ends with ending_substr
bool str_endswith (const char* str, const char* ending_substr);

// return strlen of input int
size_t get_strlen_of_int (int num);

// malloc new str from input int
StrAction malloc_str_from_int (char** new_str, int num);

// malloc new str from two concatenated input ints
StrAction malloc_str_from_two_ints (char** new_str, int num1, int num2);

// malloc new str from two concatenated input strs
StrAction malloc_str_from_two_strs (char** new_str, char* str1, char* str2);

// malloc new str from inut str concatenated with input int
StrAction malloc_str_from_str_plus_int (char** new_str, char* str, int num);

// copy source string to destination string
StrAction str_cat_src_to_dest (char* dest, size_t dest_size, char* src);

// copy source string to destination string
StrAction str_copy_src_to_dest (char* dest, size_t dest_size, char* src);

// allow this header to be included from c++ source file
#ifdef __cplusplus
}
#endif

#endif // STRUTILS_H

