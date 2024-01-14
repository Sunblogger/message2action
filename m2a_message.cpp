/*
 m2a_message.cpp
 Version: 1.01
 Date: 31.12.2023
  
 * Copyright 2023 
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

#include <thread>
#include <chrono> 
#include <exception>
#include <stdexcept>
#include <filesystem>
#include <system_error>
#include <random>
#include <cstring>	// used for memset
#include <unistd.h>
#include <algorithm>
#include <sys/socket.h>	// used for socket-communication
#include <netinet/in.h>	// used for socket-communication
#include <atomic>		// used for proecessing_flag
#include <inifile.hpp>	// used for handling the ini-file
#include <logfile.hpp>	// used for handling with logfile
#include <stopfile.hpp>	// used for handling with stopfile
#include <m2a_ini.hpp>	// handling of ini-file for messag2action
#include <m2a_message.hpp> 	// handling of messages (SMS, Telegram, mail ...)

#define devstage

extern std::atomic<bool> processing_flag; 	// the variable is defined in main-source message2action.cpp

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// we send WOL-broadcast to MAC-address
// input: mac_adr = the MAC-address to which we will send the WOL-command
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void message_processor_base_class::send_WOL(const std::string *mac_adr) {
unsigned int tempint; 
int socketdescriptor;  	// return-value of function socket
int socketoption; 		// return-value of function setsocketopt
int optionvalue = 1; 
char message[102]; // will store the magic packet 
char *message_ptr = message;

struct sockaddr_in socketaddress;

unsigned int socketport = 7;
unsigned long broadcast = 0xFFFFFFFF;
ssize_t sendlength;

// Note these defines: AF_INET = IPV4; AF_INET6 = IPV6;
socketdescriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
if (socketdescriptor < 0) { 
		throw std::invalid_argument("Socket for sending WOL could not be opened!");		
	} else {
		memset(message_ptr, 0xFF, 6);  // copy 6 times 0xFF at in the beginning
		
		message_ptr += 6; // step 6 bytes forward
		for (tempint = 0; tempint < 16; ++tempint) {
			memcpy(message_ptr, mac_adr->c_str(), 6);
			message_ptr += 6;
		}  // end of for-loop
		
	socketoption = setsockopt(socketdescriptor, SOL_SOCKET, SO_BROADCAST, &optionvalue, sizeof(optionvalue));
		if (socketoption < 0) close(socketdescriptor); else {
			socketaddress.sin_family = AF_INET;
			socketaddress.sin_addr.s_addr = broadcast;
			socketaddress.sin_port = htons(socketport);
			
			sendlength = sendto(socketdescriptor,(char *)message, sizeof(message), 0, (struct sockaddr *)&socketaddress, sizeof(socketaddress));
			if (sendlength < 0) { 
				throw std::invalid_argument("Sending of WOL-command failed!");		
			} else {
				close(socketdescriptor); 
			} 
		} 	// end of sendding message out	
	} // end of setting socket option
	
};	// end of message_processor_base_class::send_WOL

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// we check if a specific ip-address is onnlinr or not.
// input:  	ping_address = the IP-address of the device we want to check
//			result = the string of the result (online or not or something else)
// returns: true = device is online; false = device is not online or can not be reached
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool message_processor_base_class::check_onlinestatus(const std::string *ping_address, std::string *result) {	// we check the online-status
std::string pingcommand = "/bin/ping -c 1 -w 2 "; 	// full programm to call with all parameters: -c : counter or retries, -w : timeout in seconds
std::string logentry;
char buffer[256] = ""; 		// buffer where the return-value of ping is saved
FILE *pipe;   			// pipe for saving return-value of ping
unsigned int tempint = 0;

pingcommand.append(*ping_address);	// we add at the end of our command the IP-address
pingcommand.append(" 2>&1");		// standout for errors should go on screen
pipe = popen(pingcommand.c_str(), "r"); 	// open pipe for reading
	if (pipe != 0) {	// we could open the pipe
		// now we read 5 times from pipe in order to get a line with status of ping:
		for (tempint = 0; tempint < 5; ++tempint) { 
			if (fgets(buffer, 256, pipe) == NULL) break;
		}
			pclose(pipe);	// we close pipe under any circumstances, we do not read more chars from pipe
			// now we check if we have a string "0% packet loss" or not in the line:
		if (strstr(buffer, "0% packet loss") != 0) {
			result->assign(*ping_address);
			result->append(" is online.");
			return true;	// we return success = true
		} else {	// we do not have "0% packet loss" in result. Either host is offline or unknown
			result->assign(*ping_address);
			result->append(" is either offline, unknown or anything else is not OK.");
			return false;	// we return success = false
		}
	} else {	// we could not open pipe
		result->assign("Could not open pipe for "); 
		result->append(pingcommand);
		return false;	// we return success = false
	}
};	// end of message_processor_base_class::check_onlinestatus

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We setup the map with the commands we can get from the user. This map is the same for all kind of messages.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void message_processor_base_class::setup_command_map(void) {
std::string tempstring, statusstring;

	// now we setup our map with the found values for commands in our ini-file: First parameter is the found value (e.g. stop, reboot) and second is the mentioned command from our struct command_type
	ini_file->get_value("reboot_command", &tempstring); cmd_map[tempstring] = command_type::reboot;
	ini_file->get_value("shutdown_command", &tempstring); cmd_map[tempstring] = command_type::shutdown;
	ini_file->get_value("message2action_stop_command", &tempstring); cmd_map[tempstring] = command_type::stop;
	ini_file->get_value("message2action_status_command", &tempstring); cmd_map[tempstring] = command_type::status;
	ini_file->get_value("message2action_help_command", &tempstring); cmd_map[tempstring] = command_type::help;
	ini_file->get_value("message2action_questionmark_command", &tempstring); cmd_map[tempstring] = command_type::questionmark;
	ini_file->get_value("message2action_config_command", &tempstring); cmd_map[tempstring] = command_type::config;
	
	// we save the strings which represent the commands to be executed:
	ini_file->get_value("CMD00", &cmd_00); cmd_00.append("&");	// we add '&' so that we will have a background execution
	ini_file->get_value("CMD01", &cmd_01); cmd_01.append("&");	// we add '&' so that we will have a background execution
	ini_file->get_value("CMD02", &cmd_02); cmd_02.append("&");	// we add '&' so that we will have a background execution 
	ini_file->get_value("CMD03", &cmd_03); cmd_03.append("&");	// we add '&' so that we will have a background execution
	ini_file->get_value("CMD04", &cmd_04); cmd_04.append("&");	// we add '&' so that we will have a background execution
	ini_file->get_value("CMD05", &cmd_05); cmd_05.append("&");	// we add '&' so that we will have a background execution
	ini_file->get_value("CMD06", &cmd_06); cmd_06.append("&");	// we add '&' so that we will have a background execution
	ini_file->get_value("CMD07", &cmd_07); cmd_07.append("&");	// we add '&' so that we will have a background execution
	ini_file->get_value("CMD08", &cmd_08); cmd_08.append("&");	// we add '&' so that we will have a background execution
	ini_file->get_value("CMD09", &cmd_09); cmd_09.append("&");	// we add '&' so that we will have a background execution
	
	// now we get all user-defined-commands:
	ini_file->get_value("CMD00", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) cmd_map["CMD00"] = command_type::user_defined_00;	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("CMD01", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) cmd_map["CMD01"] = command_type::user_defined_01;	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("CMD02", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) cmd_map["CMD02"] = command_type::user_defined_02;	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("CMD03", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) cmd_map["CMD03"] = command_type::user_defined_03;	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("CMD04", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) cmd_map["CMD04"] = command_type::user_defined_04;	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("CMD05", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) cmd_map["CMD05"] = command_type::user_defined_05;	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("CMD06", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) cmd_map["CMD06"] = command_type::user_defined_06;	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("CMD07", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) cmd_map["CMD07"] = command_type::user_defined_07;	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("CMD08", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) cmd_map["CMD08"] = command_type::user_defined_08;	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("CMD09", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) cmd_map["CMD09"] = command_type::user_defined_09;	// we add only to our map when we have found a value in our ini-file
	
	ini_file->get_value("online_status_command_suffix" ,&statusstring);	// we save the string for command for the online-check
	// we get all WOL-data:
	ini_file->get_value("WOL00", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) { cmd_map["WOL00"] = command_type::wol_00; tempstring = "WOL00"; tempstring.append(statusstring); cmd_map[tempstring]	= command_type::wol_status_00; } 	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("WOL01", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) { cmd_map["WOL01"] = command_type::wol_01; tempstring = "WOL01"; tempstring.append(statusstring); cmd_map[tempstring]	= command_type::wol_status_01; } 	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("WOL02", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) { cmd_map["WOL02"] = command_type::wol_02; tempstring = "WOL02"; tempstring.append(statusstring); cmd_map[tempstring]	= command_type::wol_status_02; } 	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("WOL03", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) { cmd_map["WOL03"] = command_type::wol_03; tempstring = "WOL03"; tempstring.append(statusstring); cmd_map[tempstring]	= command_type::wol_status_03; } 	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("WOL04", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) { cmd_map["WOL04"] = command_type::wol_04; tempstring = "WOL04"; tempstring.append(statusstring); cmd_map[tempstring]	= command_type::wol_status_04; } 	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("WOL05", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) { cmd_map["WOL05"] = command_type::wol_05; tempstring = "WOL05"; tempstring.append(statusstring); cmd_map[tempstring]	= command_type::wol_status_05; } 	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("WOL06", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) { cmd_map["WOL06"] = command_type::wol_06; tempstring = "WOL06"; tempstring.append(statusstring); cmd_map[tempstring]	= command_type::wol_status_06; } 	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("WOL07", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) { cmd_map["WOL07"] = command_type::wol_07; tempstring = "WOL07"; tempstring.append(statusstring); cmd_map[tempstring]	= command_type::wol_status_07; } 	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("WOL08", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) { cmd_map["WOL08"] = command_type::wol_08; tempstring = "WOL08"; tempstring.append(statusstring); cmd_map[tempstring]	= command_type::wol_status_08; } 	// we add only to our map when we have found a value in our ini-file
	ini_file->get_value("WOL09", &tempstring); if (ini_file->not_found_in_ini_file != tempstring) { cmd_map["WOL09"] = command_type::wol_09; tempstring = "WOL09"; tempstring.append(statusstring); cmd_map[tempstring]	= command_type::wol_status_09; } 	// we add only to our map when we have found a value in our ini-file
	
};	// end of message_processor_base_class::setup_command_map

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We setup a string which tells us the status of our message-processor
// Input: a string which will hold the complete status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void message_processor_base_class::setup_status(std::string *status) {
	
	*status = "# of processed ";
	if (channel_type == channel_type_class::is_telegram_file) status->append(" Telegram "); else status->append(" SONSIGES ");
	status->append("messages: ");
	status->append(std::to_string(processed_messages_counter));
	status->append(", # of warnings: ");
	status->append(std::to_string(warning_counter));
	status->append(", # of errors: ");
	status->append(std::to_string(error_counter));
};	// end of message_processor_base_class::setup_status

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We setup a string which gives user a help.
// Input: a string which will hold the complete status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void message_processor_base_class::setup_help_text(std::string *help) {
std::string tempstring;
	
	*help = "These commands are available:\nstatus of program: ";
	ini_file->get_value("message2action_status_command", &tempstring);
	help->append(tempstring);
	help->append("\nstop program: ");
	ini_file->get_value("message2action_stop_command", &tempstring);
	help->append(tempstring);
	help->append("\nshutdown computer: ");
	ini_file->get_value("shutdown_command", &tempstring);
	help->append(tempstring);
	help->append("\nreboot computer: ");
	ini_file->get_value("reboot_command", &tempstring);
	help->append(tempstring);
	help->append("\nWOL00 ... WOL09: send wakeup on LAN to device 00 ... 09");
	ini_file->get_value("online_status_command_suffix", &tempstring);
	help->append("\nWOL00");
	help->append(tempstring);
	help->append(" ... WOL09");
	help->append(tempstring);
	help->append(": call for status after sending WOL00 ... WOL09\n");
	
	// now we get the userdefined commands: not all of them have to be defined in ini-file
	if (0 != ini_file->get_value("CMD00", &tempstring)) { help->append("CMD00: "); help->append(tempstring); help->append("\n"); }	// we add only the content of the command if it is defined in ini-file
	if (0 != ini_file->get_value("CMD01", &tempstring)) { help->append("CMD01: "); help->append(tempstring); help->append("\n"); }	// we add only the content of the command if it is defined in ini-file
	if (0 != ini_file->get_value("CMD02", &tempstring)) { help->append("CMD02: "); help->append(tempstring); help->append("\n"); }	// we add only the content of the command if it is defined in ini-file
	if (0 != ini_file->get_value("CMD03", &tempstring)) { help->append("CMD03: "); help->append(tempstring); help->append("\n"); }	// we add only the content of the command if it is defined in ini-file
	if (0 != ini_file->get_value("CMD04", &tempstring)) { help->append("CMD04: "); help->append(tempstring); help->append("\n"); }	// we add only the content of the command if it is defined in ini-file
	if (0 != ini_file->get_value("CMD05", &tempstring)) { help->append("CMD05: "); help->append(tempstring); help->append("\n"); }	// we add only the content of the command if it is defined in ini-file
	if (0 != ini_file->get_value("CMD06", &tempstring)) { help->append("CMD05: "); help->append(tempstring); help->append("\n"); }	// we add only the content of the command if it is defined in ini-file
	if (0 != ini_file->get_value("CMD07", &tempstring)) { help->append("CMD06: "); help->append(tempstring); help->append("\n"); }	// we add only the content of the command if it is defined in ini-file
	if (0 != ini_file->get_value("CMD08", &tempstring)) { help->append("CMD07: "); help->append(tempstring); help->append("\n"); }	// we add only the content of the command if it is defined in ini-file
	if (0 != ini_file->get_value("CMD09", &tempstring)) { help->append("CMD09: "); help->append(tempstring); help->append("\n"); }	// we add only the content of the command if it is defined in ini-file
	
};	// end of message_processor_base_class::setup_help

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Method to save the original file in our directory for processed files under a new name. The original-file will be deleted.
// Input: Filename + path of the original file, path were to move the processed file. 
// We throw an exception std::invalid_argument if we can not save the file, if we can not delete the original file or if we can not save the file in the desired path
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void message_processor_file_class::save_incoming_file(const std::filesystem::path original) {
std::string tempstring = processed_path;
std::error_code err_code;
std::filesystem::path processed;
	
	tempstring.append(std::filesystem::path(original).filename());
	processed.assign(tempstring);
	std::filesystem::rename(original, processed, err_code);					
	if (err_code) {	// something went wrong
		++error_counter;	// this is an error not a warning
		tempstring = "File ";
		tempstring.append(original);
		tempstring.append(" could not be moved to processed directory: ");
		tempstring.append(err_code.message());
		throw std::invalid_argument(tempstring);	// we throw an exception with details
	} else {
		tempstring = "File ";
		tempstring.append(original);
		tempstring.append(" was moved to processed directory ");
		tempstring.append(processed_path);
		throw tempstring;	// we throw the string for logging
	}

}; // end of message_processor_file_class::save_incoming_file

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Method to delete all files which are older than a specific # of days. The garbage collection does only take place when we call this method for first time
// after the program has started or when the day has changed after first call.
// Input: path = path where the files are saved; age = number of days which is maximum age
// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void message_processor_file_class::collect_garbage(const std::string *path, const unsigned int maximum_age, std::chrono::time_point<std::chrono::system_clock> *timepoint_last_collection, bool *initial_collection) {
std::chrono::time_point<std::chrono::system_clock> systemtime;	// the current point of time
std::chrono::time_point<std::chrono::system_clock> filetime; 	// the point of time when file was last modified 
const std::chrono::hours delta_in_hours(24);							// the time difference which we have to wait for next garbages collection; std::chrono::days is not supported yet by g++ 8.x	
const std::chrono::hours maximum_file_age(24*maximum_age);		// the maximum age expressed in hours
std::filesystem::file_time_type last_write_time_of_file;		// the point of time when file was last modified 
std::string logentry;

	if (maximum_age > 0) {	// if the maximum age for files is greater 0 then we have to check for old files
		systemtime = std::chrono::system_clock::now(); 	// we get current time
		if ((true == *initial_collection) || ((systemtime - *timepoint_last_collection) >= delta_in_hours)) {	// if we start this method for first time or if we have a change of the date we will start for garbage-collection 
			*initial_collection = false;	// we change the flag for first collection to false so that next call of the method will start with another value of this flag
			*timepoint_last_collection = systemtime;	// we set the point of time when we did last time garbage collection to current systemtime so that we compare with next call of this method 
			// now we collect garbage in our path:
			for (std::filesystem::directory_entry file: std::filesystem::directory_iterator(*path)) {	// we step through all files we find in our path
				if (true == file.is_regular_file()) {	// if we have a regular file and not something else like directory we want to have the command in the file
					#ifdef devstage
					logentry = "We check for garbage collection file: ";
					logentry.append(file.path());
					log_file->addmessage(logfile_class::logentrytype::info, logentry);
					#endif // ifdef devstage
					last_write_time_of_file = file.last_write_time(); 
					filetime = std::chrono::file_clock::to_sys(last_write_time_of_file);	// we convert the file_time to system_time
					if ((systemtime - filetime) > maximum_file_age) {	// if file is older than limit we delete it
						#ifdef devstage
						logentry = "Due to garbage collection we try to delete file: ";
						logentry.append(file.path());
						log_file->addmessage(logfile_class::logentrytype::info, logentry);
						#endif // ifdef devstage
						if (true == std::filesystem::remove(file.path())) { 	// we delete the orignal logfile						
							logentry = "Due to garbage collection we deleted file: ";
							logentry.append(file.path());
							log_file->addmessage(logfile_class::logentrytype::info, logentry);
						} else {	// we could not delete mentioned file
							logentry = "Due to garbage collection we could not delete file: ";
							logentry.append(file.path());
							log_file->addmessage(logfile_class::logentrytype::error, logentry);
						}
					} else {
						#ifdef devstage
						logentry = "Age of file is too smal for garbage collection: ";
						logentry.append(file.path());
						log_file->addmessage(logfile_class::logentrytype::info, logentry);
						#endif // ifdef devstage
					}
				}	// end if (true == file.is_regular_file())
			}	// end of for-loop
			
		} else {	// this is not first time we call this method or we did not have a change of the date
			/* #ifdef devstage
			log_file->addmessage(logfile_class::logentrytype::info, "Nothing to do for garbage collection because we did last time in last 24 hours.");
			#endif // ifdef devstage
			*/
		}	
	}
};	// end of message_processor_file_class::collect_garbage

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We save all variables which are needed for processing mails which are saved in files. Instead restoring the values from the ini-file-class for several times
// we save them separatly.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void mail_file_processor_class::init_message_processor() {
std::string tempstring;
		
	log_file->addmessage(logfile_class::logentrytype::info, "This is processor for mails with files."); 
	ini_file->get_value("wait_time_next_mail", &wait_time_next_message);
	ini_file->get_value("path_incoming_mail", &inbound_path);
	ini_file->get_value("path_outgoing_mail", &outbound_path);
	ini_file->get_value("path_processed_mail", &processed_path);
	ini_file->get_value("mail_address_00", &mail_adr_00); if (0 != mail_adr_00.compare(ini_file->not_found_in_ini_file)) mail_address_vector.push_back(mail_adr_00);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("mail_address_01", &mail_adr_01); if (0 != mail_adr_01.compare(ini_file->not_found_in_ini_file)) mail_address_vector.push_back(mail_adr_01);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("mail_address_02", &mail_adr_02); if (0 != mail_adr_02.compare(ini_file->not_found_in_ini_file)) mail_address_vector.push_back(mail_adr_02);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("mail_address_03", &mail_adr_03); if (0 != mail_adr_03.compare(ini_file->not_found_in_ini_file)) mail_address_vector.push_back(mail_adr_03);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("mail_address_04", &mail_adr_04); if (0 != mail_adr_04.compare(ini_file->not_found_in_ini_file)) mail_address_vector.push_back(mail_adr_04);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("mail_address_05", &mail_adr_05); if (0 != mail_adr_05.compare(ini_file->not_found_in_ini_file)) mail_address_vector.push_back(mail_adr_05);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("mail_address_06", &mail_adr_06); if (0 != mail_adr_06.compare(ini_file->not_found_in_ini_file)) mail_address_vector.push_back(mail_adr_06);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("mail_address_07", &mail_adr_07); if (0 != mail_adr_07.compare(ini_file->not_found_in_ini_file)) mail_address_vector.push_back(mail_adr_07);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("mail_address_08", &mail_adr_08); if (0 != mail_adr_08.compare(ini_file->not_found_in_ini_file)) mail_address_vector.push_back(mail_adr_08);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("mail_address_09", &mail_adr_09); if (0 != mail_adr_09.compare(ini_file->not_found_in_ini_file)) mail_address_vector.push_back(mail_adr_09);	// if we have a valid value for the mailaddress then we save it in the vector
	wait_cycles = 1000*wait_time_next_message / wait_time_cycle;	// we calculate the # of wait-cycles we have to wait before we check our directory for incoming files again
		
};	// end of mail_file_processor_class::init_message_processor

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This method handles the sending of WOL-command and dealing with the result.
// Input: 	wol_number: the number of the device to which we want to send the WOL-command ("00" ... "09")
//			snd = the sender to which we have to send a message to
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void mail_file_processor_class::handle_sending_wol(const std::string wol_number, const std::string *reciever) {
std::string logentry = "We send WOL to device #" + wol_number + " ...";
std::string tempstring;
std::string result;

	ini_file->get_value("WOL" + wol_number, &tempstring);
	log_file->addmessage(logfile_class::logentrytype::info, logentry);
	try {	send_WOL(&tempstring); 		// we send WOL-command to device
		std::this_thread::sleep_for(std::chrono::seconds(check_online_status_waittime));		// we wait a specific time until device may become online
		if (true == ini_file->get_value("hostname_" + wol_number, &tempstring)) {	// if we have a hostname, we use hostname
			if (tempstring != ini_file->empty_variable) {	// if we have a hostname
				if (true == check_onlinestatus(&tempstring, &result)) {	// we check online-status of device		
					log_file->addmessage(logfile_class::logentrytype::info, result);	// we add a line to our logfile
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				} else {
					log_file->addmessage(logfile_class::logentrytype::warning, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				}  
			} else {	// we do not have a hostname but an IP-address
				ini_file->get_value("ip_address_" + wol_number, &tempstring);
				if (true == check_onlinestatus(&tempstring, &result)) {	// we check online-status of device		
					log_file->addmessage(logfile_class::logentrytype::info, result);	// we add a line to our logfile
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				} else {
					log_file->addmessage(logfile_class::logentrytype::warning, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				}
			}
		}	
	} // end of try-block
	catch (const std::invalid_argument &acdc) {
		log_file->addmessage(logfile_class::logentrytype::error, acdc.what());
		result.assign(acdc.what());
		create_message_to_send(&result, reciever);	// we setup a message to the sender 
	} 	// end of catch-block
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This method handles the sending of WOL-status- command and dealing with the result.
// Input: 	wol_number: the number of the device to which we want to send the WOL-command ("00" ... "09")
//			snd = the sender to which we have to send a message to
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void mail_file_processor_class::handle_sending_wol_status(const std::string wol_number, const std::string *reciever) {
std::string logentry = "We send WOL-status to device #" + wol_number + " ...";
std::string tempstring; //, wol_stat = "WOL" + wol_number;
std::string result;
	
	log_file->addmessage(logfile_class::logentrytype::info, logentry);
	try {
		if (true == ini_file->get_value("hostname_" + wol_number, &tempstring)) {
			if (tempstring != ini_file->empty_variable) {	// if we have a hostname	
				if (true == check_onlinestatus(&tempstring, &result)) {
					log_file->addmessage(logfile_class::logentrytype::info, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				} else {
					log_file->addmessage(logfile_class::logentrytype::warning, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				}
			} else {	// we do not have a hostname but an ip-adress:
				ini_file->get_value("ip_address_" + wol_number, &tempstring);
				if (true == check_onlinestatus(&tempstring, &result)) {
					log_file->addmessage(logfile_class::logentrytype::info, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				} else {
					log_file->addmessage(logfile_class::logentrytype::warning, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				}
			}
		}	
	}	// end of try-block
	
	catch (const std::invalid_argument &acdc) {
		log_file->addmessage(logfile_class::logentrytype::error, acdc.what());
		result.assign(acdc.what());
		create_message_to_send(&result, reciever);	// we setup a message to the sender 
	} 	// end of catch-block
	
}; // end of mail_file_processor_class::handle_sending_wol_status


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We try to get the command saved in the file for mails: we open the file and try to get the sender of the mail, the command in the file
// Parameters: path = complete path of the file we want to read from; cmd = command we find in the file, snd = sender who send the mail to us
// We throw an exception std::invalid_argument when we do not find valid data for command or sender
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void mail_file_processor_class::read_command_from_file(const std::filesystem::path path, std::string *cmd, std::string *snd ) {
std::ifstream mail_file;			// the file which contains the mail 
std::ofstream mail_processed_file;	// the processed file
std::string from_line, subject_line;	// 1st line will hold the sender of the mail, 2nd line will hold the subject of the mail
// int smaler_as, bigger_as; 	// smaler_as = "<" : this is char before the mailaddress starts; bigger_as = ">" : this is last char past mailadress
std::string log_entry;

	mail_file.open(path);	// we open the file for reading
	if (true == mail_file.is_open()) {	// if we can open the file for reading
		std::getline(mail_file, from_line);		// we save at first the line which holds the sender
		std::getline(mail_file, subject_line);	// we save at second the line with the command
		mail_file.close();	// no more action will happen on the file, we close it		
		// smaler_as = from_line.find('<'); bigger_as = from_line.find('>'); // we get the position of '<' and '>' in our line
		//if (smaler_as < bigger_as) {	// if we have both signs in our line and if '<' comes before '>' (=bigger_as must be greater than smaller_as)
		*snd = from_line.substr(6, from_line.length() - 6);	// we copy all signs beyond "From: " // we copy all signs between '<' and '>' to our sender
			// for (unsigned int char_counter = 0; char_counter < snd->length(); ++char_counter) (*snd)[char_counter] = std::toupper((*snd)[char_counter]);	// we change all chars to upcase for more easy comparision
/*
		} else {	// we do not have a mailaddress in our line: we will exit here:
			log_entry = "Mailfile ";	// we prepare an entry for logfile
			log_entry.append(path);
			log_entry.append(" does not have a valid mailaddress.");
			throw std::invalid_argument(log_entry);	// it does not make sense to log here: after we have tried to get values from the file we will save the received file under a diffrent name and we should write the new name in our logfile and not the original name
			
		}	// end of if (smaler_as < bigger_as)
*/
		// now we get the comand from the file:
		if (subject_line.find("Subject: ") == 0) {	// if we have "Subject: " in beginning of the line:
			*cmd = subject_line.substr(9, subject_line.length() - 9);	// we copy all chars past 'Subject: ', this is our command; 9 = # of chars for 'Subject: '
			//for (unsigned int char_counter = 0; char_counter < cmd->length(); ++char_counter) (*cmd)[char_counter] = std::toupper((*cmd)[char_counter]);	// we change all chars to upcase for more easy comparision
		} else {	// we did not find "Subject: " in our line, we exit here
			log_entry = "Mailfile ";	// we prepare an entry for logfile
			log_entry.append(path);
			log_entry.append(" does not contain 'Subject: ' to read a command.");
			throw std::invalid_argument(log_entry);	// it does not make sense to log here: after we have tried to get values from the file we will save the received file under a diffrent name and we should write the new name in our logfile and not the original name
		}
	} else {	// we could not open file for reading
		mail_file.close();
		log_entry = "Mailfile ";	// we prepare an entry for logfile
		log_entry.append(path);
		log_entry.append(" could not be opened for reading.");
		throw std::invalid_argument(log_entry);	// it does not make sense to log here: after we have tried to get values from the file we will save the received file under a diffrent name and we should write the new name in our logfile and not the original name
	}	// end of we could not open file for reading
	
};	// end of  mail_file_processor_class::read_command_from_file

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We setup a message which is send back to sender so that sender gets a response what will happen. Or to send back back any results like the configuration 
// or a help text
// Input: message = the message we want to send back to the sender; sender = sender who has send us the message and we want to send an answer back to.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void mail_file_processor_class::create_message_to_send(const std::string *message, const std::string *sender) {
std::random_device rand_dev;	// this random-device is needed to initialize the random-generator so that we get always different random number sequence
std::default_random_engine random_generator(rand_dev());		// we initialize random generator so that we get every cycle different numbers	
std::uniform_int_distribution<int> int_distribution(0,999999);	// we want to have numbers between 0 and 999.999	
int number_to_convert;		// this integer will be converted to a string and makes the filename unique
std::chrono::time_point<std::chrono::system_clock> systemtime;	// the current point of time
std::time_t timepoint;
char timestamp[25];
std::string filename; 	// the name of file we will create
std::string log_entry;	
std::ofstream mail_file;	// the file which contains the message in form of a mail
	
	number_to_convert = int_distribution(random_generator);	// we get an integer by random between 0 and 999.999
	systemtime = std::chrono::system_clock::now();					// we get the point of time
	timepoint = std::chrono::system_clock::to_time_t(systemtime);	// convert current systemtime to a value that represents date + time
	std::strftime(timestamp, sizeof(timestamp), "%d.%m.%Y_%X_", std::localtime(&timepoint));	// we get the timestamp in format dd.mm.yyyy hh:minmin:secsec
	//ini_file->get_value("path_outgoing_mail", &filename);		// we get first the path of the outgoing mails
	filename.assign(outbound_path);
	filename.append(timestamp);
	filename.append(std::to_string(number_to_convert));
	filename.append(".txt");
	log_entry = "We will setup a mail for ";
	log_entry.append(*sender);
	log_file->addmessage(logfile_class::logfile_class::logentrytype::info, log_entry);
	mail_file.open(filename, std::ios_base::app | std::ios_base::in);	// we open the file for writing the conent to
	if (true == mail_file.is_open()) {	// if we can open the file for creating the message
		if (message->length() > max_chars_for_subject_in_mail) {	// if we have more chars than allowed to be stored in the subject:
			mail_file << "To: " << *sender << '\n' << "Subject: Mail from message2action, see body of mail for details\n" << *message;
		} else {	// we can save the message in the subject of the mail
			mail_file << "To: " << *sender << '\n' << "Subject: " << *message << "\nMail from message2action, see subject of mail for details.";
		}
		mail_file.close();	// we close the file
		log_entry = "Outgoing mail was created: ";
		log_entry.append(filename);
		log_file->addmessage(logfile_class::logfile_class::logentrytype::info, log_entry);
	} else {	// we could not open the file
		log_entry = "We could not create outgoing mail: ";
		log_entry.append(filename);
		log_file->addmessage(logfile_class::logfile_class::logentrytype::error, log_entry);
	}
	
};	// end of mail_file_processor_class::create_message_to_send

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We check if the mail address is a valid one.
// Input: snd = the mailaddress we have found in our mail-file
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool mail_file_processor_class::check_sender(const std::string *snd) {
std::string tempstring = *snd;

	if (true == capitalize_values_flag) // if we have to upcase each value we get then we turn each char to upcase
		for (unsigned int char_counter = 0; char_counter < tempstring.length(); ++char_counter) tempstring[char_counter] = std::toupper(tempstring[char_counter]);	// we change all chars to upcase for more easy comparision				
	
	std::vector<std::string>::iterator mail_address_iterator = std::find(mail_address_vector.begin(), mail_address_vector.end(), tempstring); // this iterator helps us to step through the vector with the mail-addresses
	if (mail_address_iterator != mail_address_vector.end()) return true; else return false;
	
};	// end of mail_file_processor_class::check_sender

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We create a warning for each mailaddress we know.
// Input: snd = the mailaddress we have found in our mail-file
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void mail_file_processor_class::send_warning_to_all_recipients(const std::string *snd) {
std::vector<std::string>::iterator mail_address_iterator;
std::string message;

	for (mail_address_iterator = mail_address_vector.begin(); mail_address_iterator != mail_address_vector.end(); ++mail_address_iterator) {
		message = "Warning: we received a mail from unknown receiver: ";
		message.append(*snd);
		create_message_to_send(&message, &(*mail_address_iterator));
	}
	
};	// end of mail_file_processor_class::send_warning_to_all_recipients

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We take the command from the mail and execute the command (e.g. stop program, reboot pi, perform user-defined program)
// Input: cmd = the command as a string we have to execute; snd = the sender who has send the message to us; br_flag = the flag to tell the calling function 
// that we have to break the loop. 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void mail_file_processor_class::execute_command(const std::string *cmd, const std::string *snd, bool *br_flag) {
command_type cmd_type;	// the type of the command we have received
std::string tempstring;
std::string result;
std::string logentry;

	if (true == capitalize_values_flag) { 	// if we have to upcase each value we get then we turn each char to upcase
		tempstring.assign(*cmd);
		for (unsigned int char_counter = 0; char_counter < tempstring.length(); ++char_counter) tempstring[char_counter] = std::toupper(tempstring[char_counter]);	// we change all chars to upcase for more easy comparision				
		cmd_map_iterator = cmd_map.find(tempstring);	// we try to find the command-type related to the command we have found in our message
	} else {	// we do not have to upcase each char so we search with original value
		tempstring.assign(*cmd);
		cmd_map_iterator = cmd_map.find(*cmd);	// we try to find the command-type related to the command we have found in our message
	}
		
	if (cmd_map_iterator != cmd_map.end()) {
		cmd_type = cmd_map[tempstring]; // if we have the command in our map then we get the command-type
		tempstring = "Mailfileprocessor: We process command '";
		tempstring.append(*cmd);
		tempstring.append("'");
		log_file->addmessage(logfile_class::logentrytype::info, tempstring);	
		switch (cmd_type) {
			case command_type::stop : ++processed_messages_counter; processing_flag  = false; *br_flag = true; tempstring = "We will stop message2action now."; create_message_to_send(&tempstring, snd);  break;
			case command_type::reboot : ++processed_messages_counter; processing_flag  = false; *br_flag = true; tempstring = "We will reboot the computer in 1 minute."; create_message_to_send(&tempstring, snd); sync(); system("sudo shutdown -r 1 \"message2action caused reboot in 1 minute!\" &"); break;	
			case command_type::shutdown : ++processed_messages_counter; processing_flag  = false; *br_flag = true; tempstring = "We will shutdown the computer in 1 minute."; create_message_to_send(&tempstring, snd); sync(); system("sudo shutdown -h 1 \"message2action caused a shutdown in 1 minute!\" &"); break;
			case command_type::status : ++processed_messages_counter; setup_status(&tempstring); create_message_to_send(&tempstring, snd); break;
			case command_type::config : ++processed_messages_counter; ini_file->setup_ini_file_content(&tempstring); create_message_to_send(&tempstring, snd); break;
			case command_type::help : ++processed_messages_counter; create_message_to_send(&help_text, snd); break;
			case command_type::questionmark : ++processed_messages_counter; create_message_to_send(&help_text, snd); break;
			case command_type::user_defined_00 : ++processed_messages_counter; system(cmd_00.c_str()); break;
			case command_type::user_defined_01 : ++processed_messages_counter; system(cmd_01.c_str()); break;
			case command_type::user_defined_02 : ++processed_messages_counter; system(cmd_02.c_str()); break;
			case command_type::user_defined_03 : ++processed_messages_counter; system(cmd_03.c_str()); break;
			case command_type::user_defined_04 : ++processed_messages_counter; system(cmd_04.c_str()); break;
			case command_type::user_defined_05 : ++processed_messages_counter; system(cmd_05.c_str()); break;
			case command_type::user_defined_06 : ++processed_messages_counter; system(cmd_06.c_str()); break;
			case command_type::user_defined_07 : ++processed_messages_counter; system(cmd_07.c_str()); break;
			case command_type::user_defined_08 : ++processed_messages_counter; system(cmd_08.c_str()); break;
			case command_type::user_defined_09 : ++processed_messages_counter; system(cmd_09.c_str()); break;
			case command_type::wol_00 : ++processed_messages_counter; handle_sending_wol("00", snd); break;
			case command_type::wol_01 : ++processed_messages_counter; handle_sending_wol("01", snd); break;
			case command_type::wol_02 : ++processed_messages_counter; handle_sending_wol("02", snd); break;
			case command_type::wol_03 : ++processed_messages_counter; handle_sending_wol("03", snd); break;
			case command_type::wol_04 : ++processed_messages_counter; handle_sending_wol("04", snd); break;
			case command_type::wol_05 : ++processed_messages_counter; handle_sending_wol("05", snd); break;
			case command_type::wol_06 : ++processed_messages_counter; handle_sending_wol("06", snd); break;
			case command_type::wol_07 : ++processed_messages_counter; handle_sending_wol("07", snd); break;
			case command_type::wol_08 : ++processed_messages_counter; handle_sending_wol("08", snd); break;
			case command_type::wol_09 : ++processed_messages_counter; handle_sending_wol("09", snd); break;
			case command_type::wol_status_00 : ++processed_messages_counter; handle_sending_wol_status("00", snd); break;
			case command_type::wol_status_01 : ++processed_messages_counter; handle_sending_wol_status("01", snd); break;
			case command_type::wol_status_02 : ++processed_messages_counter; handle_sending_wol_status("02", snd); break;
			case command_type::wol_status_03 : ++processed_messages_counter; handle_sending_wol_status("03", snd); break;
			case command_type::wol_status_04 : ++processed_messages_counter; handle_sending_wol_status("04", snd); break;
			case command_type::wol_status_05 : ++processed_messages_counter; handle_sending_wol_status("05", snd); break;
			case command_type::wol_status_06 : ++processed_messages_counter; handle_sending_wol_status("06", snd); break;
			case command_type::wol_status_07 : ++processed_messages_counter; handle_sending_wol_status("07", snd); break;
			case command_type::wol_status_08 : ++processed_messages_counter; handle_sending_wol_status("08", snd); break;
			case command_type::wol_status_09 : ++processed_messages_counter; handle_sending_wol_status("09", snd); break;
			default: break; 
		} // end of switch
	} else {	// we do not know the command we have received
		tempstring = "The command '";
		tempstring.append(*cmd);
		tempstring.append("' is not defined as a valid command.");
		log_file->addmessage(logfile_class::logentrytype::warning, tempstring);	
		++warning_counter;
	}
};	// end of mail_file_processor_class::execute_command

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We process all files we have in our incoming directory
// we run in a loop until the processing-flag becomes false (=we have received a command for stopping our programm or rebooting the computer).
// In this loop we step through all files in our directory for incoming messages. From each file we try to get the command and the mailaddress from the sender.
// After we have read the command we save the received file in our directory for processed files and delete the original-file.
// Now we execute the command in the file. 
// After that we take next file. NOTE: We need to process each file separatly because if we get command to stop the program or to reboot the computer we have
// to stop immediatly and to leave all other files untouched.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void mail_file_processor_class::process_mail_files() {
std::filesystem::path inbound_p(inbound_path);
std::filesystem::directory_iterator path_iterator(inbound_p);	
unsigned int cycle_counter = 0;		// counts how many times we have waited for checking processing_flag again
bool break_flag = false;
std::string log_entry;
std::chrono::hours garbage_collection_hours(0);	// we initialize the number of hours with 0
std::chrono::time_point<std::chrono::system_clock> garbage_collection_time(garbage_collection_hours);	// we initialize point of time with 0
std::string path_for_garbage_collection;

	while (true == processing_flag) {	// we run this while-loop until someone tells us to stop processing by changing the flag to false. 
		if (0 == cycle_counter) {	// if we start with our cycle for the very first time or if counter is reseted
			#ifdef devstage
			log_file->addmessage(logfile_class::logentrytype::info, "We look for mail files now ...");
			#endif
			for (std::filesystem::directory_entry file: std::filesystem::directory_iterator(inbound_path)) {	// we step through all files we find in our path
				if (true == file.is_regular_file()) {	// if we have a regular file and not something else like directory we want to have the command in the file
					try { 
						read_command_from_file(file.path(), &command, &sender); 	// we try to read the command from the file and to get the sender of the file	
						if (false == check_sender(&sender)) {	// if we have a sender which is not known to us
							send_warning_to_all_recipients(&sender);	// we send a warning to all mail-addresses we know
							++warning_counter;	// we increase counter for warnings
							log_entry = "We have received a mail from unknown sender: ";
							log_entry.append(sender);
							log_file->addmessage(logfile_class::logentrytype::warning, log_entry);
						} else {	// the sender is OK, we can execute the command
							execute_command(&command, &sender, &break_flag);	// we execute the command in the file
						}
					} catch (const std::invalid_argument &acdc) {	// we will use the content of the exception for logging:
						log_file->addmessage(logfile_class::logentrytype::error, acdc.what());}
				
					// we have to try to move the file to the processed directory under any circumstances so that we do not process it a further time 
					try {save_incoming_file(file.path());	// we save the original file in our directory for processed files and delete the original file
					} 	catch (const std::invalid_argument &acdc) {	// we will use the content for logging:
							log_file->addmessage(logfile_class::logentrytype::error, acdc.what());}
						catch (const std::string &logentry) {
							log_file->addmessage(logfile_class::logentrytype::info, logentry);}
					
					if (true == break_flag) break;	// we leave the for-loop because we have to stop program, reboot or shutdown the computer
				} // end of if-case
			} // end of for-loop for stepping through all files in directory
			++cycle_counter;	// we increase # of the cycles	
		} else { // we have to wait now in our thread to save processor-time:
			++cycle_counter;	// we increase # of the cycles	
			if (cycle_counter == wait_cycles) cycle_counter = 0; 	// we start again with our processing by resetting our counter to 0: this is done if we have waited enough times
			else std::this_thread::sleep_for(std::chrono::milliseconds(wait_time_cycle));	// we sleep for a small amount of time only if we did not have enough wait cycles
		}
		// we do garbage-collection:
		collect_garbage(&processed_path, maximum_age_in_days, &garbage_collection_time, &initial_garbage_collection_flag);		// we delete all files which are older than specified	
	}	// end of while-loop
};	// end of mail_file_processor_class::process_mail_files

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We take the command from the mail and execute the command (e.g. stop program, reboot pi, perform user-defined program)
// Input: cmd = the command as a string we have to execute; snd = the sender who has send the message to us; br_flag = the flag to tell the calling function 
// that we have to break the loop. 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void telegram_file_processor_class::execute_command(const std::string *cmd, const std::string *snd, bool *br_flag) {
command_type cmd_type;	// the type of the command we have received
std::string tempstring;
std::string result;
std::string logentry;

	if (true == capitalize_values_flag) { 	// if we have to upcase each value we get then we turn each char to upcase
		tempstring.assign(*cmd);
		for (unsigned int char_counter = 0; char_counter < tempstring.length(); ++char_counter) tempstring[char_counter] = std::toupper(tempstring[char_counter]);	// we change all chars to upcase for more easy comparision				
		cmd_map_iterator = cmd_map.find(tempstring);	// we try to find the command-type related to the command we have found in our message
	} else {	// we do not have to upcase each char so we search with original value
		tempstring.assign(*cmd);
		cmd_map_iterator = cmd_map.find(*cmd);	// we try to find the command-type related to the command we have found in our message
	}
		
	if (cmd_map_iterator != cmd_map.end()) {
		cmd_type = cmd_map[tempstring]; // if we have the command in our map then we get the command-type
		tempstring = "Telegram file processor: We process command '";
		tempstring.append(*cmd);
		tempstring.append("'");
		log_file->addmessage(logfile_class::logentrytype::info, tempstring);	
		switch (cmd_type) {
			case command_type::stop : ++processed_messages_counter; processing_flag  = false; *br_flag = true; tempstring = "We will stop message2action now."; create_message_to_send(&tempstring, snd);  break;
			case command_type::reboot : ++processed_messages_counter; processing_flag  = false; *br_flag = true; tempstring = "We will reboot the computer in 1 minute."; create_message_to_send(&tempstring, snd); sync(); system("sudo shutdown -r 1 \"message2action caused reboot in 1 minute!\" &"); break;	
			case command_type::shutdown : ++processed_messages_counter; processing_flag  = false; *br_flag = true; tempstring = "We will shutdown the computer in 1 minute."; create_message_to_send(&tempstring, snd); sync(); system("sudo shutdown -h 1 \"message2action caused a shutdown in 1 minute!\" &"); break;
			case command_type::status : ++processed_messages_counter; setup_status(&tempstring); create_message_to_send(&tempstring, snd); break;
			case command_type::config : ++processed_messages_counter; ini_file->setup_ini_file_content(&tempstring); create_message_to_send(&tempstring, snd); break;
			case command_type::help : ++processed_messages_counter; create_message_to_send(&help_text, snd); break;
			case command_type::questionmark : ++processed_messages_counter; create_message_to_send(&help_text, snd); break;
			case command_type::user_defined_00 : ++processed_messages_counter; system(cmd_00.c_str()); break;
			case command_type::user_defined_01 : ++processed_messages_counter; system(cmd_01.c_str()); break;
			case command_type::user_defined_02 : ++processed_messages_counter; system(cmd_02.c_str()); break;
			case command_type::user_defined_03 : ++processed_messages_counter; system(cmd_03.c_str()); break;
			case command_type::user_defined_04 : ++processed_messages_counter; system(cmd_04.c_str()); break;
			case command_type::user_defined_05 : ++processed_messages_counter; system(cmd_05.c_str()); break;
			case command_type::user_defined_06 : ++processed_messages_counter; system(cmd_06.c_str()); break;
			case command_type::user_defined_07 : ++processed_messages_counter; system(cmd_07.c_str()); break;
			case command_type::user_defined_08 : ++processed_messages_counter; system(cmd_08.c_str()); break;
			case command_type::user_defined_09 : ++processed_messages_counter; system(cmd_09.c_str()); break;
			case command_type::wol_00 : ++processed_messages_counter; handle_sending_wol("00", snd); break;
			case command_type::wol_01 : ++processed_messages_counter; handle_sending_wol("01", snd); break;
			case command_type::wol_02 : ++processed_messages_counter; handle_sending_wol("02", snd); break;
			case command_type::wol_03 : ++processed_messages_counter; handle_sending_wol("03", snd); break;
			case command_type::wol_04 : ++processed_messages_counter; handle_sending_wol("04", snd); break;
			case command_type::wol_05 : ++processed_messages_counter; handle_sending_wol("05", snd); break;
			case command_type::wol_06 : ++processed_messages_counter; handle_sending_wol("06", snd); break;
			case command_type::wol_07 : ++processed_messages_counter; handle_sending_wol("07", snd); break;
			case command_type::wol_08 : ++processed_messages_counter; handle_sending_wol("08", snd); break;
			case command_type::wol_09 : ++processed_messages_counter; handle_sending_wol("09", snd); break;
			case command_type::wol_status_00 : ++processed_messages_counter; handle_sending_wol_status("00", snd); break;
			case command_type::wol_status_01 : ++processed_messages_counter; handle_sending_wol_status("01", snd); break;
			case command_type::wol_status_02 : ++processed_messages_counter; handle_sending_wol_status("02", snd); break;
			case command_type::wol_status_03 : ++processed_messages_counter; handle_sending_wol_status("03", snd); break;
			case command_type::wol_status_04 : ++processed_messages_counter; handle_sending_wol_status("04", snd); break;
			case command_type::wol_status_05 : ++processed_messages_counter; handle_sending_wol_status("05", snd); break;
			case command_type::wol_status_06 : ++processed_messages_counter; handle_sending_wol_status("06", snd); break;
			case command_type::wol_status_07 : ++processed_messages_counter; handle_sending_wol_status("07", snd); break;
			case command_type::wol_status_08 : ++processed_messages_counter; handle_sending_wol_status("08", snd); break;
			case command_type::wol_status_09 : ++processed_messages_counter; handle_sending_wol_status("09", snd); break;
			default: break; 
		} // end of switch
	} else {	// we do not know the command we have received
		tempstring = "The command '";
		tempstring.append(*cmd);
		tempstring.append("' is not defined as a valid command.");
		log_file->addmessage(logfile_class::logentrytype::warning, tempstring);	
		++warning_counter;
	}
};	// end of telegram_file_processor_class::execute_command

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This method handles the sending of WOL-command and dealing with the result.
// Input: 	wol_number: the number of the device to which we want to send the WOL-command ("00" ... "09")
//			snd = the sender to which we have to send a message to
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void telegram_file_processor_class::handle_sending_wol(const std::string wol_number, const std::string *reciever) {
std::string logentry = "We send WOL to device #" + wol_number + " ...";
std::string tempstring;
std::string result;

	ini_file->get_value("WOL" + wol_number, &tempstring);
	log_file->addmessage(logfile_class::logentrytype::info, logentry);
	try {	send_WOL(&tempstring); 		// we send WOL-command to device
		std::this_thread::sleep_for(std::chrono::seconds(check_online_status_waittime));		// we wait a specific time until device may become online
		if (true == ini_file->get_value("hostname_" + wol_number, &tempstring)) {	// if we have a hostname, we use hostname
			if (tempstring != ini_file->empty_variable) {	// if we have a hostname
				if (true == check_onlinestatus(&tempstring, &result)) {	// we check online-status of device		
					log_file->addmessage(logfile_class::logentrytype::info, result);	// we add a line to our logfile
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				} else {
					log_file->addmessage(logfile_class::logentrytype::warning, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				}  
			} else {	// we do not have a hostname but an IP-address
				ini_file->get_value("ip_address_" + wol_number, &tempstring);
				if (true == check_onlinestatus(&tempstring, &result)) {	// we check online-status of device		
					log_file->addmessage(logfile_class::logentrytype::info, result);	// we add a line to our logfile
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				} else {
					log_file->addmessage(logfile_class::logentrytype::warning, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				}
			}
		}	
	} // end of try-block
	catch (const std::invalid_argument &acdc) {
		log_file->addmessage(logfile_class::logentrytype::error, acdc.what());
		result.assign(acdc.what());
		create_message_to_send(&result, reciever);	// we setup a message to the sender 
	} 	// end of catch-block
};	// end of telegram_file_processor_class::handle_sending_wol

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This method handles the sending of WOL-status- command and dealing with the result.
// Input: 	wol_number: the number of the device to which we want to send the WOL-command ("00" ... "09")
//			snd = the sender to which we have to send a message to
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void telegram_file_processor_class::handle_sending_wol_status(const std::string wol_number, const std::string *reciever) {
std::string logentry = "We send WOL-status to device #" + wol_number + " ...";
std::string tempstring; //, wol_stat = "WOL" + wol_number;
std::string result;
	
	log_file->addmessage(logfile_class::logentrytype::info, logentry);
	try {
		if (true == ini_file->get_value("hostname_" + wol_number, &tempstring)) {
			if (tempstring != ini_file->empty_variable) {	// if we have a hostname	
				if (true == check_onlinestatus(&tempstring, &result)) {
					log_file->addmessage(logfile_class::logentrytype::info, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				} else {
					log_file->addmessage(logfile_class::logentrytype::warning, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				}
			} else {	// we do not have a hostname but an ip-adress:
				ini_file->get_value("ip_address_" + wol_number, &tempstring);
				if (true == check_onlinestatus(&tempstring, &result)) {
					log_file->addmessage(logfile_class::logentrytype::info, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				} else {
					log_file->addmessage(logfile_class::logentrytype::warning, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				}
			}
		}	
	}	// end of try-block

	catch (const std::invalid_argument &acdc) {
		log_file->addmessage(logfile_class::logentrytype::error, acdc.what());
		result.assign(acdc.what());
		create_message_to_send(&result, reciever);	// we setup a message to the sender 
	} 	// end of catch-block
	
}; // end of telegram_file_processor_class::handle_sending_wol_status

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We process all files we have in our incoming directory
// we run in a loop until the processing-flag becomes false (=we have received a command for stopping our programm or rebooting the computer).
// In this loop we step through all files in our directory for incoming messages. From each file we try to get the command and the mailaddress from the sender.
// After we have read the command we save the received file in our directory for processed files and delete the original-file.
// Now we execute the command in the file. 
// After that we take next file. NOTE: We need to process each file separatly because if we get command to stop the program or to reboot the computer we have
// to stop immediatly and to leave all other files untouched.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void telegram_file_processor_class::process_telegram_files() {
std::filesystem::path inbound_p(inbound_path);
std::filesystem::directory_iterator path_iterator(inbound_p);	

unsigned int cycle_counter = 0;		// counts how many times we have waited for checking processing_flag again
bool break_flag = false;
std::string log_entry;
std::chrono::hours garbage_collection_hours(0);	// we initialize the number of hours with 0
std::chrono::time_point<std::chrono::system_clock> garbage_collection_time(garbage_collection_hours);	// we initialize point of time with 0
std::string path_for_garbage_collection;

	while (true == processing_flag) {	// we run this while-loop until someone tells us to stop processing by changing the flag to false. 
		if (0 == cycle_counter) {	// if we start with our cycle for the very first time or if counter is reseted
			#ifdef devstage
			log_file->addmessage(logfile_class::logentrytype::info, "We look for Telegram files now ...");
			#endif
			for (std::filesystem::directory_entry file: std::filesystem::directory_iterator(inbound_path)) {	// we step through all files we find in our path
				if (true == file.is_regular_file()) {	// if we have a regular file and not something else like directory we want to have the command in the file
					try { 
						read_command_from_file(file.path(), &command, &sender); 	// we try to read the command from the file and to get the sender of the file	
						if (false == check_sender(&sender)) {	// if we have a sender which is not known to us
							send_warning_to_all_recipients(&sender);	// we send a warning to all mail-addresses we know
							++warning_counter;	// we increase counter for warnings
							log_entry = "We have received a Telegram message from unknown sender: ";
							log_entry.append(sender);
							log_file->addmessage(logfile_class::logentrytype::warning, log_entry);
						} else {	// the sender is OK, we can execute the command
							execute_command(&command, &sender, &break_flag);	// we execute the command in the file
						}
					} catch (const std::invalid_argument &acdc) {	// we will use the content of the exception for logging:
						log_file->addmessage(logfile_class::logentrytype::error, acdc.what());}
				
					// we have to try to move the file to the processed directory under any circumstances so that we do not process it a further time 
					try {save_incoming_file(file.path());	// we save the original file in our directory for processed files and delete the original file
					} 	catch (const std::invalid_argument &acdc) {	// we will use the content for logging:
							log_file->addmessage(logfile_class::logentrytype::error, acdc.what());}
						catch (const std::string &logentry) {
							log_file->addmessage(logfile_class::logentrytype::info, logentry);}

					if (true == break_flag) break;	// we leave the for-loop because we have to stop program, reboot or shutdown the computer
				} // end of if-case
			} // end of for-loop for stepping through all files in directory
			++cycle_counter;	// we increase # of the cycles	
		} else { // we have to wait now in our thread to save processor-time:
			++cycle_counter;	// we increase # of the cycles	
			if (cycle_counter - 2 == wait_cycles) cycle_counter = 0; 	// we start again with our processing by resetting our counter to 0: this is done if we have waited enough times
			else {
				// log_file->addmessage(logfile_class::logentrytype::info, "We will wait now ...");
				std::this_thread::sleep_for(std::chrono::milliseconds(wait_time_cycle));	// we sleep for a small amount of time only if we did not have enough wait cycles
				// log_file->addmessage(logfile_class::logentrytype::info, "We continue now ...");
			}	// end of else
		}
		// we do garbage-collection:
		collect_garbage(&processed_path, maximum_age_in_days, &garbage_collection_time, &initial_garbage_collection_flag);		// we delete all files which are older than specified	
	}	// end of while-loop
};	// end of telegram_file_processor_class::process_telegram_files


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We save all variables which are needed for processing mails which are saved in files. Instead restoring the values from the ini-file-class for several times
// we save them separatly.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void telegram_file_processor_class::init_message_processor() {
std::string tempstring;
		
	log_file->addmessage(logfile_class::logentrytype::info, "This is processor for Telegram-messages with files."); 
	ini_file->get_value("wait_time_next_telegram", &wait_time_next_message);
	ini_file->get_value("path_incoming_telegram", &inbound_path);
	ini_file->get_value("path_outgoing_telegram", &outbound_path);
	ini_file->get_value("path_processed_telegram", &processed_path);
	ini_file->get_value("telegram_bot_token", &telegram_token);
	ini_file->get_value("telegram_chat_id_00", &telegram_chat_id_00); if (0 != telegram_chat_id_00.compare(ini_file->not_found_in_ini_file)) telegram_chat_id_vector.push_back(telegram_chat_id_00);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("telegram_chat_id_01", &telegram_chat_id_01); if (0 != telegram_chat_id_01.compare(ini_file->not_found_in_ini_file)) telegram_chat_id_vector.push_back(telegram_chat_id_01);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("telegram_chat_id_02", &telegram_chat_id_02); if (0 != telegram_chat_id_02.compare(ini_file->not_found_in_ini_file)) telegram_chat_id_vector.push_back(telegram_chat_id_02);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("telegram_chat_id_03", &telegram_chat_id_03); if (0 != telegram_chat_id_03.compare(ini_file->not_found_in_ini_file)) telegram_chat_id_vector.push_back(telegram_chat_id_03);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("telegram_chat_id_04", &telegram_chat_id_04); if (0 != telegram_chat_id_04.compare(ini_file->not_found_in_ini_file)) telegram_chat_id_vector.push_back(telegram_chat_id_04);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("telegram_chat_id_05", &telegram_chat_id_05); if (0 != telegram_chat_id_05.compare(ini_file->not_found_in_ini_file)) telegram_chat_id_vector.push_back(telegram_chat_id_05);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("telegram_chat_id_06", &telegram_chat_id_06); if (0 != telegram_chat_id_06.compare(ini_file->not_found_in_ini_file)) telegram_chat_id_vector.push_back(telegram_chat_id_06);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("telegram_chat_id_07", &telegram_chat_id_07); if (0 != telegram_chat_id_07.compare(ini_file->not_found_in_ini_file)) telegram_chat_id_vector.push_back(telegram_chat_id_07);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("telegram_chat_id_08", &telegram_chat_id_08); if (0 != telegram_chat_id_08.compare(ini_file->not_found_in_ini_file)) telegram_chat_id_vector.push_back(telegram_chat_id_08);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("telegram_chat_id_09", &telegram_chat_id_09); if (0 != telegram_chat_id_09.compare(ini_file->not_found_in_ini_file)) telegram_chat_id_vector.push_back(telegram_chat_id_09);	// if we have a valid value for the mailaddress then we save it in the vector
	wait_cycles = 1000*wait_time_next_message / wait_time_cycle;	// we calculate the # of wait-cycles we have to wait before we check our directory for incoming files again
		
};	// end of mail_file_processor_class::init_message_processor


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We setup a message which is send back to sender so that sender gets a response what will happen. Or to send back back any results like the configuration 
// or a help text
// Input: message = the message we want to send back to the sender; sender = sender who has send us the message and we want to send an answer back to.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void telegram_file_processor_class::create_message_to_send(const std::string *message, const std::string *sender) {
std::random_device rand_dev;	// this random-device is needed to initialize the random-generator so that we get always different random number sequence
std::default_random_engine random_generator(rand_dev());		// we initialize random generator so that we get every cycle different numbers	
std::uniform_int_distribution<int> int_distribution(0,999999);	// we want to have numbers between 0 and 999.999	
int number_to_convert;		// this integer will be converted to a string and makes the filename unique
std::chrono::time_point<std::chrono::system_clock> systemtime;	// the current point of time
std::time_t timepoint;
char timestamp[25];
std::string filename; 	// the name of file we will create
std::string log_entry;	
std::ofstream telegram_file;	// the file which contains the message in form of a mail
	
	number_to_convert = int_distribution(random_generator);	// we get an integer by random between 0 and 999.999
	systemtime = std::chrono::system_clock::now();					// we get the point of time
	timepoint = std::chrono::system_clock::to_time_t(systemtime);	// convert current systemtime to a value that represents date + time
	std::strftime(timestamp, sizeof(timestamp), "%d.%m.%Y_%X_", std::localtime(&timepoint));	// we get the timestamp in format dd.mm.yyyy hh:minmin:secsec
	//ini_file->get_value("path_outgoing_mail", &filename);		// we get first the path of the outgoing mails
	filename.assign(outbound_path);
	filename.append(timestamp);
	filename.append(std::to_string(number_to_convert));
	filename.append(".txt");
	log_entry = "We will setup a telegram message for ";
	log_entry.append(*sender);
	log_file->addmessage(logfile_class::logfile_class::logentrytype::info, log_entry);
	telegram_file.open(filename, std::ios_base::app | std::ios_base::in);	// we open the file for writing the conent to
	if (true == telegram_file.is_open()) {	// if we can open the file for creating the message
		telegram_file << "Token: " << telegram_token << "\nChat-id: " << *sender << "\nMessage to bot: " << *message;
		telegram_file.close();	// we close the file
		log_entry = "Outgoing telegram message was created: ";
		log_entry.append(filename);
		log_file->addmessage(logfile_class::logfile_class::logentrytype::info, log_entry);
	} else {	// we could not open the file
		log_entry = "We could not create outgoing telegram message: ";
		log_entry.append(filename);
		log_file->addmessage(logfile_class::logfile_class::logentrytype::error, log_entry);
	}
};	// end of telegram_file_processor_class::create_message_to_send

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We try to get the command saved in the file for mails: we open the file and try to get the sender of the mail, the command in the file
// Parameters: path = complete path of the file we want to read from; cmd = command we find in the file, snd = sender who send the mail to us
// We throw an exception std::invalid_argument when we do not find valid data for command or sender
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void telegram_file_processor_class::read_command_from_file(const std::filesystem::path path, std::string *cmd, std::string *snd ) {
std::ifstream telegram_file;			// the file which contains the message from telegram
std::ofstream telegram_processed_file;	// the processed file
std::string message_from_chat, chat_id, message;	
std::string log_entry;

	telegram_file.open(path);	// we open the file for reading
	if (true == telegram_file.is_open()) {	// if we can open the file for reading
		std::getline(telegram_file, message_from_chat);		// we save at first the line which holds the sender
		std::getline(telegram_file, chat_id);		// we save at first the line which holds the sender
		std::getline(telegram_file, message);	// we save at second the line with the command
		telegram_file.close();	// no more action will happen on the file, we close it		
		// now we get the chat-id from the file:
		if (std::string::npos != chat_id.find("Chat-ID: ")) {	// if we have the string in beginning of the line, that is OK
			*snd = chat_id.substr(9, chat_id.length() - 9);	// we copy all signs behind "Chat-ID: "
		} else {	// we do not have "Chat-ID: " in our line: we will exit here:
			log_entry = "Telegram file ";	// we prepare an entry for logfile
			log_entry.append(path);
			log_entry.append(" does not have 'Chat-ID ' in 2nd line.");
			throw std::invalid_argument(log_entry);	// it does not make sense to log here: after we have tried to get values from the file we will save the received file under a diffrent name and we should write the new name in our logfile and not the original name
		}
		// now we get the comand from the file:
		if (std::string::npos != message.find("Message: ")) {	// if we have "Subject: " in beginning of the line:
			*cmd = message.substr(9, message.length() - 9);	// we copy all chars past 'Subject: ', this is our command; 9 = # of chars for 'Subject: '
			for (unsigned int char_counter = 0; char_counter < cmd->length(); ++char_counter) (*cmd)[char_counter] = std::toupper((*cmd)[char_counter]);	// we change all chars to upcase for more easy comparision
		} else {	// we did not find "Message: " in our line, we exit here
			log_entry = "Telegram file ";	// we prepare an entry for logfile
			log_entry.append(path);
			log_entry.append(" does not contain 'Message: ' in 3rd line to read a command.");
			throw std::invalid_argument(log_entry);	// it does not make sense to log here: after we have tried to get values from the file we will save the received file under a diffrent name and we should write the new name in our logfile and not the original name
		}
	} else {	// we could not open file for reading
		telegram_file.close();
		log_entry = "Telegram file ";	// we prepare an entry for logfile
		log_entry.append(path);
		log_entry.append(" could not be opened for reading.");
		throw std::invalid_argument(log_entry);	// it does not make sense to log here: after we have tried to get values from the file we will save the received file under a diffrent name and we should write the new name in our logfile and not the original name
	}	// end of we could not open file for reading
	
};	// end of  telegram_file_processor_class::read_command_from_file

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We check if the mail address is a valid one.
// Input: snd = the mailaddress we have found in our mail-file
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool telegram_file_processor_class::check_sender(const std::string *snd) {
std::string tempstring = *snd;

	if (true == capitalize_values_flag) // if we have to upcase each value we get then we turn each char to upcase
		for (unsigned int char_counter = 0; char_counter < tempstring.length(); ++char_counter) tempstring[char_counter] = std::toupper(tempstring[char_counter]);	// we change all chars to upcase for more easy comparision				
	
	std::vector<std::string>::iterator telegram_chat_id_iterator = std::find(telegram_chat_id_vector.begin(), telegram_chat_id_vector.end(), tempstring); // this iterator helps us to step through the vector with the mail-addresses
	if (telegram_chat_id_iterator != telegram_chat_id_vector.end()) return true; else return false;
	
};	// end of telegram_file_processor_class::check_sender

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We create a warning for each mailaddress we know.
// Input: snd = the mailaddress we have found in our mail-file
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void telegram_file_processor_class::send_warning_to_all_recipients(const std::string *snd) {
std::vector<std::string>::iterator telegram_chat_id_iterator;
std::string message;

	for (telegram_chat_id_iterator = telegram_chat_id_vector.begin(); telegram_chat_id_iterator != telegram_chat_id_vector.end(); ++telegram_chat_id_iterator) {
		message = "Warning: we received a Telegram message from unknown receiver: ";
		message.append(*snd);
		create_message_to_send(&message, &(*telegram_chat_id_iterator));
	}
	
};	// end of telegram_file_processor_class::send_warning_to_all_recipients

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We setup a message which is send back to sender so that sender gets a response what will happen. Or to send back back any results like the configuration 
// or a help text
// Input: message = the message we want to send back to the sender; sender = sender who has send us the message and we want to send an answer back to.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sms_file_processor_class::create_message_to_send(const std::string *message, const std::string *sender) {
std::random_device rand_dev;	// this random-device is needed to initialize the random-generator so that we get always different random number sequence
std::default_random_engine random_generator(rand_dev());		// we initialize random generator so that we get every cycle different numbers	
std::uniform_int_distribution<int> int_distribution(0,999999);	// we want to have numbers between 0 and 999.999	
int number_to_convert;		// this integer will be converted to a string and makes the filename unique
std::chrono::time_point<std::chrono::system_clock> systemtime;	// the current point of time
std::time_t timepoint;
char timestamp[25];
std::string filename; 	// the name of file we will create
std::string log_entry;	
std::ofstream sms_file;	// the file which contains the message in form of a mail
	
	// we setup now the filename for creating the message:
	number_to_convert = int_distribution(random_generator);	// we get an integer by random between 0 and 999.999
	systemtime = std::chrono::system_clock::now();					// we get the point of time
	timepoint = std::chrono::system_clock::to_time_t(systemtime);	// convert current systemtime to a value that represents date + time
	std::strftime(timestamp, sizeof(timestamp), "%d.%m.%Y_%X_", std::localtime(&timepoint));	// we get the timestamp in format dd.mm.yyyy hh:minmin:secsec
	filename.assign(outbound_path);
	filename.append(timestamp);
	filename.append(std::to_string(number_to_convert));
	filename.append(".txt");
	log_entry = "We will setup a sms-file for ";
	log_entry.append(*sender);
	log_file->addmessage(logfile_class::logfile_class::logentrytype::info, log_entry);
	sms_file.open(filename);	// we try to open the file for writing
	if (true == sms_file.is_open()) {	// if we can open the file for creating the message
		sms_file << "To: " << *sender << "\n\n" << *message;
		sms_file.close();	// we close the file
		log_entry = "Outgoing sms-file was created: ";
		log_entry.append(filename);
		log_file->addmessage(logfile_class::logfile_class::logentrytype::info, log_entry);
	} else {	// we could not open the file
		log_entry = "We could not create outgoing sms-file (maybe insufficient rights): ";
		log_entry.append(filename);
		log_file->addmessage(logfile_class::logfile_class::logentrytype::error, log_entry);
	}

};	// end of sms_file_processor_class::create_message_to_send

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We try to get the command saved in the file for mails: we open the file and try to get the sender of the mail, the command in the file
// Parameters: path = complete path of the file we want to read from; cmd = command we find in the file, snd = sender who send the mail to us
// We throw an exception std::invalid_argument when we do not find valid data for command or sender
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sms_file_processor_class::read_command_from_file(const std::filesystem::path path, std::string *cmd, std::string *snd ) {
std::ifstream sms_file;			// the file which contains the mail 
std::ofstream sms_processed_file;	// the processed file
std::string from_line, command_line;	// 1st line will hold the sender of the mail, the command_line will hold the content of the sms
//int smaler_as, bigger_as; 	// smaler_as = "<" : this is char before the mailaddress starts; bigger_as = ">" : this is last char past mailadress
std::string log_entry;

	sms_file.open(path);	// we open the file for reading
	if (true == sms_file.is_open()) {	// if we can open the file for reading
		std::getline(sms_file, from_line);		// we save at first the line which holds the sender
		if (6 < from_line.length()) { // if we have more than 6 chars for value "From: " 
			for (unsigned int tempint = 0; tempint < 13; ++tempint) std::getline(sms_file, command_line);	// we read 12 lines so that we get the command
			sms_file.close();	// no more action will happen on the file, we close it		
			*snd = from_line.substr(6, from_line.length() - 6);	// we copy all signs past "From: "
			if (command_line.length() > 0) cmd->assign(command_line); // now we get the comand from the file
			else {
				sms_file.close();
				log_entry = "sms-file ";	// we prepare an entry for logfile
				log_entry.append(path);
				log_entry.append(" does not contain valid value for the expected command.");
				throw std::invalid_argument(log_entry);	// 
			}
		} else {
			sms_file.close();
			log_entry = "sms-file ";	// we prepare an entry for logfile
			log_entry.append(path);
			log_entry.append(" does not contain valid value for parameter 'From:'.");
			throw std::invalid_argument(log_entry);	// 
		}
	} else {	// we could not open file for reading
		sms_file.close();
		log_entry = "sms-file ";	// we prepare an entry for logfile
		log_entry.append(path);
		log_entry.append(" could not be opened for reading.");
		throw std::invalid_argument(log_entry);	// 
	}	// end of we could not open file for reading
	
};	// end of  sms_file_processor_class::read_command_from_file

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We save all variables which are needed for processing mails which are saved in files. Instead restoring the values from the ini-file-class for several times
// we save them separatly.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sms_file_processor_class::init_message_processor() {
std::string tempstring;
		
	log_file->addmessage(logfile_class::logentrytype::info, "This is processor for sms with files."); 
	ini_file->get_value("wait_time_next_sms", &wait_time_next_message);
	ini_file->get_value("path_incoming_SMS", &inbound_path);
	ini_file->get_value("path_outgoing_SMS", &outbound_path);
	ini_file->get_value("path_processed_SMS", &processed_path);
	ini_file->get_value("receivingfrom_cell_phone00", &cell_phone_00); if (0 != cell_phone_00.compare(ini_file->not_found_in_ini_file)) cell_phone_vector.push_back(cell_phone_00);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("receivingfrom_cell_phone01", &cell_phone_01); if (0 != cell_phone_01.compare(ini_file->not_found_in_ini_file)) cell_phone_vector.push_back(cell_phone_01);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("receivingfrom_cell_phone02", &cell_phone_02); if (0 != cell_phone_02.compare(ini_file->not_found_in_ini_file)) cell_phone_vector.push_back(cell_phone_02);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("receivingfrom_cell_phone03", &cell_phone_03); if (0 != cell_phone_03.compare(ini_file->not_found_in_ini_file)) cell_phone_vector.push_back(cell_phone_03);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("receivingfrom_cell_phone04", &cell_phone_04); if (0 != cell_phone_04.compare(ini_file->not_found_in_ini_file)) cell_phone_vector.push_back(cell_phone_04);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("receivingfrom_cell_phone05", &cell_phone_05); if (0 != cell_phone_05.compare(ini_file->not_found_in_ini_file)) cell_phone_vector.push_back(cell_phone_05);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("receivingfrom_cell_phone06", &cell_phone_06); if (0 != cell_phone_06.compare(ini_file->not_found_in_ini_file)) cell_phone_vector.push_back(cell_phone_06);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("receivingfrom_cell_phone07", &cell_phone_07); if (0 != cell_phone_07.compare(ini_file->not_found_in_ini_file)) cell_phone_vector.push_back(cell_phone_07);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("receivingfrom_cell_phone08", &cell_phone_08); if (0 != cell_phone_08.compare(ini_file->not_found_in_ini_file)) cell_phone_vector.push_back(cell_phone_08);	// if we have a valid value for the mailaddress then we save it in the vector
	ini_file->get_value("receivingfrom_cell_phone09", &cell_phone_09); if (0 != cell_phone_09.compare(ini_file->not_found_in_ini_file)) cell_phone_vector.push_back(cell_phone_09);	// if we have a valid value for the mailaddress then we save it in the vector
	wait_cycles = 1000*wait_time_next_message / wait_time_cycle;	// we calculate the # of wait-cycles we have to wait before we check our directory for incoming files again
		
};	// end of sms_file_processor_class::init_message_processor

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We take the command from the mail and execute the command (e.g. stop program, reboot pi, perform user-defined program)
// Input: cmd = the command as a string we have to execute; snd = the sender who has send the message to us; br_flag = the flag to tell the calling function 
// that we have to break the loop. 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sms_file_processor_class::execute_command(const std::string *cmd, const std::string *snd, bool *br_flag) {
command_type cmd_type;	// the type of the command we have received
std::string tempstring;
std::string result;
std::string logentry;

	if (true == capitalize_values_flag) { 	// if we have to upcase each value we get then we turn each char to upcase
		tempstring.assign(*cmd);
		for (unsigned int char_counter = 0; char_counter < tempstring.length(); ++char_counter) tempstring[char_counter] = std::toupper(tempstring[char_counter]);	// we change all chars to upcase for more easy comparision				
		cmd_map_iterator = cmd_map.find(tempstring);	// we try to find the command-type related to the command we have found in our message
	} else {	// we do not have to upcase each char so we search with original value
		tempstring.assign(*cmd);
		cmd_map_iterator = cmd_map.find(*cmd);	// we try to find the command-type related to the command we have found in our message
	}
		
	if (cmd_map_iterator != cmd_map.end()) {
		cmd_type = cmd_map[tempstring]; // if we have the command in our map then we get the command-type
		tempstring = "SMS-file processor: We process command '";
		tempstring.append(*cmd);
		tempstring.append("'");
		log_file->addmessage(logfile_class::logentrytype::info, tempstring);	
		switch (cmd_type) {
			case command_type::stop : ++processed_messages_counter; processing_flag  = false; *br_flag = true; tempstring = "We will stop message2action now."; create_message_to_send(&tempstring, snd);  break;
			case command_type::reboot : ++processed_messages_counter; processing_flag  = false; *br_flag = true; tempstring = "We will reboot the computer in 1 minute."; create_message_to_send(&tempstring, snd); sync(); system("sudo shutdown -r 1 \"message2action caused reboot in 1 minute!\" &"); break;	
			case command_type::shutdown : ++processed_messages_counter; processing_flag  = false; *br_flag = true; tempstring = "We will shutdown the computer in 1 minute."; create_message_to_send(&tempstring, snd); sync(); system("sudo shutdown -h 1 \"message2action caused a shutdown in 1 minute!\" &"); break;
			case command_type::status : ++processed_messages_counter; setup_status(&tempstring); create_message_to_send(&tempstring, snd); break;
			case command_type::config : ++processed_messages_counter; ini_file->setup_ini_file_content(&tempstring); create_message_to_send(&tempstring, snd); break;
			case command_type::help : ++processed_messages_counter; create_message_to_send(&help_text, snd); break;
			case command_type::questionmark : ++processed_messages_counter; create_message_to_send(&help_text, snd); break;
			case command_type::user_defined_00 : ++processed_messages_counter; system(cmd_00.c_str()); break;
			case command_type::user_defined_01 : ++processed_messages_counter; system(cmd_01.c_str()); break;
			case command_type::user_defined_02 : ++processed_messages_counter; system(cmd_02.c_str()); break;
			case command_type::user_defined_03 : ++processed_messages_counter; system(cmd_03.c_str()); break;
			case command_type::user_defined_04 : ++processed_messages_counter; system(cmd_04.c_str()); break;
			case command_type::user_defined_05 : ++processed_messages_counter; system(cmd_05.c_str()); break;
			case command_type::user_defined_06 : ++processed_messages_counter; system(cmd_06.c_str()); break;
			case command_type::user_defined_07 : ++processed_messages_counter; system(cmd_07.c_str()); break;
			case command_type::user_defined_08 : ++processed_messages_counter; system(cmd_08.c_str()); break;
			case command_type::user_defined_09 : ++processed_messages_counter; system(cmd_09.c_str()); break;
			case command_type::wol_00 : ++processed_messages_counter; handle_sending_wol("00", snd); break;
			case command_type::wol_01 : ++processed_messages_counter; handle_sending_wol("01", snd); break;
			case command_type::wol_02 : ++processed_messages_counter; handle_sending_wol("02", snd); break;
			case command_type::wol_03 : ++processed_messages_counter; handle_sending_wol("03", snd); break;
			case command_type::wol_04 : ++processed_messages_counter; handle_sending_wol("04", snd); break;
			case command_type::wol_05 : ++processed_messages_counter; handle_sending_wol("05", snd); break;
			case command_type::wol_06 : ++processed_messages_counter; handle_sending_wol("06", snd); break;
			case command_type::wol_07 : ++processed_messages_counter; handle_sending_wol("07", snd); break;
			case command_type::wol_08 : ++processed_messages_counter; handle_sending_wol("08", snd); break;
			case command_type::wol_09 : ++processed_messages_counter; handle_sending_wol("09", snd); break;
			case command_type::wol_status_00 : ++processed_messages_counter; handle_sending_wol_status("00", snd); break;
			case command_type::wol_status_01 : ++processed_messages_counter; handle_sending_wol_status("01", snd); break;
			case command_type::wol_status_02 : ++processed_messages_counter; handle_sending_wol_status("02", snd); break;
			case command_type::wol_status_03 : ++processed_messages_counter; handle_sending_wol_status("03", snd); break;
			case command_type::wol_status_04 : ++processed_messages_counter; handle_sending_wol_status("04", snd); break;
			case command_type::wol_status_05 : ++processed_messages_counter; handle_sending_wol_status("05", snd); break;
			case command_type::wol_status_06 : ++processed_messages_counter; handle_sending_wol_status("06", snd); break;
			case command_type::wol_status_07 : ++processed_messages_counter; handle_sending_wol_status("07", snd); break;
			case command_type::wol_status_08 : ++processed_messages_counter; handle_sending_wol_status("08", snd); break;
			case command_type::wol_status_09 : ++processed_messages_counter; handle_sending_wol_status("09", snd); break;
			default: break; 
		} // end of switch
	} else {	// we do not know the command we have received
		tempstring = "The command '";
		tempstring.append(*cmd);
		tempstring.append("' is not defined as a valid command.");
		log_file->addmessage(logfile_class::logentrytype::warning, tempstring);	
		++warning_counter;
	}
};	// end of sms_file_processor_class::execute_command

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This method handles the sending of WOL-command and dealing with the result.
// Input: 	wol_number: the number of the device to which we want to send the WOL-command ("00" ... "09")
//			snd = the sender to which we have to send a message to
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sms_file_processor_class::handle_sending_wol(const std::string wol_number, const std::string *reciever) {
std::string logentry = "We send WOL to device #" + wol_number + " ...";
std::string tempstring;
std::string result;

	ini_file->get_value("WOL" + wol_number, &tempstring);
	log_file->addmessage(logfile_class::logentrytype::info, logentry);
	try {	send_WOL(&tempstring); 		// we send WOL-command to device
		std::this_thread::sleep_for(std::chrono::seconds(check_online_status_waittime));		// we wait a specific time until device may become online
		if (true == ini_file->get_value("hostname_" + wol_number, &tempstring)) {	// if we have a hostname, we use hostname
			if (tempstring != ini_file->empty_variable) {	// if we have a hostname
				if (true == check_onlinestatus(&tempstring, &result)) {	// we check online-status of device		
					log_file->addmessage(logfile_class::logentrytype::info, result);	// we add a line to our logfile
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				} else {
					log_file->addmessage(logfile_class::logentrytype::warning, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				}  
			} else {	// we do not have a hostname but an IP-address
				ini_file->get_value("ip_address_" + wol_number, &tempstring);
				if (true == check_onlinestatus(&tempstring, &result)) {	// we check online-status of device		
					log_file->addmessage(logfile_class::logentrytype::info, result);	// we add a line to our logfile
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				} else {
					log_file->addmessage(logfile_class::logentrytype::warning, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				}
			}
		}	
	} // end of try-block
	catch (const std::invalid_argument &acdc) {
		log_file->addmessage(logfile_class::logentrytype::error, acdc.what());
		result.assign(acdc.what());
		create_message_to_send(&result, reciever);	// we setup a message to the sender 
	} 	// end of catch-block
};	// end of sms_file_processor_class::handle_sending_wol

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This method handles the sending of WOL-status-command and dealing with the result.
// Input: 	wol_number: the number of the device to which we want to send the WOL-command ("00" ... "09")
//			snd = the sender to which we have to send a message to
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sms_file_processor_class::handle_sending_wol_status(const std::string wol_number, const std::string *reciever) {
std::string logentry = "We send WOL-status to device #" + wol_number + " ...";
std::string tempstring; //, wol_stat = "WOL" + wol_number;
std::string result;
	
	log_file->addmessage(logfile_class::logentrytype::info, logentry);
	try {
		if (true == ini_file->get_value("hostname_" + wol_number, &tempstring)) {
			if (tempstring != ini_file->empty_variable) {	// if we have a hostname	
				if (true == check_onlinestatus(&tempstring, &result)) {
					log_file->addmessage(logfile_class::logentrytype::info, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				} else {
					log_file->addmessage(logfile_class::logentrytype::warning, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				}
			} else {	// we do not have a hostname but an ip-adress:
				ini_file->get_value("ip_address_" + wol_number, &tempstring);
				if (true == check_onlinestatus(&tempstring, &result)) {
					log_file->addmessage(logfile_class::logentrytype::info, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				} else {
					log_file->addmessage(logfile_class::logentrytype::warning, result);
					create_message_to_send(&result, reciever);	// we setup a message to the sender 
				}
			}
		}	
	}	// end of try-block
	catch (const std::invalid_argument &acdc) {
		log_file->addmessage(logfile_class::logentrytype::error, acdc.what());
		result.assign(acdc.what());
		create_message_to_send(&result, reciever);	// we setup a message to the sender 
	} 	// end of catch-block
	
}; // end of mail_file_processor_class::handle_sending_wol_status

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We check if the mail address is a valid one.
// Input: snd = the mailaddress we have found in our mail-file
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool sms_file_processor_class::check_sender(const std::string *snd) {
std::string tempstring = *snd;

	if (true == capitalize_values_flag) // if we have to upcase each value we get then we turn each char to upcase
		for (unsigned int char_counter = 0; char_counter < tempstring.length(); ++char_counter) tempstring[char_counter] = std::toupper(tempstring[char_counter]);	// we change all chars to upcase for more easy comparision				
	
	std::vector<std::string>::iterator cell_phone_iterator = std::find(cell_phone_vector.begin(), cell_phone_vector.end(), tempstring); // this iterator helps us to step through the vector with the mail-addresses
	if (cell_phone_iterator != cell_phone_vector.end()) return true; else return false;
	
};	// end of sms_file_processor_class::check_sender

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We create a warning for each mailaddress we know.
// Input: snd = the mailaddress we have found in our mail-file
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sms_file_processor_class::send_warning_to_all_recipients(const std::string *snd) {
std::vector<std::string>::iterator cell_phone_iterator;
std::string message;

	for (cell_phone_iterator = cell_phone_vector.begin(); cell_phone_iterator != cell_phone_vector.end(); ++cell_phone_iterator) {
		message = "Warning: we received a mail from unknown receiver: ";
		message.append(*snd);
		create_message_to_send(&message, &(*cell_phone_iterator));
	}
};	// end of mail_file_processor_class::send_warning_to_all_recipients

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// We process all files we have in our incoming directory
// we run in a loop until the processing-flag becomes false (=we have received a command for stopping our programm or rebooting the computer).
// In this loop we step through all files in our directory for incoming messages. From each file we try to get the command and the mailaddress from the sender.
// After we have read the command we save the received file in our directory for processed files and delete the original-file.
// Now we execute the command in the file. 
// After that we take next file. NOTE: We need to process each file separatly because if we get command to stop the program or to reboot the computer we have
// to stop immediatly and to leave all other files untouched.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sms_file_processor_class::process_sms_files() {
std::filesystem::path inbound_p(inbound_path);
std::filesystem::directory_iterator path_iterator(inbound_p);	
unsigned int cycle_counter = 0;		// counts how many times we have waited for checking processing_flag again
bool break_flag = false;
std::string log_entry;
std::chrono::hours garbage_collection_hours(0);	// we initialize the number of hours with 0
std::chrono::time_point<std::chrono::system_clock> garbage_collection_time(garbage_collection_hours);	// we initialize point of time with 0
std::string path_for_garbage_collection;

	while (true == processing_flag) {	// we run this while-loop until someone tells us to stop processing by changing the flag to false. 
		if (0 == cycle_counter) {	// if we start with our cycle for the very first time or if counter is reseted
			#ifdef devstage
			log_file->addmessage(logfile_class::logentrytype::info, "We look for SMS files now ...");
			#endif
			for (std::filesystem::directory_entry file: std::filesystem::directory_iterator(inbound_path)) {	// we step through all files we find in our path
				if (true == file.is_regular_file()) {	// if we have a regular file and not something else like directory we want to have the command in the file
					try { 
						read_command_from_file(file.path(), &command, &sender); 	// we try to read the command from the file and to get the sender of the file	
						if (false == check_sender(&sender)) {	// if we have a sender which is not known to us
							send_warning_to_all_recipients(&sender);	// we send a warning to all mail-addresses we know
							++warning_counter;	// we increase counter for warnings
							log_entry = "We have received a SMS from unknown sender: ";
							log_entry.append(sender);
							log_file->addmessage(logfile_class::logentrytype::warning, log_entry);
						} else {	// the sender is OK, we can execute the command
							execute_command(&command, &sender, &break_flag);	// we execute the command in the file
						}
					} catch (const std::invalid_argument &acdc) {	// we will use the content of the exception for logging:
						log_file->addmessage(logfile_class::logentrytype::error, acdc.what());}
				
					// we have to try to move the file to the processed directory under any circumstances so that we do not process it a further time 
					try {save_incoming_file(file.path());	// we save the original file in our directory for processed files and delete the original file
					} 	catch (const std::invalid_argument &acdc) {	// we will use the content for logging:
							log_file->addmessage(logfile_class::logentrytype::error, acdc.what());}
						catch (const std::string &logentry) {
							log_file->addmessage(logfile_class::logentrytype::info, logentry);}
					
					if (true == break_flag) break;	// we leave the for-loop because we have to stop program, reboot or shutdown the computer
				} // end of if-case
			} // end of for-loop for stepping through all files in directory
			++cycle_counter;	// we increase # of the cycles	
		} else { // we have to wait now in our thread to save processor-time:
			++cycle_counter;	// we increase # of the cycles	
			if (cycle_counter == wait_cycles) cycle_counter = 0; 	// we start again with our processing by resetting our counter to 0: this is done if we have waited enough times
			else std::this_thread::sleep_for(std::chrono::milliseconds(wait_time_cycle));	// we sleep for a small amount of time only if we did not have enough wait cycles
		}
		// we do garbage-collection:
		collect_garbage(&processed_path, maximum_age_in_days, &garbage_collection_time, &initial_garbage_collection_flag);		// we delete all files which are older than specified	
	}	// end of while-loop
};	// end of sms_file_processor_class::process_sms_files
