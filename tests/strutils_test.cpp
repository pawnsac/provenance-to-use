/*******************************************************************************
module:   strutils_test
author:   digimokan
date:     29 AUG 2017 (created)
purpose:  test cases for functions in strace-4.6/strutils.c
*******************************************************************************/

#include "doctest.h"
#include "strutils.h"

#include <cstring>      // ISOC: strlen()
#include <cstdbool>     // ISOC: bool, true, false
#include <cstdlib>      // ISOC: free()

TEST_CASE("str_find_stripped_end") {

  SUBCASE("empty string") {
    char instr[] = "";
    char* stripend = str_find_stripped_end(instr);
    CHECK(*stripend == '\0');  // stripped end
  }

  SUBCASE("one-word string, no whitespace") {
    char instr[] = "foo";
    char* stripend = str_find_stripped_end(instr);
    CHECK(strncmp(instr, "foo", strlen(instr)) == 0);  // input not mutated
    CHECK(*stripend == '\0');  // stripped end
  }

  SUBCASE("one-word string, trailing whitespace") {
    char instr[] = "foo  ";
    char* stripend = str_find_stripped_end(instr);
    CHECK(strncmp(instr, "foo  ", strlen(instr)) == 0);  // input not mutated
    CHECK(strncmp(stripend, "  ", 2) == 0);  // stripped end
  }

  SUBCASE("two-word string, no whitespace") {
    char instr[] = "foo bar";
    char* stripend = str_find_stripped_end(instr);
    CHECK(strncmp(instr, "foo bar", strlen(instr)) == 0);  // input not mutated
    CHECK(*stripend == '\0');  // stripped end
  }

  SUBCASE("two-word string, trailing whitespace") {
    char instr[] = "foo bar  ";
    char* stripend = str_find_stripped_end(instr);
    CHECK(strncmp(instr, "foo bar  ", strlen(instr)) == 0);  // input not mutated
    CHECK(strncmp(stripend, "  ", 2) == 0);  // stripped end
  }

  SUBCASE("three-word string, no whitespace") {
    char instr[] = "foo bar zap";
    char* stripend = str_find_stripped_end(instr);
    CHECK(strncmp(instr, "foo bar zap", strlen(instr)) == 0);  // input not mutated
    CHECK(*stripend == '\0');  // stripped end
  }

  SUBCASE("three-word string, trailing whitespace") {
    char instr[] = "foo bar zap  ";
    char* stripend = str_find_stripped_end(instr);
    CHECK(strncmp(instr, "foo bar zap  ", strlen(instr)) == 0);  // input not mutated
    CHECK(strncmp(stripend, "  ", 2) == 0);  // stripped end
  }

  SUBCASE("multi-line string, no whitespace") {
    char instr[] = "foo bar zap\ngrok lom tux";
    char* stripend = str_find_stripped_end(instr);
    CHECK(strncmp(instr, "foo bar zap\ngrok lom tux", strlen(instr)) == 0);  // input not mutated
    CHECK(*stripend == '\0');  // stripped end
  }

  SUBCASE("multi-line string, trailing whitespace") {
    char instr[] = "foo bar zap\ngrok lom tux  ";
    char* stripend = str_find_stripped_end(instr);
    CHECK(strncmp(instr, "foo bar zap\ngrok lom tux  ", strlen(instr)) == 0);  // input not mutated
    CHECK(strncmp(stripend, "  ", 2) == 0);  // stripped end
  }

  SUBCASE("tabbed string, no whitespace") {
    char instr[] = "foo bar zap\tgrok lom tux";
    char* stripend = str_find_stripped_end(instr);
    CHECK(strncmp(instr, "foo bar zap\tgrok lom tux", strlen(instr)) == 0);  // input not mutated
    CHECK(*stripend == '\0');  // stripped end
  }

  SUBCASE("tabbed string, trailing whitespace") {
    char instr[] = "foo bar zap\tgrok lom tux  ";
    char* stripend = str_find_stripped_end(instr);
    CHECK(strncmp(instr, "foo bar zap\tgrok lom tux  ", strlen(instr)) == 0);  // input not mutated
    CHECK(strncmp(stripend, "  ", 2) == 0);  // stripped end
  }

}

