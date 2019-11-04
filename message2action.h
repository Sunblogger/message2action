//************************************************************************************************************************************************************
// Includefile for message2action and message2actiongui (m2agui), version : 0.37
// by Fuseless-Project
//
// This includefile declares some constants which are used globaly like returncodes of functions and lenghts of variables
// This program is free software. Feel free to redistribute it and/or modify it under the terms of the GNU General Public License as published by the 
// Free Software Foundation. This is valid for version 2 of the license or (at your option) any later version.
// This program was setup for my own usages but I'm sure that minimum 1 other user will find it as useful for his own purposes. I've set it up 
// WITHOUT ANY WARRANTY. See the GNU General Public License for more details: http://www.gnu.org/licenses/gpl.txt 
//************************************************************************************************************************************************************

#ifndef MESSAGE2ACTION_H
#define MESSAGE2ACTION_H

// following section defines global variables

#define MAX_CONFIG_FILE_SIZE  8192 // filesize of configfile must not exceed this amount of bytes
#define MAX_CHARS_PER_LINE_CONFIGFILE  200 // maximum 200 charackters per line in configfile; this value is also used for maximum length of values in ini-file
#define MAX_CHARS_PER_LINE_LOGFILE 300 // maximum 200 charackters per line in logfile
#define MAX_NUMBER_OF_PARAMETERS_CONFIGFILE 200 // maximum 200 values in configfile
#define MAX_CHARS_FOR_PATH 150				// maximum # of chars for a path in inifile
#define MAX_CHARS_FOR_FILENAME 100			// maximum # of chars for a filename
#define MAX_CHARS_FOR_COMMAND 80			// maximum # of chars for a command in a messages
#define MESSAGE_TYPE_SMS 0					// the type of message is SMS
#define MESSAGE_TYPE_MAIL 1					// the type of message is mail
#define MESSAGE_TYPE_TELEGRAM 2				// the type of message is telegram
#define MAX_CHARS_FOR_SUBJECT_IN_MAIL 60 	// the maximum # of chars we want to have in a subject of a mail
#define MAX_SIZE_HELP_TEXT 1024				// the maximum # of chars for the help-text
#define MAX_SIZE_CONFIG_TEXT 4096			// the maximum # of chars for the config-text

#define FALSE 0
#define TRUE 1

// define a list of errorcodes which can be used in functions:
#define NOERROR 0
#define CONFIG_FILE_READ_ERROR 1    				// something wrong with reading the config-file
#define CONFIG_FILESIZE_ERROR 2    					// config-file is too large
#define CONFIG_FILE_MAC_ERROR 3    					// MAC-adress in configfile is not valid for example 'ZE'
#define CONFIG_FILE_MAX_LOGFILE_ERROR 4   			// value for maxsize of logfile is invalid
#define CONFIG_FILE_WAIT_TIME_NEXT_MESSAGE_ERROR 5 	// value for wait-time for next message is
#define CONFIG_FILE_CELL_PHONE_STRING_ERROR 6		// something wrong with cell-phone number: does not contain '0' to '9'
#define CONFIG_FILE_CELL_PHONE_FOUND_ERROR 7		// we do not have any valid cell-phone in ini-file
#define CONFIG_FILE_PATH_LOGFILE_ERROR 8			// path to logfile is not correct
#define CONFIG_FILE_IP_OR_HOST_MISSING_ERROR 9		// behind MAC-address no ip-address or hostname specified
#define CONFIG_FILE_PATH_INCOMING_SMS_ERROR 10		// the path for incoming SMS is not valid
#define CONFIG_FILE_PATH_OUTGOING_SMS_ERROR 11		// the path for outgoing SMS is not valid
#define CONFIG_FILE_PATH_PROCESSED_SMS_ERROR 12		// path for processed SMS is not valid
#define CONFIG_FILE_SMS_PID_FILE_ERROR 13			// file for saving PID for SMS is not valid
#define CONFIG_FILE_PATH_INCOMING_MAIL_ERROR 14		// the path for incoming mail is not valid
#define CONFIG_FILE_PATH_OUTGOING_MAIL_ERROR 15		// the path for outgoing mail is not valid
#define CONFIG_FILE_PATH_PROCESSED_MAIL_ERROR 16	// path for processed SMS is not valid
#define CONFIG_FILE_MAIL_PID_FILE_ERROR 17			// file for saving PID for mail is not valid
#define CONFIG_FILE_CHECK_ONLINE_WAIT_TIME_ERROR 18	// the value for the waittime after sending WOL is not valid
#define CONFIG_FILE_PATH_INCOMING_TELEGRAM_ERROR 20 // path for incoming telegram is not valid
#define CONFIG_FILE_PATH_OUTGOING_TELEGRAM_ERROR 21 // path for outgoing telegram is not valid
#define CONFIG_FILE_PATH_PROCESSED_TELEGRAM_ERROR 22	// path for processed telegram is not valid
#define CONFIG_FILE_GARBAGE_COLLECTION_ERROR 23		// value for number of days for garbage collection is not valid
#define CONFIG_FILE_WRITE_ERROR 24					// config file could not be written with default values
#define CONFIG_FILE_INITIALIZED_ERROR 25			// config file had to be initialized and was written with default values

