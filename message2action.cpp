/*
 message2action.cpp
 Version: 1.10
 Date: 23.12.2023
  
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

#include <functional>
#include <iostream>
#include <thread>
#include <chrono> 
#include <exception>
#include <stdexcept>
#include <filesystem>
#include <system_error>
#include <random>
#include <cstring>	// used for memset
#include <unistd.h>
#include <termios.h>	// used for changing the settings of the terminal
#include <sys/ioctl.h>	// used for reading chars from the terminal
#include <algorithm>
#include <sys/socket.h>	// used for socket-communication
#include <netinet/in.h>	// used for socket-communication
#include <atomic>		// used for proecessing_flag
#include <map>			// used for dealing with all variables of this program

#include <inifile.hpp>	// used for handling the ini-file
#include <logfile.hpp>	// used for handling with logfile
#include <stopfile.hpp>	// used for handling with stopfile
#include <m2a_ini.hpp>	// handling of ini-file for messag2action
#include <m2a_message.hpp>		// class for processing all messages (SMS, mail, Telegram ...)

#define devstage

std::atomic<bool> processing_flag = true;	// we initialize with true because we want to start processing


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// program specific class for handling stop-files. This class is derived from general class stop_file_class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class m2a_stop_file_class : public stop_file_class {

public:	
m2a_stop_file_class(const std::string file_name, const std::string command_to_search) : stop_file_class(file_name, command_to_search) {};	// we call constructor of base-class
m2a_stop_file_class(const std::string file_name) : stop_file_class(file_name) {};	// we call constructor of base-class
void thread_check_stop_file();	
void set_logfile(logfile_class *logfileclass) { logfile = logfileclass;};	// we set a pointer to logfileclass

private:
logfile_class *logfile = nullptr;	
};

void m2a_stop_file_class::thread_check_stop_file() {
stop_file_class::returncode stopfilereturncode;	// we save the return-code
	
	while (true == processing_flag) {	// we repeat this loop until processing-flag turns to false
		stopfilereturncode = check_stop_file();
		switch (stopfilereturncode) {
			case stop_file_class::returncode::NO_ERROR: processing_flag = false; break;	// no error, we have to leave now
			case stop_file_class::returncode::NO_STOP_FILE: std::this_thread::sleep_for(std::chrono::milliseconds(100)); break;	// if we do not have a stop-file we will simply sleep for a while
			case stop_file_class::returncode::STOP_FILE_DELETE_ERROR: if (logfile != nullptr) { logfile->addmessage(logfile_class::logentrytype::error, "Stop-file could not be deleted, maybe insufficient rights.");}	break; // we add a message to logfile	
			case stop_file_class::returncode::STOP_FILE_READ_ERROR: if (logfile != nullptr) { logfile->addmessage(logfile_class::logentrytype::error, "Stop-file could not be read, maybe insufficient rights.");}	break; // we add a message to logfile	
			case stop_file_class::returncode::STOP_FILE_CREATE_ERROR: if (logfile != nullptr) { logfile->addmessage(logfile_class::logentrytype::error, "Stop-file could not be created, maybe insufficient rights.");}	break; // we add a message to logfile	
			case stop_file_class::returncode::STOP_FILE_WRONG_CONTENT: if (logfile != nullptr) { logfile->addmessage(logfile_class::logentrytype::error, "Stop-file has wrong content.");}	break; // we add a message to logfile	
		}	// end of switch-loop
	}	// end of while-loop
}; // end of m2a_stop_file_class::thread_check_stop_file

	
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// program specific class for reading the keyboard in case that we do not run in daemon-mode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class m2a_terminal_class {
private:
struct termios temp_terminal;		// temporary terminal settings
struct termios current_terminal;	// current terminal setting
int tempint;	
logfile_class *logfile = nullptr;	// a pointer to an object for logging

public: 
int save_current_terminalsettings() {
	tempint = tcgetattr (STDIN_FILENO, &current_terminal); 	// we save terminal-settings in variable current_terminal	
	if ((-1 == tempint) && (nullptr != logfile)) logfile->addmessage(logfile_class::logentrytype::error, "We could no get attributes of terminal!");	
	return (tempint);
};

int set_terminalsettings() {
	temp_terminal = current_terminal;	// we save the terminal-settings 
	// we change some flags on the temporary terminalsettings:
	temp_terminal.c_iflag = temp_terminal.c_iflag & ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	temp_terminal.c_oflag = temp_terminal.c_iflag & ~(OPOST);
	temp_terminal.c_cflag = temp_terminal.c_cflag & ~(CSIZE | PARENB);
	temp_terminal.c_lflag = temp_terminal.c_lflag & ~(ECHO|ICANON|IEXTEN|ISIG);
	temp_terminal.c_cflag = temp_terminal.c_cflag | CS8;
	temp_terminal.c_cc[VMIN]  = 1;
	temp_terminal.c_cc[VTIME] = 0;
	tempint = tcsetattr (STDIN_FILENO, TCSAFLUSH, &temp_terminal);	// we change now our terminal-settings
	if ((-1 == tempint) && (nullptr != logfile)) logfile->addmessage(logfile_class::logentrytype::error, "We could not set new attributes of terminal!");	
	return (tempint);
};

int restore_current_terminalsettings() {
	tempint = tcsetattr (STDIN_FILENO, TCSANOW, &current_terminal);	
	if ((-1 == tempint) && (nullptr != logfile)) logfile->addmessage(logfile_class::logentrytype::error, "We could not restore attributes of terminal!");	
	return (tempint);
};

public:
m2a_terminal_class(logfile_class *log) : logfile(log) {}; // constructor with one parameter: the address of the logfile-object
	
}; // end of m2a_keyboard_class


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This function checks of user has hit the keyboard. If true, the global flag for processing will by set to false
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void check_for_keyboard_hit() {
	
int readchar;
int buffered_charackter = 0;

	while ((0 == buffered_charackter) && (true == processing_flag)) {	// we repeat until we find a char in buffer or processing-flag becomes false 
		ioctl(STDIN_FILENO, FIONREAD, &buffered_charackter);	// we check if we have something in the buffer because user hit the keyboard
		std::this_thread::sleep_for(std::chrono::milliseconds(100));	// we wait for 250 milli-seconds
	}

if (0 != buffered_charackter) readchar = getchar(); // we get the sign from the buffer so that buffer is empty when program ends: this only needed when user hit the keyboard
if ('c' == readchar) processing_flag = false; else processing_flag = false;	// we change processing_flag to false under any circumstances; we need the reading of readchar because compiler will produce a warning of unused variable

};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This is main
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
bool inbound_sms_file_flag, inbound_mail_file_flag, inbound_telegram_file_flag;	// these flags tell us if we have to process the messages (read from ini-file)
std::thread sms_file_thread, mail_file_thread, telegram_file_thread;	// these threads are defined empty for processing messages
std::string tempstring;

std::cout << "message2action 1.10" << std::endl;
logfile_class logfile("message2action.log", 500, true);		// we create an object to write to a logfile
logfile.addmessage(logfile_class::logentrytype::info, "We started message2action.");
m2a_stop_file_class m2a_stopfile((std::string)(argv[0]) + std::string(".stop"));	// we search for a stopfile without checking the content of the stopfile
m2a_stopfile.set_logfile(&logfile);	// we tell m2a_stopfile which object can be used for logging

m2a_ini_file_class m2a_ini_file("message2action.ini");	// we create 1 object of our ini-file for our program

// we create 1 object of each processor-type:
sms_file_processor_class sms_file_processor(message_processor_base_class::channel_type_class::is_sms_file, &m2a_ini_file, &logfile); 	// we create 1 object for processing sms-files
telegram_file_processor_class telegram_file_processor(message_processor_base_class::channel_type_class::is_telegram_file, &m2a_ini_file, &logfile); 	// we create 1 object for processing Telegram-files
mail_file_processor_class mail_file_processor(message_processor_base_class::channel_type_class::is_mail_file, &m2a_ini_file, &logfile); 	// we create 1 object for processing mail-files

m2a_terminal_class terminal(&logfile);	// we create one object for dealing with the terminal
	
// we check if we have a parameter given to the program:
if (argc == 2) {	// we have 1 parameter given to the program
	tempstring.assign(argv[1]);	// we save the given parameter
	bool valid_argument_flag = false;
	
	if (tempstring.compare("?") == 0) {
		std::cout << "For documentation please see file message2action.pdf." << std::endl;
		valid_argument_flag = true;
	}
	
	if (tempstring.compare("daemon") == 0) {	// we have to run in daemon-mode
		// we check if we have a correct ini-file:
		if (0 == m2a_ini_file.get_constructor_return_code()) {	// if we have no error with the ini-file
			try {m2a_ini_file.check_values_plausability();}
				catch (const std::invalid_argument &acdc) {	// check of plausability failed:
					logfile.addmessage(logfile_class::logentrytype::error, acdc.what());
					logfile.addmessage(logfile_class::logentrytype::info, "We finished message2action.");
					std::cout << "Programm is finished." << std::endl;
					return 1;
				}
				catch (const std::out_of_range &acdc) {
				}
		} else {
			logfile.addmessage(logfile_class::logentrytype::error, "Ini-file could not be read correctly!");
			logfile.addmessage(logfile_class::logentrytype::info, "We finished message2action.");
			std::cout << "Programm is finished." << std::endl;
			return 1; 	// we have to stop program here
		}
		
		std::cout << "We will run in daemon-mode. Call program with parameter 'stop' in another session to stop this program." << std::endl;
		logfile.addmessage(logfile_class::logentrytype::info, "We start with daemon-mode.");
		valid_argument_flag = true;
		std::thread stop_file_thread(&m2a_stop_file_class::thread_check_stop_file, &m2a_stopfile);	// we start a thread which is scanning for stop-file
		
		if (true == m2a_ini_file.get_value("inbound_sms_file_flag", &inbound_sms_file_flag)) {
			if (true == inbound_sms_file_flag) { 
				sms_file_thread = std::thread(&sms_file_processor_class::process_sms_files, &sms_file_processor);	// we start a thread which is dealing with sms	
				logfile.addmessage(logfile_class::logentrytype::info, "We started processing sms with files.");
				//sms_file_processing_thread.detach();
			}
		} else inbound_sms_file_flag = false;

		if (true == m2a_ini_file.get_value("inbound_mail_file_flag", &inbound_mail_file_flag)) {
			if (true == inbound_mail_file_flag) {
				mail_file_thread = std::thread(&mail_file_processor_class::process_mail_files, &mail_file_processor);	// we start a thread which is dealing with mails
				logfile.addmessage(logfile_class::logentrytype::info, "We started processing mails with files.");
			}
		} else inbound_mail_file_flag = false;
		
		if (true == m2a_ini_file.get_value("inbound_telegram_file_flag", &inbound_telegram_file_flag)) {
			if (true == inbound_telegram_file_flag) {
				telegram_file_thread = std::thread(&telegram_file_processor_class::process_telegram_files, &telegram_file_processor);	// we start a thread which is dealing with telegram messages
				logfile.addmessage(logfile_class::logentrytype::info, "We started processing telegrams with files.");
			}
		} else inbound_telegram_file_flag = false;

		if (true == stop_file_thread.joinable()) { 
			stop_file_thread.join();
			logfile.addmessage(logfile_class::logentrytype::info, "We joined thread for processing stop-files successfully.");
		} else logfile.addmessage(logfile_class::logentrytype::warning, "We can not join thread for processing stop-files.");
		
		if (true == inbound_sms_file_flag) {
			if (true == sms_file_thread.joinable()) {
				sms_file_thread.join();
				logfile.addmessage(logfile_class::logentrytype::info, "We joined thread for processing sms-files successfully.");
			} else logfile.addmessage(logfile_class::logentrytype::warning, "We can not join thread for processing sms-files.");
		}

		if (true == inbound_mail_file_flag) {
			if (true == mail_file_thread.joinable()) {
				mail_file_thread.join();
				logfile.addmessage(logfile_class::logentrytype::info, "We joined thread for processing mails successfully.");
			} else logfile.addmessage(logfile_class::logentrytype::warning, "We can not join thread for processing mails.");
		}	

		if (true == inbound_telegram_file_flag) {
			if (true == telegram_file_thread.joinable()) { 
				telegram_file_thread.join();
				logfile.addmessage(logfile_class::logentrytype::info, "We joined thread for processing telegram messages with files successfully.");
			} else logfile.addmessage(logfile_class::logentrytype::warning, "We can not join thread for processing telegram messages with files.");
		}	
	}	// end of parameter daemon
	
	if (0 == tempstring.compare("start")) {
		// we check if we have a correct ini-file:
		if (0 == m2a_ini_file.get_constructor_return_code()) {	// if we have no error with the ini-file
			try {m2a_ini_file.check_values_plausability();}
				catch (const std::invalid_argument &acdc) {	// check of plausability failed:
					logfile.addmessage(logfile_class::logentrytype::error, acdc.what());
					logfile.addmessage(logfile_class::logentrytype::info, "We finished message2action.");
					std::cout << "Programm is finished." << std::endl;
					return 1;
				}
				catch (const std::out_of_range &acdc) {
				}
		} else {
			logfile.addmessage(logfile_class::logentrytype::error, "Ini-file could not be read correctly!");
			logfile.addmessage(logfile_class::logentrytype::info, "We finished message2action.");
			std::cout << "Programm is finished." << std::endl;
			return 1; 	// we have to stop program here
		}
		
		std::cout << "We will run in standard-mode. Press any readable key for stopping the program." << std::endl;
		valid_argument_flag = true;
		terminal.save_current_terminalsettings();
		terminal.set_terminalsettings();
		std::thread check_keyboard(&check_for_keyboard_hit);	// we start a thread that will check if user has hit the keyboard
		std::thread stop_file_thread(&m2a_stop_file_class::thread_check_stop_file, &m2a_stopfile);	// we start a thread which is scanning for stop-file
		
		if (true == m2a_ini_file.get_value("inbound_sms_file_flag", &inbound_sms_file_flag)) {
			if (true == inbound_sms_file_flag) { 
				sms_file_thread = std::thread(&sms_file_processor_class::process_sms_files, &sms_file_processor);	// we start a thread which is dealing with sms	
			}
		} else inbound_sms_file_flag = false;

		if (true == m2a_ini_file.get_value("inbound_mail_file_flag", &inbound_mail_file_flag)) {
			if (true == inbound_mail_file_flag) mail_file_thread = std::thread(&mail_file_processor_class::process_mail_files, &mail_file_processor);	// we start a thread which is dealing with mails
		} else inbound_mail_file_flag = false;
		
		if (true == m2a_ini_file.get_value("inbound_telegram_file_flag", &inbound_telegram_file_flag)) {
			if (true == inbound_telegram_file_flag) 
				telegram_file_thread = std::thread(&telegram_file_processor_class::process_telegram_files, &telegram_file_processor);	// we start a thread which is dealing with telegram messages
		} else inbound_telegram_file_flag = false;

		if (true == stop_file_thread.joinable()) { 
			stop_file_thread.join();
			logfile.addmessage(logfile_class::logentrytype::info, "We joined thread for processing stop-files successfully.");
		} else logfile.addmessage(logfile_class::logentrytype::warning, "We can not join thread for processing stop-files.");
		
		if (true == inbound_sms_file_flag) {
			if (true == sms_file_thread.joinable()) {
				sms_file_thread.join();
				logfile.addmessage(logfile_class::logentrytype::info, "We joined thread for processing sms-files successfully.");
			} else logfile.addmessage(logfile_class::logentrytype::warning, "We can not join thread for processing sms-files.");
		}

		if (true == inbound_mail_file_flag) {
			if (true == mail_file_thread.joinable()) {
				mail_file_thread.join();
				logfile.addmessage(logfile_class::logentrytype::info, "We joined thread for processing mails successfully.");
			} else logfile.addmessage(logfile_class::logentrytype::warning, "We can not join thread for processing mails.");
		}	

		if (true == inbound_telegram_file_flag) {
			if (true == telegram_file_thread.joinable()) { 
				telegram_file_thread.join();
				logfile.addmessage(logfile_class::logentrytype::info, "We joined thread for processing telegram messages with files mails successfully.");
			} else logfile.addmessage(logfile_class::logentrytype::warning, "We can not join thread for processing telegram messages with files.");
		}	
		
		if (true == check_keyboard.joinable()) { 
			check_keyboard.join();
			logfile.addmessage(logfile_class::logentrytype::info, "We joined thread for checking for keyboard successfully.");
		} else logfile.addmessage(logfile_class::logentrytype::warning, "We can not join thread for checking for keyboards.");

		terminal.restore_current_terminalsettings();	// we set terminal to old values

	}	// end of parameter daemon

	if (tempstring.compare("stop") == 0) {	// we will now create a stop-file
		std::cout << "We will create a stop-file for stopping any running instance of message2action." << std::endl;
		valid_argument_flag = true;
		if (stop_file_class::returncode::NO_ERROR == m2a_stopfile.create_stop_file()) {
			std::cout << "Stop-file was created." << std::endl;
		} else 
			std::cout << "Stop-file was not created successfully!" << std::endl;
	}	// end of parameter stop	

	if (tempstring.compare("config") == 0) {
		// we check if we have a correct ini-file:
		if (0 == m2a_ini_file.get_constructor_return_code()) {	// if we have no error with the ini-file
			try {m2a_ini_file.check_values_plausability();}
				catch (const std::invalid_argument &acdc) {	// check of plausability failed:
					logfile.addmessage(logfile_class::logentrytype::error, acdc.what());
					logfile.addmessage(logfile_class::logentrytype::info, "We finished message2action.");
					std::cout << "Programm is finished." << std::endl;
					return 1;
				}
				catch (const std::out_of_range &acdc) {
				}
		} else {
			logfile.addmessage(logfile_class::logentrytype::error, "Ini-file could not be read correctly!");
			logfile.addmessage(logfile_class::logentrytype::info, "We finished message2action.");
			std::cout << "Programm is finished." << std::endl;
			return 1; 	// we have to stop program here
		}
		
		std::cout << "This is configuration of program:" << std::endl;
		m2a_ini_file.print_ini_file_content();
		valid_argument_flag = true;
	} // end of parameter config

	if (false == valid_argument_flag) std::cout << "Parameter is not allowed!" << std::endl;
	
} else {	// either no parameter or more than 1 was given to the program:
	std::cout << "No parameters given, these parameters (casesensitive) are possible:" << std::endl;
	std::cout << "? : print a little help on screen." << std::endl;
	std::cout << "daemon : program will run in daemon-mode (nothing will be printed on screen)" << std::endl;
	std::cout << "config : print content of ini-file and do checks on content of ini-file message2action.ini" << std::endl;
	std::cout << "start : starts the processing of all inbound messages (as specified in ini-file)" << std::endl;
	std::cout << "stop : another running instance of this program will be stopped." << std::endl;
}

logfile.addmessage(logfile_class::logentrytype::info, "We finished message2action.");
std::cout << "Programm is finished." << std::endl;
return 0;
}