TEST_CASE("str_rstrip") {

  SUBCASE("empty string") {
    char instr[] = "";
    char* rstripped = str_rstrip(instr);
    CHECK(instr == rstripped);  // return char* == input char*
    CHECK(*rstripped == '\0');  // stripped input
  }

  SUBCASE("one-word string, no whitespace") {
    char instr[] = "foo";
    size_t inlen = strlen(instr);
    char* rstripped = str_rstrip(instr);
    CHECK(instr == rstripped);  // return char* == input char*
    CHECK(strncmp(rstripped, "foo", inlen) == 0);  // stripped input
  }

  SUBCASE("one-word string, trailing whitespace") {
    char instr[] = "foo  ";
    size_t inlen = strlen(instr);
    char* rstripped = str_rstrip(instr);
    CHECK(instr == rstripped);  // return char* == input char*
    CHECK(strncmp(rstripped, "foo", inlen) == 0);  // stripped input
  }

  SUBCASE("three-word string, trailing whitespace") {
    char instr[] = "foo bar zap  ";
    size_t inlen = strlen(instr);
    char* rstripped = str_rstrip(instr);
    CHECK(instr == rstripped);  // return char* == input char*
    CHECK(strncmp(rstripped, "foo bar zap", inlen) == 0);  // stripped input
  }

  SUBCASE("multi-line string, trailing whitespace") {
    char instr[] = "foo bar zap\ngrok lom tux  ";
    size_t inlen = strlen(instr);
    char* rstripped = str_rstrip(instr);
    CHECK(instr == rstripped);  // return char* == input char*
    CHECK(strncmp(rstripped, "foo bar zap\ngrok lom tux", inlen) == 0);  // stripped input
  }

}

TEST_CASE("str_startswith") {

  SUBCASE("empty string starts with empty string") {
    char instr[] = "";
    bool swresult = str_startswith(instr, "");
    CHECK(swresult == true);
  }

  SUBCASE("empty string NOT start with some word") {
    char instr[] = "";
    char endstr[] = "foo";
    bool swresult = str_startswith(instr, endstr);
    CHECK(swresult == false);
  }

  SUBCASE("one-word string starts with same word") {
    char instr[] = "foo";
    bool swresult = str_startswith(instr, "foo");
    CHECK(swresult == true);
  }

  SUBCASE("one-word string NOT start with different word") {
    char instr[] = "foo";
    bool swresult = str_startswith(instr, "bar");
    CHECK(swresult == false);
  }

  SUBCASE("three-word string starts with some word") {
    char instr[] = "foo bar zap";
    bool swresult = str_startswith(instr, "foo");
    CHECK(swresult == true);
  }

  SUBCASE("three-word string NOT start with different word") {
    char instr[] = "foo bar zap";
    bool swresult = str_startswith(instr, "bar");
    CHECK(swresult == false);
  }

  SUBCASE("three-word string starts with empty string") {
    char instr[] = "foo bar zap";
    bool swresult = str_startswith(instr, "");
    CHECK(swresult == true);
  }

  SUBCASE("multi-line string starts with some word") {
    char instr[] = "foo bar zap\ngrok lom tux";
    bool swresult = str_startswith(instr, "foo");
    CHECK(swresult == true);
  }

  SUBCASE("multi-line string NOT start with different word") {
    char instr[] = "foo bar zap\ngrok lom tux";
    bool swresult = str_startswith(instr, "grok");
    CHECK(swresult == false);
  }

}

