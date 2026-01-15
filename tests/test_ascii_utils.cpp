/**
 *
 *  @file test_ascii.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */
#include <vix/utils/ascii_utils.hpp>
#include <cassert>

using namespace vix::utils;

int main()
{
  // is_printable_ascii(char c): ascii 32 -> 126
  assert(ascii::is_printable_ascii(' ') == true);
  assert(ascii::is_printable_ascii('A') == true);
  assert(ascii::is_printable_ascii('z') == true);
  assert(ascii::is_printable_ascii('0') == true);
  assert(ascii::is_printable_ascii('~') == true);
  // false tests
  assert(ascii::is_printable_ascii('\n') == false);  // 10
  assert(ascii::is_printable_ascii('\t') == false);  // 9
  assert(ascii::is_printable_ascii('x7f') == false); // 127
  assert(ascii::is_printable_ascii('x01') == false); // 1
  // is_digit_ascii(char c) :ascii 48 -> 57
  assert(ascii::is_digit_ascii('0') == true);
  assert(ascii::is_digit_ascii('5') == true);
  assert(ascii::is_digit_ascii('9') == true);
  // false tests
  assert(ascii::is_digit_ascii('a') == false);
  assert(ascii::is_digit_ascii(' ') == false);
  assert(ascii::is_digit_ascii('~') == false);
  assert(ascii::is_digit_ascii('\n') == false);
  // is_alpha_ascii(char c) : ascii A: 65 -> Z: 90 || a: 97 -> z: 122
  assert(ascii::is_alpha_ascii('A') == true);
  assert(ascii::is_alpha_ascii('Z') == true);
  assert(ascii::is_alpha_ascii('a') == true);
  assert(ascii::is_alpha_ascii('z') == true);
  // false tests
  assert(ascii::is_alpha_ascii('0') == false);
  assert(ascii::is_alpha_ascii(' ') == false);
  assert(ascii::is_alpha_ascii('~') == false);
  // is_upper_ascii(char c): ascii A: 65 -> Z: 90
  assert(ascii::is_upper_ascii('A') == true);
  assert(ascii::is_upper_ascii('Z') == true);
  // false tests
  assert(ascii::is_upper_ascii('a') == false);
  assert(ascii::is_upper_ascii('o') == false);
  assert(ascii::is_upper_ascii('~') == false);
  // is_lower_ascii(char c): ascii a:97 -> z:122
  assert(ascii::is_lower_ascii('a') == true);
  assert(ascii::is_lower_ascii('z') == true);
  // false tests
  assert(ascii::is_lower_ascii('A') == false);
  assert(ascii::is_lower_ascii('Z') == false);
  assert(ascii::is_lower_ascii('0') == false);
  assert(ascii::is_lower_ascii('~') == false);
  // to_upper_ascii(char c) : is_upper_ascii(c) ? c - 32 : c;
  assert(ascii::to_upper_ascii('a') == 'A');
  assert(ascii::to_upper_ascii('z') == 'Z');
  assert(ascii::to_upper_ascii('A') == 'A'); // inchange
  assert(ascii::to_upper_ascii('!') == '!'); // inchange
  assert(ascii::to_upper_ascii('0') == '0'); // inchange
  // to_lower_ascii(char c): is_lower_ascii(c) ? c - ('a' - 'A') : c;
  assert(ascii::to_lower_ascii('A') == 'a');
  assert(ascii::to_lower_ascii('Z') == 'z');
  assert(ascii::to_lower_ascii('a') == 'a');
  assert(ascii::to_lower_ascii('!') == '!');
  assert(ascii::to_lower_ascii('0') == '0');
  // ascii_code(char c): 'A' => 65, 'a' => 97, '0' => 48
  assert(ascii::ascii_code('A') == 65);
  assert(ascii::ascii_code('a') == 97);
  assert(ascii::ascii_code('0') == 48);
  assert(ascii::ascii_code(' ') == 32);
  assert(ascii::ascii_code('~') == 126);
  // print_ascii_table(char) // affiche le table dans 16 colonnes ou 10 colonnes
  ascii::print_ascii_table(16);
  ascii::print_ascii_table(10);
}
