/*******************************************************************************
module:   strutils
author:   digimokan
date:     31 JUL 2017 (created)
purpose:  various c-string utility functions
*******************************************************************************/

/*******************************************************************************
 * SYSTEM INCLUDES
 ******************************************************************************/

#include <stddef.h>     // ISOC: NULL, size_t
#include <stdbool.h>    // ISOC: bool, true, false
#include <string.h>     // ISOC: strlen()
#include <ctype.h>      // ISOC: isspace()
#include <stdio.h>      // ISOC: snprintf()
#include <stdlib.h>     // ISOC: malloc()

/*******************************************************************************
 * USER INCLUDES
 ******************************************************************************/

#include "strutils.h"

/*******************************************************************************
 * PRIVATE MACROS / FUNCTIONS
 ******************************************************************************/

/*******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

// return char* to pos just after last non-space char in str
char* str_find_stripped_end (char* str) {
  char* stripped_end = str;

  for (char* p = str; *p != '\0'; p++) {
    if (!isspace(*p)) {
      stripped_end = p + 1;
    }
  }

  return stripped_end;
}

// remove trailing spaces from str, return the same str (now rstripped)
char* str_rstrip (char* str) {
  *(str_find_stripped_end(str)) = '\0';
  return str;
}

// return true if str starts with starting_substr
bool str_startswith (const char* str, const char* starting_substr) {
  return ( strncmp(str, starting_substr, strlen(starting_substr)) == 0 );
}

// return true if str ends with ending_substr
bool str_endswith (const char* str, const char* ending_substr) {
  size_t len_str = strlen(str);
  size_t len_es = strlen(ending_substr);

  // early out if str smaller than ending_substr
  if (len_str < len_es) {
    return false;
  }

  return ( strncmp(str + (len_str - len_es), ending_substr, len_es) == 0 );
}

// return strlen of input int
size_t get_strlen_of_int (int num) {
  return ((size_t) (snprintf(NULL, 0, "%d", num)));
}

// malloc new str from input int
StrAction malloc_str_from_int (char** new_str, int num) {
  const size_t strsize = get_strlen_of_int(num) + 1;
  *new_str = (char*) malloc(strsize);

  if (*new_str == NULL)
    return (ERR_STR_MALLOC_FAIL);

  snprintf(*new_str, strsize, "%d", num);
  return (SUCCESS_STR_MALLOCED);
}

// malloc new str from two concatenated input ints
StrAction malloc_str_from_two_ints (char** new_str, int num1, int num2) {
  char* str1 = NULL;
  char* str2 = NULL;

  malloc_str_from_int(&str1, num1);
  malloc_str_from_int(&str2, num2);
  StrAction act = malloc_str_from_two_strs(new_str, str1, str2);

  free(str1);
  free(str2);

  return act;
}

// malloc new str from two concatenated input strs
StrAction malloc_str_from_two_strs (char** new_str, char* str1, char* str2) {
  if (str1 == NULL)
    return (ERR_STR1_IS_NULL);
  if (str2 == NULL)
    return (ERR_STR2_IS_NULL);

  const size_t new_str_size = strlen(str1) + strlen(str2) + 1;

  *new_str = (char*) malloc(new_str_size);
  if (*new_str == NULL)
    return (ERR_STR_MALLOC_FAIL);

  StrAction act1 = str_copy_src_to_dest(*new_str, new_str_size, str1);
  StrAction act2 = str_cat_src_to_dest(*new_str, new_str_size, str2);

  if ( (act1 == SUCCESS_STR_COPIED) && (act2 == SUCCESS_STR_CATED) )
    return (SUCCESS_STR_MALLOCED);
  else if (act1 == SUCCESS_STR_COPIED)
    return (act2);
  else
    return act1;
}

// malloc new str from inut str concatenated with input int
StrAction malloc_str_from_str_plus_int (char** new_str, char* str, int num) {
  char* num_str;
  StrAction act1 = malloc_str_from_int(&num_str, num);

  if (act1 == SUCCESS_STR_MALLOCED) {
    StrAction act2 =  malloc_str_from_two_strs(new_str, str, num_str);
    free(num_str);
    return act2;
  } else {
    return act1;
  }
}

// copy source string to destination string
StrAction str_cat_src_to_dest (char* dest, size_t dest_size, char* src) {
  if (src == NULL)
    return (ERR_SRC_IS_NULL);
  if (dest == NULL)
    return (ERR_DEST_IS_NULL);

  bool found_null_term = false;
  for (int i = 0; i < dest_size; i++) {
    if (dest[i] == '\0') {
      found_null_term = true;
    }
  }
  if (!found_null_term)
    return (ERR_DEST_NOT_STRING);

  const size_t srclen = strlen(src);
  const size_t destlen = strlen(dest);

  if ((srclen + destlen) < dest_size) {
    memcpy(dest + destlen, src, srclen);
    dest[srclen + destlen] = '\0';
  } else {
    memcpy(dest + destlen, src, dest_size - destlen - 1);
    dest[dest_size - 1] = '\0';
    return SUCCESS_OUT_OF_SPACE_IN_DEST;
  }

  return SUCCESS_STR_CATED;
}

// copy source string to destination string
StrAction str_copy_src_to_dest (char* dest, size_t dest_size, char* src) {
  if (src == NULL)
    return (ERR_SRC_IS_NULL);
  if (dest == NULL)
    return (ERR_DEST_IS_NULL);

  const size_t srclen = strlen(src);

  if (srclen < dest_size) {
    memcpy(dest, src, srclen);
    dest[srclen] = '\0';
  } else {
    memcpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
    return SUCCESS_OUT_OF_SPACE_IN_DEST;
  }

  return SUCCESS_STR_COPIED;
}