TEST_CASE("str_endswith") {

  SUBCASE("empty string ends with empty string") {
    char instr[] = "";
    bool ewresult = str_endswith(instr, "");
    CHECK(ewresult == true);
  }

  SUBCASE("empty string NOT end with some word") {
    char instr[] = "";
    char endstr[] = "foo";
    bool ewresult = str_endswith(instr, endstr);
    CHECK(ewresult == false);
  }

  SUBCASE("one-word string ends with same word") {
    char instr[] = "foo";
    bool ewresult = str_endswith(instr, "foo");
    CHECK(ewresult == true);
  }

  SUBCASE("one-word string NOT end with different word") {
    char instr[] = "foo";
    bool ewresult = str_endswith(instr, "bar");
    CHECK(ewresult == false);
  }

  SUBCASE("three-word string ends with some word") {
    char instr[] = "foo bar zap";
    bool ewresult = str_endswith(instr, "zap");
    CHECK(ewresult == true);
  }

  SUBCASE("three-word string NOT end with different word") {
    char instr[] = "foo bar zap";
    bool ewresult = str_endswith(instr, "foo");
    CHECK(ewresult == false);
  }

  SUBCASE("three-word string ends with empty string") {
    char instr[] = "foo bar zap";
    bool ewresult = str_endswith(instr, "");
    CHECK(ewresult == true);
  }

  SUBCASE("multi-line string ends with some word") {
    char instr[] = "foo bar zap\ngrok lom tux";
    bool ewresult = str_endswith(instr, "tux");
    CHECK(ewresult == true);
  }

  SUBCASE("multi-line string NOT end with different word") {
    char instr[] = "foo bar zap\ngrok lom tux";
    bool ewresult = str_endswith(instr, "zap");
    CHECK(ewresult == false);
  }

}

TEST_CASE("get_strlen_of_int") {

  SUBCASE("single-digit ints") {
    CHECK(get_strlen_of_int(0) == 1);
    CHECK(get_strlen_of_int(1) == 1);
    CHECK(get_strlen_of_int(2) == 1);
    CHECK(get_strlen_of_int(3) == 1);
  }

  SUBCASE("two-digit ints") {
    CHECK(get_strlen_of_int(10) == 2);
    CHECK(get_strlen_of_int(50) == 2);
    CHECK(get_strlen_of_int(99) == 2);
  }

  SUBCASE("three-digit ints") {
    CHECK(get_strlen_of_int(100) == 3);
    CHECK(get_strlen_of_int(500) == 3);
    CHECK(get_strlen_of_int(999) == 3);
  }

  SUBCASE("negative single-digit ints") {
    CHECK(get_strlen_of_int(-1) == 2);
    CHECK(get_strlen_of_int(-2) == 2);
    CHECK(get_strlen_of_int(-9) == 2);
  }

  SUBCASE("negative three-digit ints") {
    CHECK(get_strlen_of_int(-100) == 4);
    CHECK(get_strlen_of_int(-500) == 4);
    CHECK(get_strlen_of_int(-999) == 4);
  }

}

TEST_CASE("malloc_str_from_int") {

  StrAction act;
  char* new_str = NULL;

  SUBCASE("single-digit int") {
    act = malloc_str_from_int(&new_str, 0);

    CHECK(act == SUCCESS_STR_MALLOCED);
    CHECK(strncmp(new_str, "0", 1) == 0);

    free(new_str);
  }

  SUBCASE("two-digit int") {
    act = malloc_str_from_int(&new_str, 99);

    CHECK(act == SUCCESS_STR_MALLOCED);
    CHECK(strncmp(new_str, "99", 2) == 0);

    free(new_str);
  }

  SUBCASE("negative int") {
    act = malloc_str_from_int(&new_str, -1000);

    CHECK(act == SUCCESS_STR_MALLOCED);
    CHECK(strncmp(new_str, "-1000", 5) == 0);

    free(new_str);
  }

}