#define EXECUTE_COMMAND_NO_COMMAND_ERROR	1		// the command we should execute is not defined in ini-file (=empty)
#define EXECUTE_COMMAND_SYSTEM_ERROR 		2		// the system-call of the command to be executed failed
#define EXECUTE_COMMAND_WOL_ERROR			3		// the WOL-command could not be send out
 
#define KEYBOARD_THREAD_ERROR 20					// the thread could not be started to listen to keyboard-hits 

#define SENDWOL_SOCKET_ERROR 1
#define SENDWOL_SEND_ERROR 2

#define LOGFILE_OPEN_ERROR 1		// something wrong with opening the logfile
#define LOGFILE_WRITE_ERROR 2		// could not write to logfile
#define LOGFILE_TEMP_ERROR 3		// could not open to tempfile

#define SMS_SENDER_VALID 0			// The # for the cell-phone we want to send SMS to is valid
#define SMS_SENDER_NOTVALID 1		// The # for the cell-phone we want to send SMS to is invalid

#define MESSAGE_CREATE_OK 0			// the textfile could be created
#define MESSAGE_CREATE_FAIL 1 		// the textfile could not be created

#define CELL_PHONE_FOUND 0 			// we found a valid cell-phone-# in config-file
#define CELL_PHONE_NOT_FOUND 1		// we did not find a valid cell-phone-# in config-file

#define MAIL_ADDRESS_FOUND 0 		// we found a valid mailaddress in config-file
#define MAIL_ADDRESS_NOT_FOUND 1	// we did not find a mailaddress in config-file

#define TELEGRAM_CHATID_FOUND 0		// we have found a chat-id
#define TELEGRAM_CHATID_NOT_FOUND 1	// we did not find a chat-id

#define MAIL_SENDER_VALID 0			// The mailaddress we received a mail is valid
#define MAIL_SENDER_NOTVALID 1		// The mailaddress we received a mail is not valid

#define TELEGRAM_CHATID_VALID 0		// The chat-id in the telegram message we received is valid
#define TELEGRAM_CHATID_NOTVALID 1	// The chat-id in the telegram message we received is not valid

#define TELEGRAM_SENDER_VALID 0		// the sender of the telegram is valid
#define TELEGRAM_SENDER_NOTVALID 0	// the sender of the telegram is not valid

#define MESSAGE_FILE_ERROR 99		// the file which contains an error
#define PING_HOST_ONLINE 0 					// ping returns host is online
#define PING_UNKNOWN_OR_OFFLINE_HOST 1		// ping returns that host is unknown
#define PING_PIPE_ERROR 3					// something wrong with the pipe
#define SETUP_HELP_TEXT_TOO_LONG 1	// the text for the help message is too long
#define SETUP_CONFIG_TEXT_TOO_LONG 1	// the text for the config message is too long

#define MAX_CHARS_FOR_CELLPHONE 18			// maximum 18 chars for cell-phone-#
#define MAX_CHARS_FOR_MAILADDRESS 50		// maximum 50 chars for mailadress
#define MAX_CHARS_FOR_TELEGRAM_CHATID 12	// maximum 12 chars for chat-ID for Telegram
#define MAX_CHARS_FOR_MAC_ADDRESS 19		// maximum 12 chars for MAC-address: 6 x 3 chars + \0
#define MAX_CHARS_FOR_IP_ADDRESS_V4 16		// maximum 12 chars for IP V4 address: 3 x 4 + 3 + \0
#define MAX_CHARS_FOR_HOSTNAME 50			// maximum 50 chars for hostname

#define LOG_ERROR 0      	// do we have an error to log or do we have an information to log
#define LOG_INFO 1			// we log an information only
#define LOG_WARNING 2		// we log a warning
#define LOGGING_FALSE 0  	// logging is  disabled
#define LOGGING_TRUE 1		// logging is enabled


#endif	// define MESSAGE2ACTION_H
