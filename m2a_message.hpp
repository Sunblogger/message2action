/*
 m2a_message.hpp
 Version: 1.00
 Date: 28.06.2022
   
 * Copyright 2022 
 * 
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _m2a_message_class_include
#define _m2a_message_class_include

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This base class holds all methods (=functions) and fields (=variables) which are used by all classes which will be derived from this base class.
// The derived classes are processing mails, telegram-messages, SMS and more
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class message_processor_base_class {  

public:

enum class channel_type_class {	// this types of channels are we able to read messages from and write messages to as an answer
	is_sms_file,			// SMS from files
	is_telegram_file,		// Telegram from text-files
	is_telegram_online,		// Telegram from online-calls
	is_mail_file			// mails from files
};	// end of enum channel_type

message_processor_base_class(channel_type_class ct, m2a_ini_file_class *ini, logfile_class *log) : channel_type(ct), ini_file(ini), log_file(log) {	setup_help_text(&help_text); 
																																					setup_command_map();
																																					ini_file->get_value("check_online_status_waittime", &check_online_status_waittime); 
																																					ini_file->get_value("capitalize_values", &capitalize_values_flag);} // constructor

protected:

enum class command_type {	// this types of commands do we have: first we list the build-in-commands and then the user-definded ones:
	stop,				// we have to stop the program		
	reboot,				// we have to reboot the computer
	shutdown,			// we have to shutdown the computer
	status,				// we send status of program
	config,				// we have to send configuration
	help,				// we have to send help
	questionmark,		// we have to send help
	user_defined_00,	// user defined command 00
	user_defined_01,	
	user_defined_02,			
	user_defined_03,
	user_defined_04,
	user_defined_05,	// ...
	user_defined_06,
	user_defined_07,
	user_defined_08,
	user_defined_09,	// user defined command 09
	wol_00,				// send wake-up-on-LAN to device 00
	wol_01,	
	wol_02,
	wol_03,
	wol_04,
	wol_05,
	wol_06,
	wol_07,
	wol_08,
	wol_09,				// send wake-up-on-LAN to device 09
	wol_status_00,		// ask for online-status of device #00 
	wol_status_01,		// ask for online-status of device #00 
	wol_status_02,		// ask for online-status of device #00 
	wol_status_03,		// ask for online-status of device #00 
	wol_status_04,		// ask for online-status of device #00 
	wol_status_05,		// ask for online-status of device #00 
	wol_status_06,		// ask for online-status of device #00 
	wol_status_07,		// ask for online-status of device #00 
	wol_status_08,		// ask for online-status of device #00 
	wol_status_09 		// ask for online-status of device #00 

};

struct command_struct {		// this struct holds 1 variable
	command_type cmd_type;		// the type of the variable: stop, reboot, config ...
	std::string var_string;		// this is the string we've found in ini-file
	int var_int = 0;			// if variable_type is an int, then we save the int-value of the string here
	unsigned int var_uint = 0;	// if variable_type is an unsigned int, then we save the unsigned int-value of the string here
	bool	var_bool = false;	// if variable_type is a bool, then we save the bool-value of the string here
};	// end of variable_struct

channel_type_class channel_type; // the type of channel we have 
unsigned int check_online_status_waittime; // the time in seconds we are waiting after we have send WOL-command to device
unsigned int wait_time_next_message;	// time to wait for next message expressed in seconds: this value differs for each type of message
const unsigned int wait_time_cycle = 200;	// time expressed in milli-seconds how long we wait before we check processing-flag again
unsigned int wait_cycles; 		// how many cycles do we need until we have waited for next message: this value differs for each type of message
m2a_ini_file_class *ini_file;	// pointer to our ini-file to get specific values for our channel
logfile_class *log_file;		// pointer to our log-file so that we can add messages to our log-file
std::string command, sender, receiver, timestamp; 	// the command we have found in the message, the sender who sent the message to us, the receiver who will get the answer, timestamp when we received the message	
std::map<std::string, command_type> cmd_map; 	// this map holds as first parameter the command we have found in ini-file (stop, reboot, CMD00 ... CMD09). As second parameter it holds the kind of command which has to be excuted
std::map<std::string, command_type>::iterator cmd_map_iterator; 
unsigned int processed_messages_counter = 0;	// counts how many times we have processed successfully a message
unsigned int warning_counter = 0;				// counts how many times we had a warning
unsigned int error_counter = 0;					// counts how many times we had an error
std::string help_text;	// a buffer to hold a help text which helps the user when he does not know which command he should send.
std::string cmd_00, cmd_01, cmd_02, cmd_03, cmd_04, cmd_05, cmd_06, cmd_07, cmd_08, cmd_09;	// the 10 commands the user has specified in ini-file
std::string MAC_00_hex, MAC_01_hex, MAC_02_hex, MAC_03_hex, MAC_04_hex, MAC_05_hex, MAC_06_hex, MAC_07_hex, MAC_08_hex, MAC_09_hex; // the 10 MAC-addresses as string
bool capitalize_values_flag;	// this flag tells us that we have to change all values to uppercase letters (example: 'a' to 'A')

void setup_status(std::string *status);		// prepares a string holding the status of the processor
void setup_help_text(std::string *help);	// prepares a string holding a help-text for the user
void setup_command_map();	// sets up the map which hold all possible commands
void send_WOL(const std::string *mac_address);	// we send WOL-broadcast to MAC-address
bool check_onlinestatus(const std::string *ping_address, std::string *result); 	// we check online-status
};	// end of class message_processor_base_class

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Base-class for processing messages which are saved in files (and not online). Examples: mails, SMS, Telegram as files 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class message_processor_file_class : public message_processor_base_class {
public:
message_processor_file_class(channel_type_class channel, m2a_ini_file_class *ini, logfile_class *log) : message_processor_base_class(channel, ini, log) { 	ini_file->get_value("garbage_collection_days", &maximum_age_in_days);	// we get the value from ini-class for more easy use
																																							ini_file->get_value("capitalize_values", &capitalize_values_flag);}	// constructor  

protected:	
std::string inbound_path, outbound_path, processed_path;	// full path where received messages will be found, where answers will be saved, where we save the processed files
unsigned int maximum_age_in_days;			// the maximum age for processed files for garbage-collection
bool initial_garbage_collection_flag = true;	// flag that tells us if we are doing garbage collection for first time after program started

void save_incoming_file(const std::filesystem::path original); // we save the original file in our directory for processed files and delete the original file
void collect_garbage(const std::string *path, const unsigned int age, std::chrono::time_point<std::chrono::system_clock> *timepoint_last_collection, bool *initial_collection);		// we delete all files which are older than specified
}; 	// end of message_processor_file_class

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Class for processing messages from mails which are saved in files  
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class mail_file_processor_class : public message_processor_file_class {
public:

mail_file_processor_class(const channel_type_class channel, m2a_ini_file_class *ini, logfile_class *log) : message_processor_file_class(channel, ini, log) {	// constructor  
	init_message_processor();
};

void process_mail_files();	// this method will process the mails in form of files
private:
const unsigned int max_chars_for_subject_in_mail = 60; 	// the maximum # of chars we want to have in a subject of a mail

void create_message_to_send(const std::string *message, const std::string *sender);
std::string mail_adr_00, mail_adr_01, mail_adr_02, mail_adr_03, mail_adr_04, mail_adr_05, mail_adr_06, mail_adr_07, mail_adr_08, mail_adr_09 ;
std::vector<std::string> mail_address_vector; 	// this vector holds the mail-addresses we have found in the ini-file

void read_command_from_file(const std::filesystem::path path, std::string *cmd, std::string *snd);	// method to read a command from a file	
void init_message_processor();	// sets variables specific for processing this kind of messages
void execute_command(const std::string *cmd, const std::string *snd, bool *br_flag);
void handle_sending_wol(const std::string wol_number, const std::string *reciever);
void handle_sending_wol_status(const std::string wol_number, const std::string *reciever);
bool check_sender(const std::string *snd);
void send_warning_to_all_recipients(const std::string *snd); 
}; 	// end of mail_file_processor_class

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Class for processing messages from telegram which are saved in files  
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class telegram_file_processor_class : public message_processor_file_class {
public:

telegram_file_processor_class(const channel_type_class channel, m2a_ini_file_class *ini, logfile_class *log) : message_processor_file_class(channel, ini, log) {	// constructor  
	init_message_processor();
};

void process_telegram_files();	// this method will process the telegram-messages in form of files

private:
void create_message_to_send(const std::string *message, const std::string *sender);
std::string telegram_chat_id_00, telegram_chat_id_01, telegram_chat_id_02, telegram_chat_id_03, telegram_chat_id_04, telegram_chat_id_05, telegram_chat_id_06, telegram_chat_id_07, telegram_chat_id_08, telegram_chat_id_09;
std::vector<std::string> telegram_chat_id_vector; 	// this vector holds the chat-ids we have found in the ini-file
std::string telegram_token;	// the token for sending and receiving Telegram-messages

void init_message_processor();	// sets variables specific for processing this kind of messages
void execute_command(const std::string *cmd, const std::string *snd, bool *br_flag);
void read_command_from_file(const std::filesystem::path path, std::string *cmd, std::string *snd);	// method to read a command from a file	
bool check_sender(const std::string *snd);
void send_warning_to_all_recipients(const std::string *snd); 
void handle_sending_wol(const std::string wol_number, const std::string *reciever);
void handle_sending_wol_status(const std::string wol_number, const std::string *reciever);

}; 	// end of telegram_file_processor_class


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Class for processing messages from SMS which are saved in files  
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class sms_file_processor_class : public message_processor_file_class {
public:

sms_file_processor_class(const channel_type_class channel, m2a_ini_file_class *ini, logfile_class *log) : message_processor_file_class(channel, ini, log) {	// constructor  
	init_message_processor();
};

void process_sms_files();	// this method will process the sms in form of files

private:

void create_message_to_send(const std::string *message, const std::string *sender);
std::string cell_phone_00, cell_phone_01, cell_phone_02, cell_phone_03, cell_phone_04, cell_phone_05, cell_phone_06, cell_phone_07, cell_phone_08, cell_phone_09;
std::vector<std::string> cell_phone_vector; 	// this vector holds the cell-phone-numbers we have found in the ini-file

void read_command_from_file(const std::filesystem::path path, std::string *cmd, std::string *snd);	// method to read a command from a file	
void init_message_processor();	// sets variables specific for processing this kind of messages
void execute_command(const std::string *cmd, const std::string *snd, bool *br_flag);
void handle_sending_wol(const std::string wol_number, const std::string *reciever);
void handle_sending_wol_status(const std::string wol_number, const std::string *reciever);
bool check_sender(const std::string *snd);
void send_warning_to_all_recipients(const std::string *snd); 
}; 	// end of sms_file_processor_class

#endif // #ifndef _m2a_message_class_include