TEST_CASE("malloc_str_from_two_ints") {

  StrAction act;
  char* new_str = NULL;

  SUBCASE("two single-digit ints") {
    act = malloc_str_from_two_ints(&new_str, 0, 0);
    CHECK(act == SUCCESS_STR_MALLOCED);
    CHECK(strncmp(new_str, "00", 2) == 0);

    act = malloc_str_from_two_ints(&new_str, 0, 1);
    CHECK(act == SUCCESS_STR_MALLOCED);
    CHECK(strncmp(new_str, "01", 2) == 0);

    act = malloc_str_from_two_ints(&new_str, 1, 0);
    CHECK(act == SUCCESS_STR_MALLOCED);
    CHECK(strncmp(new_str, "10", 2) == 0);

    free(new_str);
  }

  SUBCASE("one single-digit and one two-digit int") {
    act = malloc_str_from_two_ints(&new_str, 1, 99);
    CHECK(act == SUCCESS_STR_MALLOCED);
    CHECK(strncmp(new_str, "199", 3) == 0);

    act = malloc_str_from_two_ints(&new_str, 99, 1);
    CHECK(act == SUCCESS_STR_MALLOCED);
    CHECK(strncmp(new_str, "991", 3) == 0);

    free(new_str);
  }

  SUBCASE("one positive int and one negative int") {
    act = malloc_str_from_two_ints(&new_str, 1, -1);
    CHECK(act == SUCCESS_STR_MALLOCED);
    CHECK(strncmp(new_str, "1-1", 3) == 0);

    act = malloc_str_from_two_ints(&new_str, -1, 1);
    CHECK(act == SUCCESS_STR_MALLOCED);
    CHECK(strncmp(new_str, "-11", 3) == 0);

    act = malloc_str_from_two_ints(&new_str, 999, -11);
    CHECK(act == SUCCESS_STR_MALLOCED);
    CHECK(strncmp(new_str, "999-11", 6) == 0);

    act = malloc_str_from_two_ints(&new_str, -11, 999);
    CHECK(act == SUCCESS_STR_MALLOCED);
    CHECK(strncmp(new_str, "-11999", 6) == 0);

    free(new_str);
  }

}

TEST_CASE("malloc_str_from_two_strs") {

  StrAction act;

  SUBCASE("str1 is NULL") {
    char* new_str = NULL;
    char* str1 = NULL;
    char str2[] = "bar zap";

    act = malloc_str_from_two_strs(&new_str, str1, str2);

    CHECK(act == ERR_STR1_IS_NULL);
  }

  SUBCASE("str2 is NULL") {
    char* new_str = NULL;
    char str1[] = "bar zap";
    char* str2 = NULL;

    act = malloc_str_from_two_strs(&new_str, str1, str2);

    CHECK(act == ERR_STR2_IS_NULL);
  }

  SUBCASE("str1 is null string") {
    char* new_str = NULL;
    char str1[] = "";
    char str2[] = "bar zap";

    act = malloc_str_from_two_strs(&new_str, str1, str2);

    CHECK(act == SUCCESS_STR_MALLOCED);
    CHECK(strlen(str1) == 0);
    CHECK(strncmp(str2, "bar zap", 7) == 0);
    CHECK(strlen(str2) == 7);
    CHECK(strncmp(new_str, "bar zap", 7) == 0);
    CHECK(strlen(new_str) == 7);
  }

  SUBCASE("str1 is null string") {
    char* new_str = NULL;
    char str1[] = "bar zap";
    char str2[] = "";

    act = malloc_str_from_two_strs(&new_str, str1, str2);

    CHECK(act == SUCCESS_STR_MALLOCED);
    CHECK(strncmp(str1, "bar zap", 7) == 0);
    CHECK(strlen(str1) == 7);
    CHECK(strlen(str2) == 0);
    CHECK(strncmp(new_str, "bar zap", 7) == 0);
    CHECK(strlen(new_str) == 7);
  }

  SUBCASE("str1 and str2 are valid strings") {
    char* new_str = NULL;
    char str1[] = "bar zap";
    char str2[] = "foo";

    act = malloc_str_from_two_strs(&new_str, str1, str2);

    CHECK(act == SUCCESS_STR_MALLOCED);
    CHECK(strncmp(str1, "bar zap", 7) == 0);
    CHECK(strlen(str1) == 7);
    CHECK(strncmp(str2, "foo", 3) == 0);
    CHECK(strlen(str2) == 3);
    CHECK(strncmp(new_str, "bar zapfoo", 10) == 0);
    CHECK(strlen(new_str) == 10);
  }

}

