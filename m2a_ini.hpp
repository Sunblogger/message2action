#ifndef _m2a_ini_file_class_include
#define _m2a_ini_file_class_include

/*
 m2a_ini.hpp
 Release: 0.1, date: 23.05.2021
  
 
 * Copyright 2021
 * 
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This class is derived from ini_file_class and specific for our program message2action
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class m2a_ini_file_class : public ini_file_class
{
public:
// constructor with one parameter: the file-name which contains the ini-data
m2a_ini_file_class(const std::string file_name) : ini_file_class(file_name) { 	// we call constructor from base-class (initialize ini_file_name and read the file)
													setup_variable_map(); 		// we setup a map of variables specific for our program
													inifile_return_code = read_ini_file(); 	// we try to read the ini-file for our program
													if (0 == inifile_return_code) {
															inifile_return_code = fill_variables(); // we fill our map with content from our ini-file
															if (0 == inifile_return_code) {
																bool ini_file_capitalize_values_flag;
																if (true == get_value("capitalize_values", &ini_file_capitalize_values_flag)) { // if we have found a value for parameter "capitalize_values"
																	if (true == ini_file_capitalize_values_flag) { // if we have to upcase all strings we do so
																		upcase_all_string_values();
																	}
																}
															}
														}
												}	// end of constructor 	

void check_values_plausability();	// checks the plausibility of the values we have found in ini-file	
void print_ini_file_content() { setup_ini_file_content(&temp_string_buffer); std::cout << temp_string_buffer << std::endl; };	// prints the content of the ini-file on screen	 
void setup_ini_file_content(std::string *config); 	// we overload the function from our base-class

private:
void setup_variable_map();		// sets up the map for all variables we have to search for in ini-file.	
unsigned get_hexvalue_from_string(const unsigned char *buffer, unsigned char *hexvalue);	// helper-method to convert a string "C8" to a hex-value 0xc8.
void convert_MAC_hex_to_string(const std::string *hex, std::string *target);	// we convert the 6 hex-numbers to readable format (for example 0x41 to '41')
void upcase_all_string_values();	// we upcase all strings in our variable-map
};	// end of class m2a_ini_file_class


#endif // #define _m2a_ini_file_class_include
