/*
 m2a_ini.cpp
 Release: 0.3, date: 10.06.2022
  
 
 * Copyright 2022
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

#include <exception>
#include <stdexcept>
#include <vector>
#include <filesystem>
#include <inifile.hpp>	// ini-file base-class
#include <logfile.hpp>	// logfile base-class
#include <m2a_ini.hpp>	// handling of ini-file for messag2action

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This method upcases all strings in our variable-map
// wäre das nicht besser, diese Methode in die Basis-Klasse zu übernehmen? Dann hätten alle was davon
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void m2a_ini_file_class::upcase_all_string_values() {
	
std::vector<std::string> variables {"mail_address_00", "mail_address_01", "mail_address_02", "mail_address_03", "mail_address_04", "mail_address_05", "mail_address_06", "mail_address_07", "mail_address_08", "mail_address_09", 
									"message2action_stop_command", "shutdown_command", "reboot_command", "message2action_status_command", "message2action_help_command", "message2action_config_command", 
									"online_status_command_suffix" } ;	// a vector with all variables which values we want to change to capital letters
std::vector<std::string>::iterator variable_it;
std::string tempstring;
	
	for (variable_it = variables.begin(); variable_it != variables.end(); ++variable_it) {	// we step throug all variables in our vector
		get_value(*variable_it, &tempstring);
		for (unsigned int char_counter = 0; char_counter < tempstring.length(); ++char_counter) tempstring[char_counter] = std::toupper(tempstring[char_counter]);	// we change all chars to upcase for more easy comparision			
		set_value(*variable_it, &tempstring);	
		get_value(*variable_it, &tempstring);
	}	// end of for-loop
};	// end of m2a_ini_file_class::upcase_all_string_values()

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This method converts the 6 hex-chars which represent the MAC-address in a readable string. We convert the 6 hex-numbers to readable format (for example 0x41 to '41')
// Input: hex = the hex string with 6 chars; target = the string which will hold the readable string. The 2-chars are separated by ':'
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void m2a_ini_file_class::convert_MAC_hex_to_string(const std::string *hex, std::string *target) { 	
std::string temp_string;
unsigned char temp_char, low_byte, high_byte;

	target->clear();	// we reset our target
	for (unsigned int counter = 0; counter < 6; ++counter) {
		temp_char = (*hex)[counter];
		low_byte = temp_char << 4;	// we shift 4 bits to the right first in order to loose the 4 highest bits. Then we shift 4 bits to left to keep the 4 lowest bits
		low_byte = low_byte >> 4;
		high_byte = temp_char >> 4;
		switch (high_byte) {
			case 0 : target->append("0"); break;
			case 1 : target->append("1"); break;
			case 2 : target->append("2"); break;
			case 3 : target->append("3"); break;
			case 4 : target->append("4"); break;
			case 5 : target->append("5"); break;
			case 6 : target->append("6"); break;
			case 7 : target->append("7"); break;
			case 8 : target->append("8"); break;
			case 9 : target->append("9"); break;
			case 10: target->append("A"); break;
			case 11: target->append("B"); break;
			case 12: target->append("C"); break;
			case 13: target->append("D"); break;
			case 14: target->append("E"); break;
			case 15: target->append("F"); break;
		}	// end of switch
		switch (low_byte) {
			case 0 : target->append("0"); break;
			case 1 : target->append("1"); break;
			case 2 : target->append("2"); break;
			case 3 : target->append("3"); break;
			case 4 : target->append("4"); break;
			case 5 : target->append("5"); break;
			case 6 : target->append("6"); break;
			case 7 : target->append("7"); break;
			case 8 : target->append("8"); break;
			case 9 : target->append("9"); break;
			case 10: target->append("A"); break;
			case 11: target->append("B"); break;
			case 12: target->append("C"); break;
			case 13: target->append("D"); break;
			case 14: target->append("E"); break;
			case 15: target->append("F"); break;
		}	// end of switch
		if (counter < 5) target->append(":");	// we append ':' only for the first 5 hex-values
		
	}	// end of for-loop
	
	
};	// end of m2a_ini_file_class::convert_MAC_hex_to_string

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This method sets up a buffer which contains the complete configuration of the program. This buffer can be used to print it on the screen or to send
// it back to sender if he requests configuration
// Input: config_buf = the string which will hold the complete configuration
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void m2a_ini_file_class::setup_ini_file_content(std::string *config_buf) {

std::string value, temp_string;
unsigned int counter; //, temp_int;
char char_buf[200] = "";
std::map<const std::string, std::unique_ptr<variable_struct>>::iterator hostname_var_map_iterator, ip_adr_var_map_iterator;
std::unique_ptr<variable_struct> temp_var;

config_buf->clear();	// we reset the buffer first

if (true == var_map["capitalize_values"]->var_bool) sprintf(char_buf, "All received values will be changed to capitals.\n"); else sprintf(char_buf, "All received values will be kept unchanged.");
config_buf->append(char_buf);

sprintf(char_buf, "Shutdown: %s. Reboot: %s. Status: %s. Stop: %s\n", var_map["shutdown_command"]->var_string.c_str(), var_map["reboot_command"]->var_string.c_str(), var_map["message2action_status_command"]->var_string.c_str(), var_map["message2action_stop_command"]->var_string.c_str());
config_buf->append(char_buf);

sprintf(char_buf, "Help: %s or %s. Get configuration: %s\n", var_map["message2action_help_command"]->var_string.c_str(), var_map["message2action_questionmark_command"]->var_string.c_str(), var_map["message2action_config_command"]->var_string.c_str());
config_buf->append(char_buf);

sprintf(char_buf, "Maximum age of files for garbage collection: %i\n", var_map["garbage_collection_days"]->var_uint);
config_buf->append(char_buf);

//sprintf(char_buf, "Filename for saving stop-message: %s\n", var_map["message2action_stop_filename"]->var_string.c_str());
//config_buf->append(char_buf);

sprintf(char_buf, "Path for incoming SMS: %s, outgoing SMS: %s, processed SMS: %s\n", var_map["path_incoming_SMS"]->var_string.c_str(), var_map["path_outgoing_SMS"]->var_string.c_str(), var_map["path_processed_SMS"]->var_string.c_str());
config_buf->append(char_buf);

sprintf(char_buf, "Path for incoming mail: %s, outgoing mail: %s, processed mail: %s\n", var_map["path_incoming_mail"]->var_string.c_str(), var_map["path_outgoing_mail"]->var_string.c_str(), var_map["path_processed_mail"]->var_string.c_str());
config_buf->append(char_buf);

sprintf(char_buf, "Path for incoming telegram: %s, outgoing telegram: %s, processed telegram: %s\n", var_map["path_incoming_telegram"]->var_string.c_str(), var_map["path_outgoing_telegram"]->var_string.c_str(), var_map["path_processed_telegram"]->var_string.c_str());
config_buf->append(char_buf);

sprintf(char_buf, "Logfile: %s with maximum size of logfile: %i kBytes\n", var_map["file_logfile"]->var_string.c_str(), var_map["max_size_logfile"]->var_uint);
config_buf->append(char_buf);
	
sprintf(char_buf, "Time to wait for next message in seconds: SMS: %i, mail: %i, telegram: %i\n", var_map["wait_time_next_sms"]->var_uint, var_map["wait_time_next_mail"]->var_uint, var_map["wait_time_next_telegram"]->var_uint);
config_buf->append(char_buf);
	
sprintf(char_buf, "Suffix after WOL00 ... WOL09 for checking onlinestatus: %s\n", var_map["online_status_command_suffix"]->var_string.c_str());
config_buf->append(char_buf);

sprintf(char_buf, "Waittime after sending WOL for online-check: %i seconds\n", var_map["check_online_status_waittime"]->var_uint);
config_buf->append(char_buf);

// now we print cell-phone-numbers:
var_map_iterator = var_map.find("receivingfrom_cell_phone00");
for (counter = 0; counter < 10; ++counter) {
	if (var_map_iterator->second->var_string == not_found_in_ini_file) { 
		#ifdef DEVSTAGE
		sprintf(char_buf, "Cellphone # %d: not specified in ini-file.\n", counter);
		config_buf->append(char_buf);
		#endif
 	} else { 
		sprintf(char_buf, "Cellphone # %d: %s\n", counter, var_map_iterator->second->var_string.c_str());
		config_buf->append(char_buf);
	}
	++var_map_iterator;
}

// now we print the mailaddresses:
var_map_iterator = var_map.find("mail_address_00");
for (counter = 0; counter < 10; ++counter) {
	if (var_map_iterator->second->var_string == not_found_in_ini_file) { 
		#ifdef DEVSTAGE
		sprintf(char_buf, "Mailadress # %d: not specified in ini-file.\n", counter);
		config_buf->append(char_buf);
		#endif
 	} else { 
		sprintf(char_buf, "Mailadress # %d: %s\n", counter, var_map_iterator->second->var_string.c_str());
		config_buf->append(char_buf);
	}
	++var_map_iterator;
}

// now we print the token for the bot :
sprintf(char_buf, "Token for Telegram bot: %s\n", var_map["telegram_bot_token"]->var_string.c_str());
config_buf->append(char_buf);

// now we print all Telegram chat-ids:
var_map_iterator = var_map.find("telegram_chat_id_00");
for (counter = 0; counter < 10; ++counter) {
	if (var_map_iterator->second->var_string == not_found_in_ini_file) { 
		#ifdef DEVSTAGE
		sprintf(char_buf, "Telegram chat ID # %d: not specified in ini-file.\n", counter);
		config_buf->append(char_buf);
		#endif
 	} else { 
		sprintf(char_buf, "Telegram chat ID # %d: %s\n", counter, var_map_iterator->second->var_string.c_str());
		config_buf->append(char_buf);
	}
	++var_map_iterator;
}

// now we print the commands out which can be send to raspberry:
var_map_iterator = var_map.find("CMD00");
for (counter = 0; counter < 10; ++counter) {
	if (var_map_iterator->second->var_string == not_found_in_ini_file) { 
		#ifdef DEVSTAGE
		sprintf(char_buf, "Command ID # %d: not specified in ini-file.\n", counter);
		config_buf->append(char_buf);
		#endif
 	} else { 
		sprintf(char_buf, "Command # %d: %s\n", counter, var_map_iterator->second->var_string.c_str());
		config_buf->append(char_buf);
	}
	++var_map_iterator;
}

// now we print the MAC-addresses to which the WOL-broadcast can be send:
var_map_iterator = var_map.find("WOL00");
hostname_var_map_iterator = var_map.find("hostname_00");
ip_adr_var_map_iterator = var_map.find("ip_address_00");
for (counter = 0; counter < 10; ++counter) {
	if (var_map_iterator->second->var_string == not_found_in_ini_file) { 
		sprintf(char_buf, "MAC-address # %d: not specified in ini-file.\n", counter);
		config_buf->append(char_buf);
	} else { // we have a MAC-address. We want to save the MAC-address without hostname or ip-address behind:
		for (unsigned int tempint = 0; tempint < 6; tempint++) {temp_string[tempint] = var_map_iterator->second->var_string[tempint];} // we save MAC-address for more easy use; we do not use assign here because assign stops with first '\0'
		convert_MAC_hex_to_string(&temp_string, &value);	// we convert the 6 hex-numbers to readable format (for example 0x41 to 'A')
		sprintf(char_buf, "MAC-address # %d: %s with ", counter, value.c_str()); // var_map_iterator->second->var_string.c_str());	
		config_buf->append(char_buf);
		if (hostname_var_map_iterator->second->var_string != empty_variable) {
			sprintf(char_buf, "hostname %s.\n", hostname_var_map_iterator->second->var_string.c_str());
			config_buf->append(char_buf);
		} else { if (ip_adr_var_map_iterator->second->var_string != empty_variable) {
				sprintf(char_buf, "IP-address %s.\n", ip_adr_var_map_iterator->second->var_string.c_str());
				config_buf->append(char_buf);
			} else config_buf->append("neither hostname nor ip-address.\n");
		}
	}
	++ip_adr_var_map_iterator;
	++hostname_var_map_iterator;
	++var_map_iterator;
}	// end of for-loop

// now we print which inbound-channels we will listen to:
var_map_iterator = var_map.find("inbound_sms_file_flag");
	if (var_map_iterator->second->var_string == not_found_in_ini_file) { 
		#ifdef DEVSTAGE
		sprintf(char_buf, "SMS file based processing is not specified in ini-file.\n");
		config_buf->append(char_buf);
		#endif
	} else {
		if (true == var_map_iterator->second->var_bool) {
			sprintf(char_buf, "SMS files will be processed.\n");
			config_buf->append(char_buf);
		} else {
			sprintf(char_buf, "SMS files will not be processed.\n");
			config_buf->append(char_buf);	
		}
	}	

var_map_iterator = var_map.find("inbound_mail_file_flag");
	if (var_map_iterator->second->var_string == not_found_in_ini_file) { 
		#ifdef DEVSTAGE
		sprintf(char_buf, "Mail file based processing is not specified in ini-file.\n");
		config_buf->append(char_buf);
		#endif
	} else {
		if (true == var_map_iterator->second->var_bool) {
			sprintf(char_buf, "Mail files will be processed.\n");
			config_buf->append(char_buf);
		} else {
			sprintf(char_buf, "Mail files will not be processed.\n");
			config_buf->append(char_buf);	
		}
	}	

var_map_iterator = var_map.find("inbound_telegram_file_flag");
	if (var_map_iterator->second->var_string == not_found_in_ini_file) { 
		#ifdef DEVSTAGE
		sprintf(char_buf, "Telegram file based processing is not specified in ini-file.\n");
		config_buf->append(char_buf);
		#endif
	} else {
		if (true == var_map_iterator->second->var_bool) {
			sprintf(char_buf, "Telegram files will be processed.\n");
			config_buf->append(char_buf);
		} else {
			sprintf(char_buf, "Telegram files will not be processed.\n");
			config_buf->append(char_buf);	
		}
	}	

};	// end of m2a_ini_file_class::setup_ini_file_content

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This method sets up the map for all variables we have to search for in ini-file. The map contains as first parameter the string in the ini-file
// and as second parameter an unique-pointer to a struct of variable_struct. The advantage of the unique-pointer is that we do not care of the 
// delete-operation because this is handled by unique-pointer on its own. 
// We initialize the variable_struct with values which specify the variable either as string, unsigned int ... bool. Further on we can fill it with 
// default-values in case that we do not find the variable in ini-file.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void m2a_ini_file_class::setup_variable_map() {

var_map["receivingfrom_cell_phone00"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["receivingfrom_cell_phone01"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["receivingfrom_cell_phone02"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["receivingfrom_cell_phone03"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["receivingfrom_cell_phone04"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["receivingfrom_cell_phone05"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["receivingfrom_cell_phone06"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["receivingfrom_cell_phone07"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["receivingfrom_cell_phone08"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["receivingfrom_cell_phone09"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});

var_map["mail_address_00"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["mail_address_01"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["mail_address_02"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["mail_address_03"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["mail_address_04"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["mail_address_05"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["mail_address_06"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["mail_address_07"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["mail_address_08"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["mail_address_09"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});

var_map["CMD00"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["CMD01"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["CMD02"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["CMD03"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["CMD04"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["CMD05"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["CMD06"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["CMD07"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["CMD08"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["CMD09"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});

var_map["WOL00"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["WOL01"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["WOL02"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["WOL03"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["WOL04"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["WOL05"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["WOL06"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["WOL07"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["WOL08"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["WOL09"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});

var_map["telegram_chat_id_00"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["telegram_chat_id_01"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["telegram_chat_id_02"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["telegram_chat_id_03"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["telegram_chat_id_04"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["telegram_chat_id_05"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["telegram_chat_id_06"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["telegram_chat_id_07"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["telegram_chat_id_08"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["telegram_chat_id_09"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});

var_map["garbage_collection_days"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_unsigned_int, empty_variable, 30, 0, 0, false});
var_map["telegram_bot_token"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["inbound_sms_file_flag"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_bool, empty_variable, 0, 0, 0, false});
var_map["inbound_mail_file_flag"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_bool, empty_variable, 0, 0, 0, false});
var_map["inbound_telegram_file_flag"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_bool, empty_variable, 0, 0, 0, false});

var_map["message2action_stop_command"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, "stop", 0, 0, 0, false});
var_map["reboot_command"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, "reboot", 0, 0, 0, false});
var_map["shutdown_command"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, "shutdown", 0, 0, 0, false});
var_map["message2action_status_command"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, "status", 0, 0, 0, false});
var_map["message2action_help_command"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, "help", 0, 0, 0, false});
var_map["message2action_questionmark_command"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, "?", 0, 0, 0, false});
var_map["message2action_config_command"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, "config", 0, 0, 0, false});
var_map["path_incoming_SMS"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["path_outgoing_SMS"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["path_processed_SMS"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["path_incoming_mail"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["path_outgoing_mail"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["path_processed_mail"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["path_incoming_telegram"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["path_outgoing_telegram"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["path_processed_telegram"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});
var_map["file_logfile"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, "message2action.log", 0, 0, 0, false});
var_map["max_size_logfile"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_unsigned_int, empty_variable, 500, 0, 0, false});
var_map["wait_time_next_sms"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_unsigned_int, empty_variable, 30, 0, 0, false});
var_map["wait_time_next_mail"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_unsigned_int, empty_variable, 120, 0, 0, false});
var_map["wait_time_next_telegram"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_unsigned_int, empty_variable, 1, 0, 0, false});
var_map["online_status_command_suffix"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, "stat", 0, 0, 0, false});
var_map["check_online_status_waittime"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_unsigned_int, empty_variable, 10, 0, 0, false});
var_map["capitalize_values"] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_bool, empty_variable, 0, 0, 0, false});

};	// end of m2a_ini_file_class::setup_variable_map

//*****************************************************************************************************************
// get_hexvalue_from_string: converts a string to its hex-value. For example string "A9" will be converted to 0xa9. 
// input: 	buffer : the source where to read from, it is a string with 2 chars, for example "A9"
// 			hexvalue: the value in hex-form, for example 0xa9
// returncodes: 0 = no error
// 				1, 2 = any error
//*****************************************************************************************************************
unsigned m2a_ini_file_class::get_hexvalue_from_string(const unsigned char *buffer, unsigned char *hexvalue) {
		
	unsigned temp = 0; 
	
	switch (buffer[0]) {
			case '0': hexvalue[0] = 0; break; 
			case '1': hexvalue[0] = 16; break; 
			case '2': hexvalue[0] = 32; break; 
			case '3': hexvalue[0] = 48; break; 
			case '4': hexvalue[0] = 64; break; 
			case '5': hexvalue[0] = 80; break; 
			case '6': hexvalue[0] = 96; break; 
			case '7': hexvalue[0] = 112; break; 
			case '8': hexvalue[0] = 128; break; 
			case '9': hexvalue[0] = 144; break; 
			case 'A': hexvalue[0] = 160; break; 
			case 'B': hexvalue[0] = 176; break; 
			case 'C': hexvalue[0] = 192; break; 
			case 'D': hexvalue[0] = 208; break; 
			case 'E': hexvalue[0] = 224; break; 
			case 'F': hexvalue[0] = 240; break; 

			default: temp = 1; break;   // we don't have a valid charackter, the value != 0 is our flag for this
		}	
					
	switch (buffer[1]) {
			case '0': hexvalue[0] += 0; break; 
			case '1': hexvalue[0] += 1; break; 
			case '2': hexvalue[0] += 2; break; 
			case '3': hexvalue[0] += 3; break; 
			case '4': hexvalue[0] += 4; break; 
			case '5': hexvalue[0] += 5; break; 
			case '6': hexvalue[0] += 6; break; 
			case '7': hexvalue[0] += 7; break; 
			case '8': hexvalue[0] += 8; break; 
			case '9': hexvalue[0] += 9; break; 
			case 'A': hexvalue[0] += 10; break; 
			case 'B': hexvalue[0] += 11; break; 
			case 'C': hexvalue[0] += 12; break; 
			case 'D': hexvalue[0] += 13; break; 
			case 'E': hexvalue[0] += 14; break; 
			case 'F': hexvalue[0] += 15; break; 

			default: temp = 2; break; // we don't have a valid charackter, the value != 0 is our flag for this
		}	
//	printf("temphex: %i\n", temphex);
	return (temp); 
}	// end of ini_file_class::get_hexvalue_from_string


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This method checks if the values we have found in ini-file are OK.
// Examples: path to incoming mails must exist.
// For "WOLxy" the MAC-address will be separated from host-name or IP-address and a new member will be added for either 
// If we have any error we will throw an exception which will tell what it is not OK
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void m2a_ini_file_class::check_values_plausability() {
std::ofstream log_file;		// the filestream we will read all data from	
char char_buf[200];
std::string temp_string, throw_string, hostname, ip_address, mac_adr_string; 
unsigned char mac_adr_2char[2], mac_adr_hex[6];	
unsigned int temp_int, counter, vector_counter;
std::unique_ptr<variable_struct> temp_var;
const std::vector<std::string> variables_vector = {	"path_incoming_SMS", "path_outgoing_SMS", "path_processed_SMS",
												"path_incoming_mail", "path_outgoing_mail", "path_processed_mail",
												"path_incoming_telegram", "path_outgoing_telegram", "path_processed_telegram"};	// a vector holding all variables with path-values
std::filesystem::path path_to_check; 	// a path which has to be checked if it is valid

// 	we check if logfile is OK:
log_file.open(var_map["file_logfile"]->var_string, std::ios::app);	// we try to open the file but do not create a new one if it exists
if (log_file.is_open() == false)	{
	sprintf(char_buf, "Value for file_logfile %s is invalid or file can not be written to.", var_map["file_logfile"]->var_string.c_str());
	throw std::invalid_argument(char_buf);
} else log_file.close();

// we separate the ip-address or hostname behind MAC-address and fill our map with new values:
var_map_iterator = var_map.find("WOL00");	// we set iterator to first MAC-address
for (counter = 0; counter < 10; ++counter) {	// we search for 10 values
	if (var_map_iterator->second->var_string != not_found_in_ini_file) { // we start searching for hostname or ip-address only if we have a value for MAC-address
		mac_adr_string = var_map_iterator->second->var_string; 	// we save the value for more easy operation
		// we need to save the MAC-address in hex-values like 0x9a and not in char-values like "9A"
		for (temp_int = 0; temp_int < 6; ++temp_int) {
			mac_adr_2char[0] = mac_adr_string[3*temp_int];
			mac_adr_2char[1] = mac_adr_string[3*temp_int + 1];
			if (get_hexvalue_from_string(mac_adr_2char, &(mac_adr_hex[temp_int])) != 0) { 	// we can not convert the value to hex
				sprintf(char_buf, "Parameter %s for MAC-address is not valid!", var_map_iterator->second->var_string.c_str());
				throw std::invalid_argument(char_buf);	// the content of exception can be used for logging	
			}
		}	// end of for-loop
		// mac_adr_hex[6]='\0';	// we terminate the char-array
		temp_int = var_map_iterator->second->var_string.find(';');	// do we have a separator sign ';'?
		if (temp_int != std::string::npos) {	// if semikolon is in string we erase all chars from ';' to end
			mac_adr_string.erase(temp_int, (mac_adr_string.length() - temp_int));	
			temp_string = var_map_iterator->second->var_string.substr(temp_int + 1);
			for (unsigned int char_counter = 0; char_counter < temp_string.length(); ++char_counter) temp_string[char_counter] = std::toupper(temp_string[char_counter]);	// we change all chars to upcase for more easy comparision
			temp_int = temp_string.find("HOST="); 
			sprintf(char_buf, "hostname_%02d", counter);	// we prepare the string which will be the first element of the map-element
			if (temp_int != std::string::npos) {	// we have "HOST=" in our string, now we get all chars behind "="
				hostname = temp_string.substr(temp_int + 5);	// we save the value for hostname
				var_map[char_buf] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, hostname, 0, 0, 0, false});	// we add 1 element to the map
				sprintf(char_buf, "ip_address_%02d", counter);	// we prepare the string which will be the first element of the map-element
				var_map[char_buf] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});	// we add 1 element to the map
			} else {	// we do not have "HOST=" in our string
				var_map[char_buf] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, empty_variable, 0, 0, 0, false});	// we add 1 element of hostnames to the map
				temp_int = temp_string.find("IP="); 
				if (temp_int != std::string::npos) {
					ip_address = temp_string.substr(temp_int + 3);	// we save IP-address
					sprintf(char_buf, "ip_address_%02d", counter);	// we prepare the string which will be the first element of the map-element
					var_map[char_buf] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, ip_address, 0, 0, 0, false});	// we add 1 element to the map
				} else {	// we have ';' but neither host= nor ip= in our string
					sprintf(char_buf, "Parameter for hostname or ip-address is missing in value for MAC-address %s in MAC-address #%d", var_map_iterator->second->var_string.c_str(), counter + 1);
					throw std::invalid_argument(char_buf);	// the content of exception can be used for logging
				}
			}
		} else {	// we do not have a ';' 
			sprintf(char_buf, "Parameter for hostname or ip-address is missing in value for MAC-address %s in MAC-address #%d", var_map_iterator->second->var_string.c_str(), counter + 1);
			throw std::invalid_argument(char_buf);	// the content of exception can be used for logging
		}
			for (unsigned int tempint = 0; tempint < 6; tempint++) { var_map_iterator->second->var_string[tempint] = mac_adr_hex[tempint]; } // we save the hex-formated-MAC address in our variable; we do not use assign here because assign stops at first '\0'
		
	} else {	// we do not have a MAC-address so we add empty values for hostname and ip_address	
		sprintf(char_buf, "hostname_%02d", counter);	// we prepare the string which will be the first element of the map-element
		var_map[char_buf] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, not_found_in_ini_file, 0, 0, 0, false});	// we add 1 element to the map
		sprintf(char_buf, "ip_address_%02d", counter);	// we prepare the string which will be the first element of the map-element
		var_map[char_buf] = std::unique_ptr<variable_struct>(new variable_struct {variable_type::is_string, not_found_in_ini_file, 0, 0, 0, false});	// we add 1 element to the map
		sprintf(char_buf, "MAC-address #%02d is not found in ini-file.\n", counter);
		throw_string.append(char_buf);	// we add this warning to our buffer
	}
	++var_map_iterator;	// we step to next element of the map
}	// end of for-loop
	
// we check receiving and sending cell-phone-#:
// Note: on 32-bit-OS unsigned long can have maximum 10 digits but cellphone-# can have more than 10 digits. So we have to convert each digit separatly:
var_map_iterator = var_map.find("receivingfrom_cell_phone00");	// we set iterator to first cell-phone-#
for (temp_int = 0; temp_int < 10; ++temp_int) {	// we check 10 cell-phone-numbers
	if (var_map_iterator->second->var_string.compare(not_found_in_ini_file) != 0) {	// we check the value only when we have found in ini-file: so we search for string "<NOT FOUND IN INIFILE>"
		if (var_map_iterator->second->var_string.length() > 0) {	 // we check cell-phone-# only when we have a value
			// now we check if every char of our value is '0' ... '9' :
			for (counter = 0; counter < var_map_iterator->second->var_string.length(); ++counter) {	// end of for-loop
				if (!(var_map_iterator->second->var_string[counter] >= '0') && (var_map_iterator->second->var_string[counter] <= '9')) {
					// if we have a sign which does not represent a digit we have a problem:
					sprintf(char_buf, "Invalid sign %c found in value for cell-phone %s.", var_map_iterator->second->var_string[counter], var_map_iterator->second->var_string.c_str());
					throw std::invalid_argument(char_buf);	// the content of exception can be used for logging
				}
			}
		} else {	// we do not have a string, length == 0 and that's not OK for us
			sprintf(char_buf, "Value for %s does not have any content.", var_map_iterator->second->var_string.c_str());
			throw std::invalid_argument(char_buf);	// the content of exception can be used for logging
		}
	} else {	// we did find the value in ini-file, this is a fair warning:
		sprintf(char_buf, "Value for %s was not found in ini-file.\n", var_map_iterator->first.c_str());	
		throw_string.append(char_buf);	// we add this warning to our buffer
	}
	++var_map_iterator;
}	// end of for-loop

// we check a lot of paths (incoming SMS, outgoing SMS, mail, telegram ...) if they exist. We use a vector containing all strings to values we want to check:
for (vector_counter = 0; vector_counter < (unsigned int)variables_vector.size(); ++vector_counter) {	
	var_map_iterator = var_map.find(variables_vector[vector_counter]);	// we set iterator to element of vector with variables
	if (var_map_iterator->second->var_string.compare(not_found_in_ini_file) != 0) {	// we check the value only when we have found in ini-file: so we search for string "<NOT FOUND IN INIFILE>"
		if (var_map_iterator->second->var_string.length() > 0) {	// now we check if we have a '/' at end of path, this is often forgotten by user
			if (var_map_iterator->second->var_string[var_map_iterator->second->var_string.length() - 1] != '/' ) {
				sprintf(char_buf, "Path for %s in ini-file does not have ending '/'!", var_map_iterator->first.c_str());
				throw std::invalid_argument(char_buf);	// the content of exception can be used for logging
			} 
			if (std::filesystem::exists(var_map_iterator->second->var_string) != true) {
				sprintf(char_buf, "Path for %s in ini-file is not valid or does not exist!", var_map_iterator->first.c_str());
				throw std::invalid_argument(char_buf);	// the content of exception can be used for logging
			}	
		} else {	// we do not have a value for the mentioned variable:
			sprintf(char_buf, "Path for %s in ini-file is empty.", var_map_iterator->first.c_str());
			throw std::invalid_argument(char_buf);	// the content of exception can be used for logging
		}
	} else { 	// we did find the value in ini-file, this is a fair warning:
		sprintf(char_buf, "Value for %s was not found in ini-file.\n", var_map_iterator->first.c_str());	
		throw_string.append(char_buf);	// we add this warning to our buffer
		// throw std::out_of_range(char_buf);
	}
}	// end of for-loop

// now we check the mailadresses:
var_map_iterator = var_map.find("mail_address_00");	// we set iterator to first mail-address
for (temp_int = 0; temp_int < 10; ++temp_int) {	// we check 10 mail-addresses
	if (var_map_iterator->second->var_string.compare(not_found_in_ini_file) != 0) {	// we check the value only when we have found in ini-file: so we search for string "<NOT FOUND IN INIFILE>"
		if (var_map_iterator->second->var_string.length() > 0) {	 // we check mails-address only when we have a value
			
			// now we check if every mailaddress has '@':
			if (var_map_iterator->second->var_string.find('@') == std::string::npos) {
				sprintf(char_buf, "Value %s for mailaddress #%02d does not contain '@'.", var_map_iterator->second->var_string.c_str(), temp_int);
				throw std::invalid_argument(char_buf);	// the content of exception can be used for logging
			}
		} else {	// we do not have a string, length == 0 and that's not OK for us
			sprintf(char_buf, "Value for %s does not have any content.", var_map_iterator->second->var_string.c_str());
			throw std::invalid_argument(char_buf);	// the content of exception can be used for logging
		}
	} else {	// we did find the value in ini-file, this is a fair warning:
		sprintf(char_buf, "Value for %s was not found in ini-file.\n", var_map_iterator->first.c_str());	
		throw_string.append(char_buf);	// we add this warning to our buffer
	}
	++var_map_iterator;
}	// end of for-loop


if (throw_string.length() > 0) 	// if we have any warnings (not errors!) then we throw them out
		throw std::out_of_range(throw_string);
	
warning_status_flag = false;	
};	// end of m2a_ini_file_class::check_plausability()