TEST_CASE("malloc_str_from_str_plus_int") {

  StrAction act;

  SUBCASE("str is NULL") {
    char* new_str = NULL;
    char* str1 = NULL;
    int num = 9;

    act = malloc_str_from_str_plus_int(&new_str, str1, num);

    CHECK(act == ERR_STR1_IS_NULL);
  }

  SUBCASE("str is null string") {
    char* new_str = NULL;
    char str1[] = "";
    int num = 9;

    act = malloc_str_from_str_plus_int(&new_str, str1, num);

    CHECK(act == SUCCESS_STR_MALLOCED);
    CHECK(strlen(str1) == 0);
    CHECK(strncmp(new_str, "9", 1) == 0);
    CHECK(strlen(new_str) == 1);
  }

  SUBCASE("str1 and str2 are valid strings") {
    char* new_str = NULL;
    char str1[] = "bar zap";
    int num = 9999;

    act = malloc_str_from_str_plus_int(&new_str, str1, num);

    CHECK(act == SUCCESS_STR_MALLOCED);
    CHECK(strncmp(str1, "bar zap", 7) == 0);
    CHECK(strlen(str1) == 7);
    CHECK(strncmp(new_str, "bar zap9999", 11) == 0);
    CHECK(strlen(new_str) == 11);
  }

}

TEST_CASE("str_cat_src_to_dest") {

  StrAction act;

  SUBCASE("source str is NULL") {
    char* src = NULL;
    char dest[] = "bar zap";
    size_t dest_size = strlen(dest) + 1;

    act = str_cat_src_to_dest(dest, dest_size, src);

    CHECK(act == ERR_SRC_IS_NULL);
  }

  SUBCASE("dest str is NULL") {
    char src[] = "foo";
    char* dest = NULL;
    size_t dest_size = 0;

    act = str_cat_src_to_dest(dest, dest_size, src);

    CHECK(act == ERR_DEST_IS_NULL);
  }

  SUBCASE("source str is null string") {
    char src[] = "";
    char dest[] = "bar zap";
    size_t dest_size = strlen(dest) + 1;

    act = str_cat_src_to_dest(dest, dest_size, src);

    CHECK(act == SUCCESS_STR_CATED);
    CHECK(strlen(src) == 0);
    CHECK(strncmp(dest, "bar zap", 7) == 0);
    CHECK(strlen(dest) == 7);
  }

  SUBCASE("dest str is null string") {
    char src[] = "foo";
    char dest[] = "";
    size_t dest_size = 1;

    act = str_copy_src_to_dest(dest, dest_size, src);

    CHECK(act == SUCCESS_OUT_OF_SPACE_IN_DEST);
    CHECK(strncmp(src, "foo", 3) == 0);
    CHECK(strlen(src) == 3);
    CHECK(strlen(dest) == 0);
  }

  SUBCASE("dest is not valid null-terminated string") {
    char src[] = "foo";
    char dest[] = "";
    dest[0] = 1;
    size_t dest_size = 1;

    act = str_cat_src_to_dest(dest, dest_size, src);

    CHECK(act == ERR_DEST_NOT_STRING);

    char dest2[] = "fo";
    size_t dest2_size = 3;
    dest2[0] = 1;
    dest2[1] = 1;
    dest2[2] = 1;

    act = str_cat_src_to_dest(dest2, dest2_size, src);

    CHECK(act == ERR_DEST_NOT_STRING);
  }

  SUBCASE("source str does not fit when appended in dest") {
    char src[] = "foo";
    char dest[] = "bar zap";
    size_t dest_size = strlen(dest) + 1;
    dest[6] = '\0';

    act = str_cat_src_to_dest(dest, dest_size, src);

    CHECK(act == SUCCESS_OUT_OF_SPACE_IN_DEST);
    CHECK(strncmp(dest, "bar zaf", 7) == 0);
    CHECK(strlen(dest) == 7);
    CHECK(strncmp(src, "foo", 3) == 0);
    CHECK(strlen(src) == 3);
  }

  SUBCASE("source str exactly fits when appended in dest") {
    char src[] = "foo";
    char dest[] = "bar zap";
    size_t dest_size = strlen(dest) + 1;
    dest[4] = '\0';

    act = str_cat_src_to_dest(dest, dest_size, src);

    CHECK(act == SUCCESS_STR_CATED);
    CHECK(strncmp(dest, "bar foo", 7) == 0);
    CHECK(strlen(dest) == 7);
    CHECK(strncmp(src, "foo", 3) == 0);
    CHECK(strlen(src) == 3);
  }

  SUBCASE("source str fits when appended in dest, with leftover room in dest") {
    char src[] = "foo";
    char dest[] = "bar zap grok";
    size_t dest_size = strlen(dest) + 1;
    dest[4] = '\0';

    act = str_cat_src_to_dest(dest, dest_size, src);

    CHECK(act == SUCCESS_STR_CATED);
    CHECK(strncmp(dest, "bar foo", 7) == 0);
    CHECK(strlen(dest) == 7);
    CHECK(strncmp(src, "foo", 3) == 0);
    CHECK(strlen(src) == 3);
  }

}

TEST_CASE("str_copy_src_to_dest") {

  StrAction act;

  SUBCASE("source str is NULL") {
    char* src = NULL;
    char dest[] = "bar zap";
    size_t dest_size = strlen(dest) + 1;

    act = str_copy_src_to_dest(dest, dest_size, src);

    CHECK(act == ERR_SRC_IS_NULL);
  }

  SUBCASE("dest str is NULL") {
    char src[] = "foo";
    char* dest = NULL;
    size_t dest_size = 0;

    act = str_copy_src_to_dest(dest, dest_size, src);

    CHECK(act == ERR_DEST_IS_NULL);
  }

  SUBCASE("source str is null string") {
    char src[] = "";
    char dest[] = "bar zap";
    size_t dest_size = strlen(dest) + 1;

    act = str_copy_src_to_dest(dest, dest_size, src);

    CHECK(act == SUCCESS_STR_COPIED);
    CHECK(strlen(src) == 0);
    CHECK(strlen(dest) == 0);
  }

  SUBCASE("dest str is null string") {
    char src[] = "foo";
    char dest[] = "";
    size_t dest_size = 1;

    act = str_copy_src_to_dest(dest, dest_size, src);

    CHECK(act == SUCCESS_OUT_OF_SPACE_IN_DEST);
    CHECK(strncmp(src, "foo", 3) == 0);
    CHECK(strlen(src) == 3);
    CHECK(strlen(dest) == 0);
  }

  SUBCASE("source str len > dest size") {
    char src[] = "bar zap";
    char dest[] = "foo";
    size_t dest_size = strlen(dest) + 1;

    act = str_copy_src_to_dest(dest, dest_size, src);

    CHECK(act == SUCCESS_OUT_OF_SPACE_IN_DEST);
    CHECK(strncmp(dest, "bar", 3) == 0);
    CHECK(strlen(dest) == 3);
    CHECK(strncmp(src, "bar zap", 7) == 0);
    CHECK(strlen(src) == 7);
  }

  SUBCASE("source str len == (dest size - 1)") {
    char src[] = "foo";
    char dest[] = "bar";
    size_t dest_size = strlen(dest) + 1;

    act = str_copy_src_to_dest(dest, dest_size, src);

    CHECK(act == SUCCESS_STR_COPIED);
    CHECK(strncmp(dest, "foo", 3) == 0);
    CHECK(strlen(dest) == 3);
    CHECK(strncmp(src, "foo", 3) == 0);
    CHECK(strlen(src) == 3);
  }

  SUBCASE("source str len < (dest size - 1)") {
    char src[] = "foo";
    char dest[] = "bar zap";
    size_t dest_size = strlen(dest) + 1;

    act = str_copy_src_to_dest(dest, dest_size, src);

    CHECK(act == SUCCESS_STR_COPIED);
    CHECK(strncmp(dest, "foo", 3) == 0);
    CHECK(strlen(dest) == 3);
    CHECK(strncmp(src, "foo", 3) == 0);
    CHECK(strlen(src) == 3);
  }

}

