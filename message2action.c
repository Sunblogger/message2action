//************************************************************************************************************************************************************
// message2action, version : see variable version_of_program
// by Fuseless-Project
//
// This program is free software. Feel free to redistribute it and/or modify it under the terms of the GNU General Public License as published by the 
// Free Software Foundation. This is valid for version 2 of the license or (at your option) any later version.
// This program was setup for my own usages but I'm sure that minimum 1 other user will find it as useful for his own purposes. I've set it up 
// WITHOUT ANY WARRANTY. See the GNU General Public License for more details: http://www.gnu.org/licenses/gpl.txt 
//************************************************************************************************************************************************************

#define DEVSTAGE // we define here that we are in developping stage and not in production stage. In order to switch to production stage comment this out

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <termios.h>
#include <pthread.h>
#include <linux/reboot.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/reboot.h>
#include <sys/select.h>
#include "message2action.h"

const char version_of_program[] = "message2action version 0.37\n";

const char config_filename[] = "message2action.ini";

// command in message which tells Raspberry to reboot:
char reboot_pi_command[MAX_CHARS_FOR_COMMAND]; 
const char reboot_pi_command_string[] = "reboot_pi_command";  // this is the string in ini-file we are searching for in order to find a specific command

// command in message which tells Raspberry to shutdown:
char shutdown_pi_command[MAX_CHARS_FOR_COMMAND]; 
const char shutdown_pi_command_string[] = "shutdown_pi_command";

// command in message which tells message2action to stop processing:
char message2action_stop_command[MAX_CHARS_FOR_COMMAND]; 
const char message2action_stop_command_string[] = "message2action_stop_command";

// command in message which tells message2action to return a help:
char message2action_help_command[MAX_CHARS_FOR_COMMAND]; 
const char message2action_help_command_string[] = "message2action_help_command";

// command in message which tells message2action to return a help, here: '?' :
char message2action_questionmark_command[MAX_CHARS_FOR_COMMAND]; 
const char message2action_questionmark_command_string[] = "message2action_questionmark_command";

// command in message which tells message2action to return configuration:
char message2action_config_command[MAX_CHARS_FOR_COMMAND]; 
const char message2action_config_command_string[] = "message2action_config_command";

// filename where the stop-command will be written to done by message2action itself to stop processing:
char message2action_stop_filename[MAX_CHARS_FOR_PATH]; 
const char message2action_stop_filename_string[] = "message2action_stop_filename";

// command which causes the program to send back the current status of the program and the Pi:
char status_pi_command[MAX_CHARS_FOR_COMMAND];
const char status_pi_command_string[] = "status_pi_command";

// path where message2action reads incoming SMS-text-messages:
char path_incoming_SMS[MAX_CHARS_FOR_PATH] = "\0";		// default is empty
const char path_incoming_SMS_string[] = "path_incoming_SMS";

// path where message2action reads incoming mails:
char path_incoming_mail[MAX_CHARS_FOR_PATH] = "\0";		// default is empty
const char path_incoming_mail_string[] = "path_incoming_mail";

// path where message2action reads incoming telegram-messages:
char path_incoming_telegram[MAX_CHARS_FOR_PATH] = "\0";		// default is empty
const char path_incoming_telegram_string[] = "path_incoming_telegram";

// path where message2action writes outgoing SMS-text-messages:
char path_outgoing_SMS[MAX_CHARS_FOR_PATH] = "\0";		// default is empty
const char path_outgoing_SMS_string[] = "path_outgoing_SMS";

// token for telegram bot: 
char telegram_bot_token[MAX_CHARS_FOR_PATH] = "\0"; 
const char telegram_bot_token_string[] = "telegram_bot_token";

// # of days for garbage collection. 0 = no garbage collection. Any other value specifies the number of days, how old files should be and not older
char garbage_collection_days[MAX_CHARS_FOR_COMMAND] = "\0";
const char garbage_collection_days_string[] = "garbage_collection_days";
unsigned long garbage_collection_days_long = 0;

// path where the read and processed SMS-text-messages will be moved after reading and processing
char path_processed_SMS[MAX_CHARS_FOR_PATH] = "\0";		// default is empty 
const char path_processed_SMS_string[] = "path_processed_SMS";

// path where the read and processed mails will be moved after reading and processing
char path_processed_mail[MAX_CHARS_FOR_PATH] = "\0";		// default is empty 
const char path_processed_mail_string[] = "path_processed_mail";

// path where the read and processed telegram-messages will be moved after reading and processing
char path_processed_telegram[MAX_CHARS_FOR_PATH] = "\0";		// default is empty 
const char path_processed_telegram_string[] = "path_processed_telegram";

// the file were the content of the mail will be saved which is send with ssmtp:
char path_outgoing_mail[MAX_CHARS_FOR_PATH] = "\0";		// default is empty 
const char path_outgoing_mail_string[] = "path_outgoing_mail";

// the file with the shell-skript which is called by systemd-timer to send the mail out:
char fullpath_systemd_shell_mail[MAX_CHARS_FOR_PATH] = "\0";	
// default is empty 
const char fullpath_systemd_shell_mail_string[] = "fullpath_systemd_shell_mail";

// the path where outgoing telegram messages are saved:
char path_outgoing_telegram[MAX_CHARS_FOR_PATH] = "\0";		// default is empty
const char path_outgoing_telegram_string[] = "path_outgoing_telegram";
 
// path where process-id of the current programm is written to:
char fullpath_smsfile_PID[MAX_CHARS_FOR_PATH] = "\0";		// default is empty 
const char fullpath_file_PID_string[] = "fullpath_smsfile_PID";

char fullpath_mailfile_PID[MAX_CHARS_FOR_PATH] = "\0";		// default is empty 
const char fullpath_mailfile_PID_string[] = "fullpath_mailfile_PID";

char fullpath_telegramfile_PID[MAX_CHARS_FOR_PATH] = "\0";		// default is empty 
const char fullpath_telegramfile_PID_string[] = "fullpath_telegramfile_PID";

//path where logfile of message2action is saved:
char file_logfile[MAX_CHARS_FOR_PATH]; 
const char file_logfile_string[] = "file_logfile";

// max size of the logfile expressed in kBytes:
char max_size_logfile[12] = "500"; 
const char max_size_logfile_string[] = "max_size_logfile"; 
long max_size_logfile_long = 500;	// if ini-file could not be read correctly we take 500 as default 

// time for waiting for next SMS specified in seconds:
char wait_time_next_sms[6]; // greatest value is 99999 seconds
char wait_time_next_sms_string[] = "wait_time_next_sms";
unsigned long wait_time_next_sms_long;

// time for waiting for next mail specified in seconds:
char wait_time_next_mail[6]; // greatest value is 99999 seconds
const char wait_time_next_mail_string[] = "wait_time_next_mail";
unsigned long wait_time_next_mail_long;

// time for waiting for next telegram specified in seconds:
char wait_time_next_telegram[6]; // greatest value is 99999 seconds
const char wait_time_next_telegram_string[] = "wait_time_next_telegram";
unsigned long wait_time_next_telegram_long;

// suffix following WOL00 ... WOL09 in order to check for online status:
char online_status_command_suffix[20] = "";
const char online_status_command_sufffix_string[] = "online_status_command_suffix";

// wait time in seconds after sending out WOL to a device: after this wait time the devices will be checked if it online:
char check_online_status_waittime[3] = "30";	// default is 30 seconds, values are alowed 00 to 99
const char check_online_status_waittime_string[] = "check_online_status_waittime";
unsigned long check_online_status_waittime_long = 30;	// default is 30 seconds

// 10 cell-phone-# from which we can send SMS to our Pi:
char cell_phone_00[20] = "\0"; 
char cell_phone_01[20] = "\0"; 
char cell_phone_02[20] = "\0"; 
char cell_phone_03[20] = "\0"; 
char cell_phone_04[20] = "\0"; 
char cell_phone_05[20] = "\0"; 
char cell_phone_06[20] = "\0"; 
char cell_phone_07[20] = "\0"; 
char cell_phone_08[20] = "\0"; 
char cell_phone_09[20] = "\0"; 

// 10 emailadresses which we can send mails to our Pi:
char mail_address_00[40] = "\0"; 
char mail_address_01[40] = "\0"; 
char mail_address_02[40] = "\0"; 
char mail_address_03[40] = "\0"; 
char mail_address_04[40] = "\0"; 
char mail_address_05[40] = "\0"; 
char mail_address_06[40] = "\0"; 
char mail_address_07[40] = "\0"; 
char mail_address_08[40] = "\0"; 
char mail_address_09[40] = "\0"; 

// 10 chat-ids to which we can send messages with telegram from our Pi:
char telegram_chat_id_00[40] = "\0"; 
char telegram_chat_id_01[40] = "\0"; 
char telegram_chat_id_02[40] = "\0"; 
char telegram_chat_id_03[40] = "\0"; 
char telegram_chat_id_04[40] = "\0"; 
char telegram_chat_id_05[40] = "\0"; 
char telegram_chat_id_06[40] = "\0"; 
char telegram_chat_id_07[40] = "\0"; 
char telegram_chat_id_08[40] = "\0"; 
char telegram_chat_id_09[40] = "\0"; 


// 10 commands which can be send to raspberry
char messsagecommand_00[80] = "\0";
char messsagecommand_01[80] = "\0";
char messsagecommand_02[80] = "\0";
char messsagecommand_03[80] = "\0";
char messsagecommand_04[80] = "\0";
char messsagecommand_05[80] = "\0";
char messsagecommand_06[80] = "\0";
char messsagecommand_07[80] = "\0";
char messsagecommand_08[80] = "\0";
char messsagecommand_09[80] = "\0";

char default_MAC_address[] = "00:00:00:00:00:00";
// 10 MAC-adresses for sending WOL-broadcast to: 6 times 2 charackters + 6 times a ':' + '\0' : 
char WOL_MAC_00[100] = "00:00:00:00:00:00";
char WOL_MAC_01[100] = "00:00:00:00:00:00";
char WOL_MAC_02[100] = "00:00:00:00:00:00";
char WOL_MAC_03[100] = "00:00:00:00:00:00";
char WOL_MAC_04[100] = "00:00:00:00:00:00";
char WOL_MAC_05[100] = "00:00:00:00:00:00";
char WOL_MAC_06[100] = "00:00:00:00:00:00";
char WOL_MAC_07[100] = "00:00:00:00:00:00";
char WOL_MAC_08[100] = "00:00:00:00:00:00";
char WOL_MAC_09[100] = "00:00:00:00:00:00";

// 10 MAC-adresses in hex-format without any delimeters, 6 times 1 chars 
char WOL_MAC_00_hex[6];
char WOL_MAC_01_hex[6];
char WOL_MAC_02_hex[6];
char WOL_MAC_03_hex[6];
char WOL_MAC_04_hex[6];
char WOL_MAC_05_hex[6];
char WOL_MAC_06_hex[6];
char WOL_MAC_07_hex[6];
char WOL_MAC_08_hex[6];
char WOL_MAC_09_hex[6];

// 10 IP-adresses in V4-format. 3 times 3 digits + '.' + 3 digits + \0
char ip_address_00[16] = "\0"; 
char ip_address_01[16] = "\0"; 
char ip_address_02[16] = "\0"; 
char ip_address_03[16] = "\0"; 
char ip_address_04[16] = "\0"; 
char ip_address_05[16] = "\0"; 
char ip_address_06[16] = "\0"; 
char ip_address_07[16] = "\0"; 
char ip_address_08[16] = "\0"; 
char ip_address_09[16] = "\0"; 

// 10 hostnames, each not longer than 75 chars:
char hostname_00[75] = "\0";
char hostname_01[75] = "\0";
char hostname_02[75] = "\0";
char hostname_03[75] = "\0";
char hostname_04[75] = "\0";
char hostname_05[75] = "\0";
char hostname_06[75] = "\0";
char hostname_07[75] = "\0";
char hostname_08[75] = "\0";
char hostname_09[75] = "\0";

// array of pointers to chars for storing all strings to search in configfile:
const char *configparameters[] = { "reboot_pi_command", "shutdown_pi_command", "status_pi_command", "message2action_help_command", "message2action_questionmark_command", "message2action_config_command",
							"message2action_stop_command", "message2action_stop_filename", 
							"path_incoming_SMS", "path_outgoing_SMS", "path_processed_SMS", "wait_time_next_sms", 
							"path_incoming_mail", "path_outgoing_mail", "path_processed_mail", "wait_time_next_mail", "fullpath_systemd_shell_mail", 							 
							"telegram_bot_token", "garbage_collection_days",
							"path_incoming_telegram", "path_outgoing_telegram", "path_processed_telegram", "wait_time_next_telegram", 
							"file_logfile", "max_size_logfile", "fullpath_smsfile_PID", "fullpath_mailfile_PID","fullpath_telegramfile_PID", "online_status_command_suffix", "check_online_status_waittime",
							"cell_phone00", "cell_phone01", "cell_phone02", "cell_phone03", "cell_phone04", "cell_phone05", "cell_phone06", "cell_phone07", "cell_phone08", "cell_phone09", 
							"mail_address_00", "mail_address_01", "mail_address_02", "mail_address_03", "mail_address_04", "mail_address_05", "mail_address_06", "mail_address_07", "mail_address_08", "mail_address_09", 
							"telegram_chat_id_00", "telegram_chat_id_01", "telegram_chat_id_02", "telegram_chat_id_03", "telegram_chat_id_04", "telegram_chat_id_05", "telegram_chat_id_06", "telegram_chat_id_07", "telegram_chat_id_08", "telegram_chat_id_09", 
							"CMD00", "CMD01", "CMD02", "CMD03", "CMD04", "CMD05", "CMD06", "CMD07", "CMD08", "CMD09", 
							"WOL00", "WOL01", "WOL02", "WOL03", "WOL04", "WOL05", "WOL06", "WOL07", "WOL08", "WOL09", 
							'\0'}; // NULL-pointer marks end of array, do not delete this entry
							
// array of pointers for storing the values we have in ini-file:
char *configvariables[] = { reboot_pi_command, shutdown_pi_command, status_pi_command, message2action_help_command, message2action_questionmark_command, message2action_config_command,
							message2action_stop_command, message2action_stop_filename, 
							path_incoming_SMS, path_outgoing_SMS, path_processed_SMS, wait_time_next_sms,
							path_incoming_mail, path_outgoing_mail, path_processed_mail, wait_time_next_mail, fullpath_systemd_shell_mail,
							telegram_bot_token, garbage_collection_days,
							path_incoming_telegram, path_outgoing_telegram, path_processed_telegram, wait_time_next_telegram,
							file_logfile, max_size_logfile, fullpath_smsfile_PID, fullpath_mailfile_PID, fullpath_telegramfile_PID, online_status_command_suffix, check_online_status_waittime,
							cell_phone_00, cell_phone_01, cell_phone_02, cell_phone_03, cell_phone_04, cell_phone_05, cell_phone_06, cell_phone_07, cell_phone_08, cell_phone_09, 
							mail_address_00, mail_address_01, mail_address_02, mail_address_03, mail_address_04, mail_address_05, mail_address_06, mail_address_07, mail_address_08, mail_address_09, 
							telegram_chat_id_00, telegram_chat_id_01, telegram_chat_id_02, telegram_chat_id_03, telegram_chat_id_04, telegram_chat_id_05, telegram_chat_id_06, telegram_chat_id_07, telegram_chat_id_08, telegram_chat_id_09,  
							messsagecommand_00, messsagecommand_01, messsagecommand_02, messsagecommand_03, messsagecommand_04, messsagecommand_05, messsagecommand_06, messsagecommand_07, messsagecommand_08, messsagecommand_09, 
							WOL_MAC_00, WOL_MAC_01, WOL_MAC_02, WOL_MAC_03, WOL_MAC_04, WOL_MAC_05, WOL_MAC_06, WOL_MAC_07, WOL_MAC_08, WOL_MAC_09, 
							ip_address_00, ip_address_01, ip_address_02, ip_address_03, ip_address_04, ip_address_05, ip_address_06, ip_address_07, ip_address_08, ip_address_09,
							hostname_00, hostname_01, hostname_02, hostname_03, hostname_04, hostname_05, hostname_06, hostname_07, hostname_08, hostname_09,  
							'\0'}; 	// NULL-pointer marks end of array, do not delete this entry

// enumeration of the variables we have in config-file:
enum configcount { 	reboot_pi_command_count, shutdown_pi_command_count, status_pi_command_count, message2action_help_command_count, message2action_questionmark_command_count, message2action_config_command_count,
						message2action_stop_command_count, message2action_stop_filename_count, 
						path_incoming_SMS_count, path_outgoing_SMS_count, path_processed_SMS_count, wait_time_next_sms_count, 
						path_incoming_mail_count, path_outgoing_mail_count, path_processed_mail_count, wait_time_next_mail_count, fullpath_systemd_shell_mail_count, 
						telegram_bot_token_count, garbage_collection_days_count,
						path_incoming_telegram_count, path_outgoing_telegram_count, path_processed_telegram_count, wait_time_next_telegram_count, 
						file_logfile_count, max_lines_logfile_count, smsfile_PID_count, mailfile_PID_count, telegramfile_PID_count, online_status_command_suffix_count, check_online_status_waittime_count,
						cell_phone00_count, cell_phone01_count, cell_phone02_count, cell_phone03_count, cell_phone04_count, cell_phone05_count, cell_phone06_count, cell_phone07_count, cell_phone08_count, cell_phone09_count, 
						mail_address_00_count, mail_address_01_count, mail_address_02_count, mail_address_03_count, mail_address_04_count, mail_address_05_count, mail_address_06_count, mail_address_07_count, mail_address_08_count, mail_address_09_count,
						telegram_chat_id_00_count, telegram_chat_id_01_count, telegram_chat_id_02_count, telegram_chat_id_03_count, telegram_chat_id_04_count, telegram_chat_id_05_count, telegram_chat_id_06_count, telegram_chat_id_07_count, telegram_chat_id_08_count, telegram_chat_id_09_count, 
						messsagecommand_00_count, messsagecommand_01_count, messsagecommand_02_count, messsagecommand_03_count, messsagecommand_04_count, messsagecommand_05_count, messsagecommand_06_count, messsagecommand_07_count, messsagecommand_08_count, messsagecommand_09_count,
						WOL_MAC_00_count, WOL_MAC_01_count, WOL_MAC_02_count, WOL_MAC_03_count, WOL_MAC_04_count, WOL_MAC_05_count, WOL_MAC_06_count, WOL_MAC_07_count, WOL_MAC_08_count, WOL_MAC_09_count,  
						ip_address_00_count, ip_address_01_count, ip_address_02_count, ip_address_03_count, ip_address_04_count, ip_address_05_count, ip_address_06_count, ip_address_07_count, ip_address_08_count, ip_address_09_count, 
						hostname_00_count, hostname_01_count, hostname_02_count, hostname_03_count, hostname_04_count, hostname_05_count, hostname_06_count, hostname_07_count, hostname_08_count, hostname_09_count 
					}; 

int configparametercount = 0;	// # of configparameters we have to search for

const char *cmdline_parameters[] = { 		// list of commandlineparameter which can be used
		"config", "?", "stopsignal", "stopfile", "ini=",
		"display=y", "display=n", "log=y", "log=n", "daemon=y", "daemon=n",
		"out=sms", "out=mail", "out=telegram", 
		"in=sms", "in=mail", "in=telegram", 
		'\0'	// marks end of array
};

enum cmdline_parameter_count  {
		config_counter, questionmark_counter, stopsignal_counter, stopfile_counter, inifile_counter,
		display_yes_counter, display_no_counter, log_yes_counter, log_no_counter, daemon_yes_counter, daemon_no_counter,
		outbound_sms_counter, outbound_mail_counter, outbound_telegram_counter,		// we count first for outgoing parameters because we search for them first
		inbound_sms_counter, inbound_mail_counter, inbound_telegram_counter, 
};

int reboot_piflag = FALSE; // flags if we have to reboot the Raspberry Pi, default is no
int shutdown_piflag = FALSE; // flags if we have to shutdown the Raspberry Pi, default is no

// global variables because we can have warnings and erros is any function. The variables are only changed in function write_logfile
unsigned int warningcounter = 0; 		// counts the # of warnings we had
unsigned int errorcounter = 0; 		// counts the # of errors we had

// global variable for all threads and sub-processes:
volatile sig_atomic_t processingflag = TRUE; // if TRUE: yes, we are still processing, =FALSE, no, we stop processing

// flags can have only value = 0 (not set) or 1 (set)
// the following flags sign if we have found a specific commandlline-parameter. For example: if cmdline_logflag is TRUE than we know that we have to log in loggingfile
int cmdline_startflag = 0;
int cmdline_logflag = 0;
int cmdline_stopflag = 0;
int cmdline_helpflag = 0;
int cmdline_displayflag = 0;
int cmdline_configflag = 0;
int cmdline_signalflag = 0;
int cmdline_flag = 0;
int logging_flag = 0; 
int display_flag = 0;
int daemon_flag = 0;
int inbound_flag = 0; 		// flags if we have defined the inputchannel like sms, mail
int inbound_sms_flag = 0;
int inbound_mail_flag = 0;
int inbound_telegram_flag = 0;
int outbound_sms_flag = 0;
int outbound_mail_flag = 0;
int outbound_telegram_flag = 0;
int valid_parameter_found_flag = 0;
int stopsignal_flag = 0;
int stopfile_flag = 0;

#define LOGGING_FALSE 0  	// logging is  disabled
#define LOGGING_TRUE 1		// logging is enabled
#define DISPLAY_FALSE 0		// displaying is disabled, this is quiete mode
#define DISPLAY_TRUE 1		// displaying is enabled
#define DAEMON_FALSE 0		// daemon mode is disabled, this is quiete mode
#define DAEMON_TRUE 1		// daemon mode is enabled
#define LOG_ERROR 0      	// do we have an error to log or do we have an information to log
#define LOG_INFO 1			// we log an information only
#define LOG_WARNING 2		// we log a warning
#define STOPSIGNAL_TRUE 1	// we have to stop program by sending a signal
#define STOPFILE_TRUE 1		// we have to stop program by creating a file which contains stop-message


//*****************************************************************************************************************
// upcase_string : each char in string will be in upper case ('a' to 'A', 'g' to 'G')
// input: string to modify and to write result to
// returncodes: 0 = no error
// 				!=0  = any error
//*****************************************************************************************************************

void upcase_string(char* input) {
	
// char tempstring[MAX_CHARS_PER_LINE_CONFIGFILE] = ""; 	
unsigned int tempint;
unsigned int stringlength;

// strncpy(tempstring, input, MAX_CHARS_PER_LINE_CONFIGFILE - 1); 	// we save input in temp
stringlength = strlen(input);

for (tempint = 0; tempint < stringlength; tempint++) {
	input[tempint] = toupper(input[tempint]);	
}	
	
}	// end of upcase_string ***************************************************************************************

//*****************************************************************************************************************
// write_logfile: writes content to logfile. Date + time is in the beginning of each line. If the logfile becomes too
// large, the file will be divided by 2.
// input: string to write to logfile
// returncodes: 0 = no error
// 				1 = any error
//*****************************************************************************************************************
int write_logfile(const char* logcontent, const int message_flag) {
	
time_t timestamp_time_logfile; 	
struct tm *timestamp_tm_logfile;
char date_time[20] = "01.01.2000 01:01:01";  // string to save date + time, this is default-value
FILE *logfile;
FILE *tempfile;
unsigned linecounter; 
unsigned counter; 
char logfileline[MAX_CHARS_PER_LINE_LOGFILE]; 
struct stat fileattributes;
char tempfilename[100 + 4] = "";  

if (message_flag == LOG_WARNING) warningcounter++;  	// if we have a warning we have to increment the counter
if (message_flag == LOG_ERROR) errorcounter++;			// if we have an error we have to increment the counter

if ((logging_flag == LOGGING_TRUE) || ((message_flag == LOG_ERROR) || (message_flag == LOG_WARNING))) { // we log only when logging is enabled or if we have an error

	time(&timestamp_time_logfile);
	timestamp_tm_logfile = localtime( &timestamp_time_logfile);
	strftime(date_time, 20, "%d.%m.%Y %H:%M:%S", timestamp_tm_logfile);
	logfile = fopen(file_logfile, "a+");

	if (logfile != 0) { // logfile could  be opened
		fprintf(logfile, "%s : %s", date_time, logcontent);  //we add to the logfile one new line
		// now we check if we have reached the maximum # of lines in the logfile
		stat(file_logfile, &fileattributes);
		if (fileattributes.st_size > max_size_logfile_long*1024) {  // file is to big, we have to divide it by 2:
			fprintf(logfile, "%s : %s\n", date_time, "Info: Logfile was truncated due to limit of size.\n");  //we add to the logfile one new line
			rewind(logfile);
			for (linecounter = 0;  fgets(logfileline, MAX_CHARS_PER_LINE_LOGFILE, logfile) != 0; linecounter++) ;
			rewind(logfile);
			linecounter = linecounter / 2; 
			for (counter = 0;  counter < linecounter; counter++) fgets(logfileline, MAX_CHARS_PER_LINE_LOGFILE, logfile);  // we step to the position from which we want to read from
			strcpy(tempfilename, file_logfile);
			strcat(tempfilename, ".tmp");
			tempfile = fopen(tempfilename, "w+");
			if (tempfile != 0) { // now we copy from logfile to tempfile the newest lines:
				while ( fgets(logfileline, MAX_CHARS_PER_LINE_LOGFILE, logfile) != 0) {
					fputs(logfileline, tempfile);
				}

				fclose(tempfile);	
				remove(file_logfile);	
				rename(tempfilename, file_logfile);
				if (message_flag == LOG_ERROR) errorcounter++;		// we increment counter for errors
				if (message_flag == LOG_WARNING) warningcounter++;	// we increment counter for warnings
				
			} else { // tempfile could not be opened
				printf("Error: Temporary file for truncating logfile could not be opened!\n"); 
				return LOGFILE_TEMP_ERROR; 
			}
		
		} // end of if logfile is too big
		fclose(logfile);
		
	} else {
		printf("Error: Logfile %s could not be opened! Message: %s\n", file_logfile, logcontent);
		return LOGFILE_OPEN_ERROR;
	}
} 

if ((display_flag == DISPLAY_TRUE) || (message_flag == LOG_ERROR)) 
	printf("\r%s", logcontent) ;     // we print error-message on the screen in any cases but we print info-message only when global display-flag is true	

return 0;
	
} // end of write_logfile  ***************************************************************


//*****************************************************************************************************************
// gethexvaluefromstring: converts a string to its hex-value. For example string "A9" will be converted to 0xa9. 
// input: 	buffer : the source where to read from, it is a string, for example "A9"
// 			hexvalue: the value in hex-form, for example 0xa9
// returncodes: 0 = no error
// 				1, 2 = any error
//*****************************************************************************************************************

unsigned gethexvaluefromstring(char *buffer, char *hexvalue) {
	
	
	unsigned temp = 0; 
	
	// printf("Eingabewert #1: %c\n", buffer[0]);
	switch (buffer[0]) {
			case '0': *hexvalue = 0; break; 
			case '1': *hexvalue = 16; break; 
			case '2': *hexvalue = 32; break; 
			case '3': *hexvalue = 48; break; 
			case '4': *hexvalue = 64; break; 
			case '5': *hexvalue = 80; break; 
			case '6': *hexvalue = 96; break; 
			case '7': *hexvalue = 112; break; 
			case '8': *hexvalue = 128; break; 
			case '9': *hexvalue = 144; break; 
			case 'A': *hexvalue = 160; break; 
			case 'B': *hexvalue = 176; break; 
			case 'C': *hexvalue = 192; break; 
			case 'D': *hexvalue = 208; break; 
			case 'E': *hexvalue = 224; break; 
			case 'F': *hexvalue = 240; break; 

			default: temp = 1; break;   // we don't have a valid charackter, the value != 0 is our flag for this
		}	
					
// printf("Eingabewert #2: %c\n", buffer[1]);
					
	switch (buffer[1]) {
			case '0': *hexvalue += 0; break; 
			case '1': *hexvalue += 1; break; 
			case '2': *hexvalue += 2; break; 
			case '3': *hexvalue += 3; break; 
			case '4': *hexvalue += 4; break; 
			case '5': *hexvalue += 5; break; 
			case '6': *hexvalue += 6; break; 
			case '7': *hexvalue += 7; break; 
			case '8': *hexvalue += 8; break; 
			case '9': *hexvalue += 9; break; 
			case 'A': *hexvalue += 10; break; 
			case 'B': *hexvalue += 11; break; 
			case 'C': *hexvalue += 12; break; 
			case 'D': *hexvalue += 13; break; 
			case 'E': *hexvalue += 14; break; 
			case 'F': *hexvalue += 15; break; 
			
			default: temp = 2; break; // we don't have a valid charackter, the value != 0 is our flag for this
		}	
//	printf("temphex: %i\n", temphex);
	return (temp); 
}

//*****************************************************************************************************************
// read_config_file: reads content of message2action.ini and fills global variables from ini-file
// input: name of config-file for message2action
// returncodes: 0 = no error
// 				!= 0 = any error
//*****************************************************************************************************************
unsigned int read_config_file(const char* configfilename) {

DIR *tempdirectory; 	// the directory which will be checked for plausibility only		
FILE *configfilep;  // file-handler which contains the config-file
FILE *logfile; 		// file-handler which contains the logfile

char configfileline[MAX_CHARS_PER_LINE_CONFIGFILE];
struct stat fileattributes;
long filepos = 0;
int tempint = 0;
int length = 0;
int counter = 0; 
unsigned int valid_cellphonenumber_flag = FALSE; 	// flags if we have found minimum 1 valid cell-phone in ini-file
char digitstring[20] = "";
char *fileposcommentmarker; // position of '#'
char *fileposNL;  // position of '\n'

char *fileposequalsign; // position of '=' in ini-file
char *configparameterpos; // position of variable in ini file. The variable can also be written with leading blanks in front
int valuelength = 0; // number of charackters the value has in ini-file
char *stop_ptr = 0;
char logentry[MAX_CHARS_PER_LINE_LOGFILE]; 		// one line for the logfile

configfilep = fopen(configfilename, "r");
if (configfilep != 0) { // opening of configfile was successfull
	stat(config_filename, &fileattributes); 
	if (fileattributes.st_size > MAX_CONFIG_FILE_SIZE) 
		{ 	// filesize of config-file is greater than specified, we stop here
			printf("Error: Filesize of configfile is too large: %ld bytes.\n", fileattributes.st_size);
						
			fclose(configfilep); // close file, it can not be processed
			return CONFIG_FILESIZE_ERROR;
		} else { // file is small enough, we start to extract all config-parameters:
		 	memset(configfileline, '\0', MAX_CHARS_PER_LINE_CONFIGFILE);
				
			while (configparameters[configparametercount++] != '\0'); // look for NULL-pointer at end of array in order to determine the # of parameters we have to search for. 
			configparametercount--; 								// configparametercount will be set with the number of configparameters we have: '\0' does not belong to the commands
			
			while ( fgets(configfileline, MAX_CHARS_PER_LINE_CONFIGFILE, configfilep) != 0) {
				fileposequalsign = strchr(configfileline, '=');
				fileposcommentmarker = strchr(configfileline, '#');
				fileposNL = strchr(configfileline, '\n');
				if (fileposcommentmarker != 0) { // if '#' is in the line then delete all signs between '#' and '\n' but leave '\n' in line
				memset(fileposcommentmarker, ' ', fileposNL - fileposcommentmarker);	
				} 
				
				for (tempint = 0; tempint < configparametercount ;  tempint++ ) {
					configparameterpos = strstr(configfileline, configparameters[tempint]);
					if ( configparameterpos != 0) { // if we find in current line the name of the variable, we copy the value as written in ini-file to our variable
							valuelength = fileposNL - fileposequalsign - 1;
								strncpy( configvariables[tempint], configparameterpos + strlen(configparameters[tempint]) + 1, valuelength );
								memset(configvariables[tempint] + valuelength, '\0', 1); // terminate string with a NULL
								if ( ((tempint >= reboot_pi_command_count) && (tempint <= message2action_stop_command_count)) || ((tempint >= mail_address_00_count) && (tempint <=mail_address_09_count )) || tempint == online_status_command_suffix_count)
									// we upcase only the first variables and the mailaddresses and the MAC-addresses. The other variables must not be modified because they are case sensitive
									upcase_string(configvariables[tempint]);
							
					} 
				} // end of for-loop
				filepos += strlen(configfileline);	// calculate how many chars we have to seek in file for next line in file
				fseek(configfilep, filepos,  SEEK_SET);
				memset(configfileline, '\0', MAX_CHARS_PER_LINE_CONFIGFILE);
				} // end of while-loop
				
			fclose(configfilep); // close file, we have all parameters from ini-file	
			
			// now we do some plausibility-checks concerning the parameters we have read from ini-file:
			// first we check if we have a valid value for the logfile:
			logfile = fopen(file_logfile, "a+");
			if (logfile != NULL) { // value for logfile is correct
				fclose(logfile);
			} else { // value for logfile is not correct
				printf("Error: logfile can not be opened for writing: %s!\n", file_logfile);
				return(CONFIG_FILE_PATH_LOGFILE_ERROR);
			} 
			
			// we check the MAC-adresses in ini-file:
			for (tempint = 0; tempint < 6; tempint++) {
				if (gethexvaluefromstring(&WOL_MAC_00[3*tempint], &WOL_MAC_00_hex[tempint]) != 0) { write_logfile("Error: MAC-address WOL00 in ini-file is not valid!\n", LOG_ERROR); return CONFIG_FILE_MAC_ERROR;}
				if (gethexvaluefromstring(&WOL_MAC_01[3*tempint], &WOL_MAC_01_hex[tempint]) != 0) { write_logfile("Error: MAC-address WOL01 in ini-file is not valid!\n", LOG_ERROR); return CONFIG_FILE_MAC_ERROR;}
				if (gethexvaluefromstring(&WOL_MAC_02[3*tempint], &WOL_MAC_02_hex[tempint]) != 0) { write_logfile("Error: MAC-address WOL02 in ini-file is not valid!\n", LOG_ERROR); return CONFIG_FILE_MAC_ERROR;}
				if (gethexvaluefromstring(&WOL_MAC_03[3*tempint], &WOL_MAC_03_hex[tempint]) != 0) { write_logfile("Error: MAC-address WOL03 in ini-file is not valid!\n", LOG_ERROR); return CONFIG_FILE_MAC_ERROR;}
				if (gethexvaluefromstring(&WOL_MAC_04[3*tempint], &WOL_MAC_04_hex[tempint]) != 0) { write_logfile("Error: MAC-address WOL04 in ini-file is not valid!\n", LOG_ERROR); return CONFIG_FILE_MAC_ERROR;}
				if (gethexvaluefromstring(&WOL_MAC_05[3*tempint], &WOL_MAC_05_hex[tempint]) != 0) { write_logfile("Error: MAC-address WOL05 in ini-file is not valid!\n", LOG_ERROR); return CONFIG_FILE_MAC_ERROR;}
				if (gethexvaluefromstring(&WOL_MAC_06[3*tempint], &WOL_MAC_06_hex[tempint]) != 0) { write_logfile("Error: MAC-address WOL06 in ini-file is not valid!\n", LOG_ERROR); return CONFIG_FILE_MAC_ERROR;}
				if (gethexvaluefromstring(&WOL_MAC_07[3*tempint], &WOL_MAC_07_hex[tempint]) != 0) { write_logfile("Error: MAC-address WOL07 in ini-file is not valid!\n", LOG_ERROR); return CONFIG_FILE_MAC_ERROR;}
				if (gethexvaluefromstring(&WOL_MAC_08[3*tempint], &WOL_MAC_08_hex[tempint]) != 0) { write_logfile("Error: MAC-address WOL08 in ini-file is not valid!\n", LOG_ERROR); return CONFIG_FILE_MAC_ERROR;}
				if (gethexvaluefromstring(&WOL_MAC_09[3*tempint], &WOL_MAC_09_hex[tempint]) != 0) { write_logfile("Error: MAC-address WOL09 in ini-file is not valid!\n", LOG_ERROR); return CONFIG_FILE_MAC_ERROR;}
			} // end of for-loop
			
			// we check now if we have an ip-address or a hostname behind the MAC-address:
			for (tempint = WOL_MAC_00_count; tempint < WOL_MAC_09_count; tempint++) {
				if (strcmp(configvariables[tempint], default_MAC_address) != 0) { 
				configparameterpos = strstr(configvariables[tempint], "host=");
				// printf("Inhalt: %s\n", configvariables[tempint]);
				if ( configparameterpos != 0 ) {  // we have a host-name behind MAC-address: 
					fileposequalsign = strchr(configvariables[tempint], '=');
					valuelength = strlen(configvariables[tempint]) - 23;
					strncpy(configvariables[tempint + hostname_00_count - WOL_MAC_00_count], configparameterpos + 5, valuelength);
					memset(configvariables[tempint] + 17, 0, 1);  // we terminate MAC-address with \0
				} else {  // check if we have an ip-address behind MAC-address:
					configparameterpos = strstr(configvariables[tempint], "ip=");
					if (configparameterpos != 0 ) {
						fileposequalsign = strchr(configvariables[tempint], '=');
						valuelength = strlen(configvariables[tempint]) - 21;
						strncpy(configvariables[tempint + ip_address_00_count - WOL_MAC_00_count], configparameterpos + 3, valuelength);
						memset(configvariables[tempint] + 17, 0, 1); // we terminate MAC-address with \0
					}
				}
				// now we check if we had either host or ip specified:
				if (configparameterpos == NULL) {
					sprintf(logentry, "Error: Neither ip-address nor hostname related to MAC-addresss #%d: %s!\n", tempint - WOL_MAC_00_count, configvariables[tempint]);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_IP_OR_HOST_MISSING_ERROR);		
				}
				
				} // end of if (strcmp(configvariables[tempint], default_MAC_address) != 0)
				
			}	// end of for-loop (tempint = WOL_MAC_00_count; tempint < WOL_MAC_09_count; tempint++)
			
			// we check receiving and sending cell-phone-#:
			// Note: on 32-bit-OS unsigned long can have maximum 10 digits but cellphone-# can have more than 10 digits. So we have to convert each digit separatly:
			for (tempint = 0; tempint < 10; tempint++) {
				length = strlen(configvariables[cell_phone00_count + tempint]); 
				if (length > 0) valid_cellphonenumber_flag = TRUE;
				strcpy(digitstring, configvariables[cell_phone00_count + tempint]); 
				for (counter = 0; counter < length; counter++) {
					if ( !(digitstring[counter] >= '0' && digitstring[counter] <= '9')) {
						printf("Error: Invalid sign \"%c\" in cell-phone number #%d in ini-file!\n", digitstring[counter], tempint + 1);
						return CONFIG_FILE_CELL_PHONE_STRING_ERROR;
					} 
				} // end of for-loop
			}  // end of for-loop
			if (valid_cellphonenumber_flag == FALSE) { 
				printf("Error: no valid cell-phone-number found in ini-file!\n");
				return CONFIG_FILE_CELL_PHONE_FOUND_ERROR;
			}		

			// we convert the filesize of logfile to integer:
			max_size_logfile_long = strtoul(max_size_logfile, &stop_ptr, 10);				
			if (*stop_ptr != 0) {
				printf("Error: %s is not valid for parameter \"max_size_logfile\" in inifile!\n", max_size_logfile );
				return CONFIG_FILE_MAX_LOGFILE_ERROR; 
			}

			// we have to convert the value for the wait-time for next SMS:	
			wait_time_next_sms_long = strtoul(wait_time_next_sms, &stop_ptr,10);
			if (*stop_ptr != 0) {
				printf("Error: Invalid value for parameter %s in inifile: %s!\n", wait_time_next_sms_string, wait_time_next_sms);
				return CONFIG_FILE_WAIT_TIME_NEXT_MESSAGE_ERROR; 
			}			
					
			// we have to convert the value for the wait-time for next mail:	
			wait_time_next_mail_long = strtoul(wait_time_next_mail, &stop_ptr,10);
			if (*stop_ptr != 0) {
				printf("Error: Invalid value for parameter %s in inifile: %s!\n", wait_time_next_mail_string, wait_time_next_mail);
				return CONFIG_FILE_WAIT_TIME_NEXT_MESSAGE_ERROR; 
			}			

			// we have to convert the value for the wait-time for next telegram:	
			wait_time_next_telegram_long = strtoul(wait_time_next_telegram, &stop_ptr,10);
			if (*stop_ptr != 0) {
				printf("Error: Invalid value for parameter %s in inifile: %s!\n", wait_time_next_telegram_string, wait_time_next_telegram);
				return CONFIG_FILE_WAIT_TIME_NEXT_MESSAGE_ERROR; 
			}
			
			// we have to convert the value for the number of days for garbage collection:	
			if (strlen(garbage_collection_days) == 0) {	// if we do not have a value at all then we set garbage_collection_days to 0 = no garbage collection at all
				garbage_collection_days_long = 0;
			} else {
				garbage_collection_days_long = strtoul(garbage_collection_days, &stop_ptr, 10);
				if (*stop_ptr != 0) {
					printf("Error: Invalid value for parameter %s in inifile: %s!\n", garbage_collection_days_string, garbage_collection_days);
					return CONFIG_FILE_GARBAGE_COLLECTION_ERROR; 
				}		
			}
			
			// we check the path for incoming SMS:
			if (strlen(path_incoming_SMS) > 0) { // there's a value given in ini-file
				if (path_incoming_SMS[ strlen(path_incoming_SMS) - 1] != '/') {
					sprintf(logentry, "Error: path for incoming SMS must have \'/\' at end: %s !\n", path_incoming_SMS);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_INCOMING_SMS_ERROR);
				}		
			tempdirectory = opendir(path_incoming_SMS);
			if (tempdirectory != 0) closedir(tempdirectory); 
				else {
					sprintf(logentry, "Error: path for incoming SMS is not valid: %s!\n", path_incoming_SMS);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_INCOMING_SMS_ERROR);		
				}
			} else {	// we have no value in ini-file:
				write_logfile("Warning: path for incoming SMS is not specified in ini-file!\n", LOG_WARNING);
			}
			
			// we check the path for outgoing SMS:
			if (strlen(path_outgoing_SMS) > 0) { // there's a value given in ini-file 
				if (path_outgoing_SMS[ strlen(path_outgoing_SMS) - 1] != '/') {
					sprintf(logentry, "Error: path for outgoing SMS must have \'/\' at end: %s !\n", path_outgoing_SMS);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_OUTGOING_SMS_ERROR);
				}
			tempdirectory = opendir(path_outgoing_SMS);
			if (tempdirectory != 0) closedir(tempdirectory); 
				else {	// directory does not exist:
					sprintf(logentry, "Error: path for outgoing SMS is not valid: %s!\n", path_outgoing_SMS);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_OUTGOING_SMS_ERROR);		
				}
			} else {	// we have no value in ini-file:
				write_logfile("Warning: path for outgoing SMS is not specified in ini-file!\n", LOG_WARNING);
			}			
			
			// we check the path for processed SMS:
			if (strlen(path_processed_SMS) > 0) { // there's a value given in ini-file
				if (path_processed_SMS[ strlen(path_processed_SMS) - 1] != '/') {
					sprintf(logentry, "Error: path for processed SMS must have \'/\' at end: %s !\n", path_processed_SMS);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_PROCESSED_SMS_ERROR);
				}				 
			tempdirectory = opendir(path_processed_SMS);
			if (tempdirectory != 0) closedir(tempdirectory); 
				else {	// directory does not exist:
					sprintf(logentry, "Error: path for processed SMS is not valid: %s!\n", path_processed_SMS);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_PROCESSED_SMS_ERROR);		
				}
			} else {	// we have no value in ini-file:
				write_logfile("Warning: path for processed SMS is not specified in ini-file!\n", LOG_WARNING);
			}			

			// we check file for saving process-ID for SMS:
			if (strlen(fullpath_file_PID_string) == 0) // we have no value in ini-file:
				write_logfile("Warning: file for saving PID for SMS is not specified in ini-file!\n", LOG_WARNING);

			// we check the path for incoming mail:
			if (strlen(path_incoming_mail) > 0) { // there's a value given in ini-file 
				if (path_incoming_mail[ strlen(path_incoming_mail) - 1] != '/') {
					sprintf(logentry, "Error: path for incoming mail must have \'/\' at end: %s !\n", path_incoming_mail);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_INCOMING_MAIL_ERROR);
				}
			tempdirectory = opendir(path_incoming_mail);
			if (tempdirectory != 0) closedir(tempdirectory); 
				else {
					sprintf(logentry, "Error: path for incoming mail is not valid: %s!\n", path_incoming_mail);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_INCOMING_MAIL_ERROR);		
				}
			} else {	// we have no value in ini-file:
				write_logfile("Warning: path for incoming mail is not specified in ini-file!\n", LOG_WARNING);
			}
			
			// we check the fullpath for outgoing mail:
			if (strlen(path_outgoing_mail) > 0) { // there's a value given in ini-file 
				if (path_outgoing_mail[ strlen(path_outgoing_mail) - 1] != '/') {
					sprintf(logentry, "Error: path for outgoing mail must have \'/\' at end: %s !\n", path_outgoing_mail);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_OUTGOING_MAIL_ERROR);
				}
			tempdirectory = opendir(path_outgoing_mail);
			if (tempdirectory != 0) closedir(tempdirectory);
				else {	// directory does not exist:
					sprintf(logentry, "Error: path for outgoing mail is not valid: %s!\n", path_outgoing_mail);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_OUTGOING_MAIL_ERROR);		
				}
			} else {	// we have no value in ini-file:
				write_logfile("Warning: path for outgoing mail is not specified in ini-file!\n", LOG_WARNING);
			}			
			
			// we check the path for processed mail:
			if (strlen(path_processed_mail) > 0) { // there's a value given in ini-file 
				if (path_processed_mail[ strlen(path_processed_mail) - 1] != '/') {
					sprintf(logentry, "Error: path for processed mail must have \'/\' at end: %s !\n", path_processed_mail);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_PROCESSED_MAIL_ERROR);
				}
			tempdirectory = opendir(path_processed_mail);
			if (tempdirectory != 0) closedir(tempdirectory); 
				else {	// directory does not exist:
					sprintf(logentry, "Error: path for processed mail is not valid: %s!\n", path_processed_mail);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_PROCESSED_MAIL_ERROR);		
				}
			} else {	// we have no value in ini-file:
				write_logfile("Warning: path for processed mail is not specified in ini-file!\n", LOG_WARNING);
			}			

			// we check file for saving process-ID for mail:
			if (strlen(fullpath_mailfile_PID) == 0)  // there's no value given in ini-file :
				write_logfile("Warning: file for saving PID for mail is not specified in ini-file!\n", LOG_WARNING);

			// we check the path for incoming telegram:
			if (strlen(path_incoming_telegram) > 0) { // there's a value given in ini-file 
				if (path_incoming_telegram[ strlen(path_incoming_telegram) - 1] != '/') {
					sprintf(logentry, "Error: path for incoming telegrams must have \'/\' at end: %s !\n", path_incoming_telegram);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_INCOMING_TELEGRAM_ERROR);
				}			
			tempdirectory = opendir(path_incoming_telegram);
			if (tempdirectory != 0) closedir(tempdirectory); 
				else {
					sprintf(logentry, "Error: path for incoming telegram is not valid: %s!\n", path_incoming_telegram);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_INCOMING_TELEGRAM_ERROR);		
				}
			} else {	// we have no value in ini-file:
				write_logfile("Warning: path for incoming telegram is not specified in ini-file!\n", LOG_WARNING);
			}				
			
			// we check the path for outgoing telegram:
			if (strlen(path_outgoing_telegram) > 0) { // there's a value given in ini-file 
				if (path_outgoing_telegram[ strlen(path_outgoing_telegram) - 1] != '/') {
					sprintf(logentry, "Error: path for outgoing telegrams must have \'/\' at end: %s !\n", path_outgoing_telegram);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_OUTGOING_TELEGRAM_ERROR);
				}			
			
			tempdirectory = opendir(path_outgoing_telegram);
			if (tempdirectory != 0) closedir(tempdirectory); 
				 else {	// directory does not exist:
					sprintf(logentry, "Error: path for outgoing telegram is not valid: %s!\n", path_outgoing_telegram);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_OUTGOING_TELEGRAM_ERROR);		
				}
			} else {	// we have no value in ini-file:
				write_logfile("Warning: path for outgoing telegram is not specified in ini-file!\n", LOG_WARNING);
			}
			
			// we check the path for processed telegram:
			if (strlen(path_processed_telegram) > 0) { // there's a value given in ini-file 
				if (path_processed_telegram[ strlen(path_processed_telegram) - 1] != '/') {
					sprintf(logentry, "Error: path for processed telegrams must have \'/\' at end: %s !\n", path_processed_telegram);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_PROCESSED_TELEGRAM_ERROR);
				}			

			tempdirectory = opendir(path_processed_telegram);
			if (tempdirectory != 0) closedir(tempdirectory); 
				else {	// directory does not exist:
					sprintf(logentry, "Error: path for processed telegram is not valid: %s!\n", path_processed_telegram);
					write_logfile(logentry, LOG_ERROR);
					return(CONFIG_FILE_PATH_PROCESSED_TELEGRAM_ERROR);		
				}
			} else {	// we have no value in ini-file:
				write_logfile("Warning: path for processed telegram is not specified in ini-file!\n", LOG_WARNING);
			}			
				
			// we check if check_online_status_waittime is a valid value:
			check_online_status_waittime_long = strtoul(check_online_status_waittime, &stop_ptr,10);
			if (*stop_ptr != 0) {
				printf("Error: Invalid value for parameter %s in inifile: %s!\n", check_online_status_waittime_string, check_online_status_waittime);
				return CONFIG_FILE_CHECK_ONLINE_WAIT_TIME_ERROR; 
			}			


				
		} // end of file is small enough, we start to extract all config-parameters
	} else { // opening of configfile was NOT successfull
		printf("Error: file %s could not be opened for reading!\n", configfilename);
		return CONFIG_FILE_READ_ERROR;
	};
		
	return NOERROR;
} // end of function read_config_file ***************************************************************

//*****************************************************************************************************************
// setup_help_text: sets up the text which is send back when we get the command to send a help.
// input: buffer for the text 
// returncodes: 0 = no error
// 				else = any error
//*****************************************************************************************************************
unsigned int setup_help_text(char *buffer) {

strcat(buffer, "Available commands for message2action:\n");
strcat(buffer, message2action_help_command);
strcat(buffer, " : get this help. You can use also ");
strcat(buffer, message2action_questionmark_command);
strcat(buffer, "\n");
strcat(buffer, message2action_config_command);
strcat(buffer, " : request configuration\n");
strcat(buffer, status_pi_command);
strcat(buffer, " : send status about processed files, errors and warnings\n");
strcat(buffer, "WOL00 ... WOL09 : send wake on LAN to device 00 ... 09\n");
strcat(buffer, "WOL00");
strcat(buffer, online_status_command_suffix);
strcat(buffer, " ... WOL09");
strcat(buffer, online_status_command_suffix);
strcat(buffer, " : check if device 00 ... 09 is online\n");
strcat(buffer, "CMD00 ... CMD09 : execute command 00 ... 09 on computer\n");
strcat(buffer, message2action_stop_command);
strcat(buffer, " : stop message2action\n");
strcat(buffer, reboot_pi_command);
strcat(buffer, " : reboot the computer\n");
strcat(buffer, shutdown_pi_command); 
strcat(buffer, " : shutdown the computer");
	
if (strlen(buffer) >= MAX_SIZE_HELP_TEXT) return SETUP_HELP_TEXT_TOO_LONG; else return NOERROR; 	
	
}	// end of setup_help_text

//*****************************************************************************************************************
// setup_config_text: sets up the text which is send back when we get the command to send the configuration.
// input: buffer for the text 
// returncodes: 0 = no error
// 				else = any error
//*****************************************************************************************************************
int setup_config_text(char *buffer) {

char tempchar[200] = "";
int tempint = 0;

	sprintf(buffer, "Number of parameters found: %d\nCommands for Raspberry:\n", configparametercount);
	sprintf(tempchar, "Shutdown: %s. Reboot: %s. Status: %s. Stop: %s\n", shutdown_pi_command, reboot_pi_command, status_pi_command, message2action_stop_command);
	strcat(buffer, tempchar);
	sprintf(tempchar, "Help: %s or %s. Config: %s\n", message2action_help_command, message2action_questionmark_command, message2action_config_command);
	strcat(buffer, tempchar);
	sprintf(tempchar, "Maximum age of files for garbage collection: %lu\n", garbage_collection_days_long);
	strcat(buffer, tempchar);
	sprintf(tempchar, "Files for saving process-ID, sms: %s; mail: %s; telegram: %s\n", fullpath_smsfile_PID, fullpath_mailfile_PID, fullpath_telegramfile_PID);
	strcat(buffer, tempchar);
	sprintf(tempchar, "Filename for saving stop-message: %s\n", message2action_stop_filename);
	strcat(buffer, tempchar);
	sprintf(tempchar, "Path for incoming SMS: %s, outgoing SMS: %s, processed SMS: %s\n", path_incoming_SMS, path_outgoing_SMS, path_processed_SMS);
	strcat(buffer, tempchar);
	sprintf(tempchar, "Path for incoming mail: %s, outgoing mail: %s, processed mail: %s\n", path_incoming_mail, path_outgoing_mail, path_processed_mail);	
	strcat(buffer, tempchar);
	sprintf(tempchar, "Path for incoming telegram: %s, outgoing telegram: %s, processed telegram: %s\n", path_incoming_telegram, path_outgoing_telegram, path_processed_telegram);
	strcat(buffer, tempchar);
	sprintf(tempchar, "Logfile: %s\n", file_logfile);
	strcat(buffer, tempchar);
	sprintf(tempchar, "Maximum size of logfile (kBytes): %s\n", max_size_logfile);
	strcat(buffer, tempchar);
	sprintf(tempchar, "Maximum size of config-file (bytes): %d\n", MAX_CONFIG_FILE_SIZE);
	strcat(buffer, tempchar);
	sprintf(tempchar, "Time to wait for next message in seconds: ");
	strcat(buffer, tempchar);
	sprintf(tempchar, "SMS: %s, mail: %s, telegram: %s\n", wait_time_next_sms, wait_time_next_mail, wait_time_next_telegram);
	strcat(buffer, tempchar);
	sprintf(tempchar, "Suffix after WOL00 ... WOL09 for checking onlinestatus: %s\n", online_status_command_suffix);
	strcat(buffer, tempchar);
	sprintf(tempchar, "Waittime after sending WOL for online-check: %s\n", check_online_status_waittime);
	strcat(buffer, tempchar);

	// now we print the cell-phone-# :
	for (tempint = cell_phone00_count; tempint < cell_phone09_count + 1; tempint++) {
		if (strcmp(configvariables[tempint], "\0") == 0) { 
			sprintf(tempchar, "Cellphone # %d: not specified in ini-file.\n", tempint - cell_phone00_count);
			strcat(buffer, tempchar);
 
		} else { 
			sprintf(tempchar, "Cellphone # %d: %s\n", tempint - cell_phone00_count , configvariables[tempint]);
			strcat(buffer, tempchar);
		}
	} // end of for-loop 

	// now we print the mailaddresses:
	for (tempint = mail_address_00_count; tempint < mail_address_09_count + 1; tempint++) {
		if (strcmp(configvariables[tempint], "\0") == 0) {
			sprintf(tempchar, "Mailaddress # %d: not specified in ini-file.\n", tempint - mail_address_00_count);
			strcat(buffer, tempchar);
		} else { 
			sprintf(tempchar, "Mailaddress # %d: %s\n", tempint - mail_address_00_count , configvariables[tempint]);
			strcat(buffer, tempchar);
		}
	} // end of for-loop 

	// now we print the token for the bot :
	sprintf(tempchar, "Token for bot: %s\n", telegram_bot_token);
	strcat(buffer, tempchar);
	
	// now we print all Telegram chat-ids:
	for (tempint = telegram_chat_id_00_count; tempint < telegram_chat_id_09_count + 1; tempint++) {
		if (strcmp(configvariables[tempint], "\0") == 0) {
			sprintf(tempchar, "Chat-id # %d: not specified in ini-file.\n", tempint - mail_address_00_count);
			strcat(buffer, tempchar);
		} else { 
			sprintf(tempchar, "Chat-id # %d: %s\n", tempint - telegram_chat_id_00_count , configvariables[tempint]);
			strcat(buffer, tempchar);
		}
	} // end of for-loop 
	
	// now we print the commands out which can be send to raspberry:
	for (tempint = messsagecommand_00_count; tempint < messsagecommand_09_count + 1; tempint++) {
		if (strcmp(configvariables[tempint], "\0") == 0) { 
			sprintf(tempchar, "Message-command # %d: not specified in ini-file.\n", tempint - messsagecommand_00_count);
			strcat(buffer, tempchar);
		} else {
			sprintf(tempchar, "Message-command # %d: %s\n", tempint - messsagecommand_00_count , configvariables[tempint]);
			strcat(buffer, tempchar);
		}
	} // end of for-loop 

	// now we print the MAC-addresses to which the WOL-broadcast can be send:
	for (tempint = WOL_MAC_00_count; tempint < WOL_MAC_09_count + 1; tempint++) {
		if (strcmp(configvariables[tempint], "00:00:00:00:00:00") == 0) { 
			sprintf(tempchar, "WOL-MAC-adress # %d: not specified in ini-file.\n", tempint - WOL_MAC_00_count);
			strcat(buffer, tempchar);
		} else { 
			sprintf(tempchar, "WOL-MAC-adress # %d: %s", tempint - WOL_MAC_00_count, configvariables[tempint]);
			strcat(buffer, tempchar);
			if ( strlen( configvariables[tempint + hostname_00_count - WOL_MAC_00_count]) != 0) { 
				sprintf(tempchar, " with hostname: %s\n", configvariables[tempint + hostname_00_count - WOL_MAC_00_count]); 
				strcat(buffer, tempchar);
			} else {
				if ( strlen(configvariables[tempint + ip_address_00_count - WOL_MAC_00_count]) != 0) { 
					sprintf(tempchar, " with ip-address: %s\n", configvariables[tempint + ip_address_00_count - WOL_MAC_00_count]); 
					strcat(buffer, tempchar);
				} else {
				 strcat(buffer, "\n");
				}
			}
		}
	} // end of for-loop 

if (strlen(buffer) >= MAX_SIZE_CONFIG_TEXT) return SETUP_CONFIG_TEXT_TOO_LONG; else return NOERROR; 	
	
}	// end of setup_config_text

//*****************************************************************************************************************
// print_configfile: prints the content of the lofile on the screen. Therefore it prints simply the config-text.
// input: none
// returncodes: 0 = no error
// 				1 = any error
//*****************************************************************************************************************
int print_configfile(void) {

int tempint;
char config_text[MAX_SIZE_CONFIG_TEXT] = "";	// this array will save the text which is send as a reply when we receive a config-command

if (read_config_file(config_filename) == NOERROR) {
	tempint = setup_config_text(config_text);
	if (tempint != 0) return SETUP_CONFIG_TEXT_TOO_LONG; else printf(config_text);
}
	
	
return 0;	
} // end of function print_configfile ***************************************************************

//*****************************************************************************************************************
// sendWOL: sends out the WOL-message to the a MAC-adress. The MAC-adress is taken from the ini-file. 
// input: 	buffer : the source where to read from, it is a string, for example "A9"

// returncodes: 0 = no error
// 				!= 0 means any error
//*****************************************************************************************************************

unsigned sendWOL(char *WOL_hex) {
unsigned int tempint; 
int socketdescriptor;  	// return-value of function socket
int socketoption; 		// return-value of function setsocketopt
int optionvalue = 1; 
unsigned char message[102]; // will store the magic packet 
unsigned char *message_ptr = message;

struct sockaddr_in socketaddress;

unsigned int socketport = 60000;
unsigned long broadcast = 0xFFFFFFFF;
ssize_t sendlength;

// Note these defines: AF_INET = IPV4; AF_INET6 = IPV6;

socketdescriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
if (socketdescriptor < 0) { 
		write_logfile("Error: Socket for sending WOL could not be opened!\n", LOG_ERROR); 
		return SENDWOL_SOCKET_ERROR;	
	} else {
		memset(message_ptr, 0xFF, 6);  // copy 6 times 0xFF at in the beginning
		message_ptr += 6; // step 6 bytes forward
		for (tempint = 0; tempint < 16; ++tempint) {
			memcpy(message_ptr, WOL_hex, 6);
			message_ptr += 6;
		}  // end of for-loop
		
	socketoption = setsockopt(socketdescriptor, SOL_SOCKET, SO_BROADCAST, &optionvalue, sizeof(optionvalue));
		if (socketoption < 0) close(socketdescriptor); else {
			socketaddress.sin_family = AF_INET;
			socketaddress.sin_addr.s_addr = broadcast;
			socketaddress.sin_port = htons(socketport);
			
			sendlength = sendto(socketdescriptor,(char *)message, sizeof(message), 0, (struct sockaddr *)&socketaddress, sizeof(socketaddress));
			if (sendlength < 0) { 
				write_logfile("Error: sending of WOL-command failed!\n", LOG_ERROR);
				return SENDWOL_SEND_ERROR;
			} else {
				close(socketdescriptor); 
			} 
		} 	// end of sendding message out	
	} // end of setting socket option
		
return NOERROR;
	
} // end of sendWOL ***********************************************************************************************

unsigned int return_receiver_counter(const char* sender, const int inbound) {
unsigned int tempint;

switch (inbound) {
	case MESSAGE_TYPE_SMS: for (tempint=cell_phone00_count;;tempint++) {
								if (strcmp(sender, configvariables[tempint]) == 0) return tempint - cell_phone00_count;
							}	// end of for-loop 	 
		break;
	
	case MESSAGE_TYPE_MAIL: for (tempint=mail_address_00_count;;tempint++) {
								if (strcmp(sender, configvariables[tempint] ) == 0) return tempint - mail_address_00_count;
							}	// end of for-loop 	 
		break;

	case MESSAGE_TYPE_TELEGRAM: for (tempint=telegram_chat_id_00_count;;tempint++) {
								if (strcmp(sender, configvariables[tempint] ) == 0) return tempint - telegram_chat_id_00_count;
							}	// end of for-loop 	 
		break;

} // end of switch	

return NOERROR;	

} // end of return_receiver_counter *******************************************************************************

//*****************************************************************************************************************
// return_outbound_messagetype: we need to know several times the messagetype (sms, mail...) we have to use for sending back messages
// input: none because we use global variables 
// returns: it returns the messagetype as an int 
//*****************************************************************************************************************
unsigned int return_outbound_messagetype(void) {
	
if (outbound_sms_flag == TRUE) return MESSAGE_TYPE_SMS;
if (outbound_mail_flag == TRUE) return MESSAGE_TYPE_MAIL;
if (outbound_telegram_flag == TRUE) return MESSAGE_TYPE_TELEGRAM;
	
return NOERROR;	
} // end of return_outbound_messagetype

//*****************************************************************************************************************
// return_inbound_messagetype: we need to know several times the messagetype (sms, mail...) we have to use for receivng messages
// input: none because we use global variables 
// returns: it returns the messagetype as an int 
//*****************************************************************************************************************
unsigned int return_inbound_messagetype(void) {
	
if (inbound_sms_flag == TRUE) return MESSAGE_TYPE_SMS;
if (inbound_mail_flag == TRUE) return MESSAGE_TYPE_MAIL;
if (inbound_telegram_flag == TRUE) return MESSAGE_TYPE_TELEGRAM;

return NOERROR;	
} // end of return_inbound_messagetype


//*****************************************************************************************************************
// create_message_to_send: creates a text file which will be send out to cellphone (SMS), mailaddress (mail), 
// telegram (sending a note) 
// input: string which has to be send out; string which contains the address (cell-phone-#, mailaddress, telegram chat-id), 
// type of inbound, type of outbound
// returncodes: MESSAGE_CREATE_OK = 0, MESSAGE_CREATE_FAIL = 1
//*****************************************************************************************************************
int create_message_to_send(const char* messagetosend, const char* receiver, const unsigned int inbound, const unsigned int outbound) {
	
FILE *messagefile; 	// the textfile which will contain the message we want to send out
char smsfilenamebegin[5 + 11 + 8 + 1] = "GSM1."; 	// filename will be like "GSM1.DD.MM.YYYY_HH:MM:SS", so this is the beginning of the filename
time_t timestamp_time; 	
struct tm *timestamp_tm;
char date_time[40] = "01.01.2000 01:01:01";  // string to save date + time, this is default-value
char pathtofile[125] = "\0";	// the path to the file we will create						
char logentry[MAX_CHARS_PER_LINE_LOGFILE]; 		// one line for the logfile
char message_content[MAX_SIZE_CONFIG_TEXT + 250] = "";		// the content for the message (mail, sms, telegram ...) we will create
char fullpath_for_message[MAX_CHARS_PER_LINE_CONFIGFILE + 50];
char tempstring[200] = "";
int randomnumber;
char randomnumberstring[20];
time(&timestamp_time);		// we get a timestamp				
timestamp_tm = localtime( &timestamp_time);	// format the timestamp
strftime(date_time, 21, "%d.%m.%Y_%H:%M:%S_", timestamp_tm);	// get a string with timestamp

randomnumber = rand() / 1048576;	// we need only a small number
sprintf(randomnumberstring, "%i", randomnumber); 	// get a string with a random-number
		

switch (outbound) { // depending on the outbound we create our file
	case MESSAGE_TYPE_SMS: 
		time(&timestamp_time);
		timestamp_tm = localtime( &timestamp_time);
		strftime(date_time, 21, "%d.%m.%Y_%H:%M:%S_", timestamp_tm);
		strcat(smsfilenamebegin, date_time);
		strcpy(pathtofile, path_outgoing_SMS);
		strcat(pathtofile, smsfilenamebegin);
		srand(clock());			// initialize random generator
		randomnumber = rand() / 1048576;	// we need only a small number
		sprintf(randomnumberstring, "%i", randomnumber); 
		strcat(pathtofile, randomnumberstring);

		messagefile = fopen(pathtofile, "w+");
		if ( messagefile != 0) {
			strcpy(tempstring, configvariables[ return_receiver_counter(receiver, inbound) + cell_phone00_count]);
			sprintf(logentry, "Info: outgoing SMS-file was created for # %s: %s\n", tempstring, pathtofile);
			write_logfile(logentry, LOG_INFO);
			sprintf(message_content, "To: %s\n\n%s", tempstring, messagetosend); // we build the complete message 
			fputs(message_content, messagefile);
			fclose(messagefile);
			return MESSAGE_CREATE_OK; 
		} else { 	// we could not open the outgoing message
			sprintf(logentry, "Error: outgoing SMS-file could not be created: %s!\n", pathtofile);
			write_logfile(logentry, LOG_ERROR);
			return MESSAGE_CREATE_FAIL;
		} 	// end of else
		break;
		
	case MESSAGE_TYPE_MAIL: 
		strcpy(fullpath_for_message, path_outgoing_mail);
		strcat(fullpath_for_message, date_time);
		strcat(fullpath_for_message, randomnumberstring);
		strcat(fullpath_for_message, ".mail");

		// we create the file which holds the mail-content:
		messagefile = fopen(fullpath_for_message, "w+");
		if (messagefile != 0) {
			strcpy(tempstring, configvariables[ return_receiver_counter(receiver, inbound) + mail_address_00_count]);
			sprintf(logentry, "Info: file for saving mail-content was created for: %s\n", tempstring);
			write_logfile(logentry, LOG_INFO);
			if (strlen(messagetosend) < MAX_CHARS_FOR_SUBJECT_IN_MAIL) 
				sprintf(message_content, "To: %s\nSubject: %s\nMessage from message2action, see subject for details.", tempstring, messagetosend); // we build the complete mail-content 
			else { 	// the message is to big to be send in the subject of a mail, we put the message in the body of the mail:
				sprintf(message_content, "To: %s\nSubject: Message from message2action, see body for details.\n%s", tempstring, messagetosend);
			}
			fputs(message_content, messagefile);
			fclose(messagefile);			
		} else {	// we could not open the file for creating + writing:
			sprintf(logentry, "Error: file for saving mail-content could not be created: %s!\n", fullpath_for_message);
			write_logfile(logentry, LOG_ERROR);
			return MESSAGE_CREATE_FAIL;
		}

		break;
		
	case MESSAGE_TYPE_TELEGRAM:
		strcpy(fullpath_for_message, path_outgoing_telegram);
		strcat(fullpath_for_message, date_time);
		strcat(fullpath_for_message, randomnumberstring);
		strcat(fullpath_for_message, ".telegram");
		// we create the file which holds the telegram-content:
		messagefile = fopen(fullpath_for_message, "w+");
		if (messagefile != 0) {
			strcpy(tempstring, configvariables[ return_receiver_counter(receiver, inbound) + telegram_chat_id_00_count]);
			sprintf(logentry, "Info: file for saving telegram-content was created; %s\n", fullpath_for_message);
			write_logfile(logentry, LOG_INFO);
			sprintf(message_content, "Token: %s\nChat-id: %s\nMessage to bot: %s", telegram_bot_token, tempstring, messagetosend); // we build the complete message for telegram 
			fputs(message_content, messagefile);
			fclose(messagefile);			
		} else {	// we could not open the file for creating + writing:
			sprintf(logentry, "Error: file for saving telegram-content could not be created: %s!\n", fullpath_for_message);
			write_logfile(logentry, LOG_ERROR);
			return MESSAGE_CREATE_FAIL;
		}			
		break;	
} // end of switch-loop (outbound)

return NOERROR;
	
} 	// end of create_message_to_send ******************************************************************************


//*****************************************************************************************************************
// check_onlinestatus: Check if an ip-address or a hostname is online via sending ping
// input: 	pingaddress : string of ip-address or hostname
//			resultstring : save the resultmessage to this string 
// returncodes: 0 = no error
// 				1 = any error
//*****************************************************************************************************************
int check_onlinestatus(const char * pingaddress, char * resultstring) {

char logentry[MAX_CHARS_PER_LINE_LOGFILE]; 	// one line in logfile
char pingcommand[256] = "/bin/ping -c 1 -w 2 "; 	// full programm to call with all parameters: -c : counter or retries, -w : timeout in seconds
char buffer[256] = ""; 		// buffer where the return-value of ping is saved
FILE *pipe;   			// pipe for saving return-value of ping
int tempint = 0;

strcat(pingcommand, pingaddress);
strcat(pingcommand, " 2>&1");		// standout for errors should go on screen
pipe = popen(pingcommand, "r"); 	// open pipe for reading
if (pipe != 0) {	// we could open the pipe
	// now we read 5 times from pipe in order to get a line with status of ping:
		for (tempint = 0; tempint < 5; tempint++) 
			if (fgets(buffer, 256, pipe) == NULL) break;
	
		pclose(pipe);	// we close pipe anywhere, we do not read more chars from pipe
	
		// now we check if we have a string "0% packet loss" or not in the line:
		if (strstr(buffer, "0% packet loss") != 0) {
			sprintf(logentry, "Info: %s is online.\n", pingaddress);
			write_logfile(logentry, LOG_INFO);
			strcpy(resultstring, logentry); 
			return PING_HOST_ONLINE; 
		} else {	// we do not have "0% packet loss" in result. Either host is offline or unknown
			sprintf(logentry, "Error: %s is either offline, unknown or anything else is not OK.\n", pingaddress);
			write_logfile(logentry, LOG_ERROR);
			strcpy(resultstring, logentry); 
			sprintf(logentry, "Error: result from ping-command: %s \n", buffer);
			write_logfile(logentry, LOG_ERROR);
			return PING_UNKNOWN_OR_OFFLINE_HOST;
		}
	/*
	if ( strstr(buffer, "ping: unknown host") != 0) {
		pclose(pipe);
		sprintf(logentry, "Warning: %s is unknown.\n", pingaddress);
		write_logfile(logentry, LOG_WARNING);
		strcpy(resultstring, logentry); 
		return PING_UNKNOWN_HOST; 
		} else { 
			do {	// we read from pipe until we have the line containing "% packet loss"
				fgets(buffer, 256, pipe);
			} while (strstr(buffer, "% packet loss") == 0);
			// now we check if we have 0% packet loss or not:
			if ( strstr(buffer, " 0%") != 0 ) { 
				pclose(pipe);
				sprintf(logentry, "Info: %s is online.\n", pingaddress);
				write_logfile(logentry, LOG_INFO);
				strcpy(resultstring, logentry); 
				return PING_HOST_ONLINE; 
			} else {
				pclose(pipe);
				sprintf(logentry, "Info: %s is offline.\n", pingaddress);
				write_logfile(logentry, LOG_INFO);
				strcpy(resultstring, logentry); 
				return PING_HOST_OFFLINE;  
			}
		}
	pclose(pipe);
	*/
	
	} else {	// we could not open pipe
		sprintf(logentry, "Error: could not open pipe for %s\n", pingcommand);
		write_logfile(logentry, LOG_ERROR);
		return PING_PIPE_ERROR; 
	}

} // end of function check_onlinestatus ****************************************************************

//*****************************************************************************************************************
// execute_command: performs the command we have found in message
// input: number of command we have found, the sender of the message
// returncodes: 0 = no error
// 				1 = any error
//*****************************************************************************************************************
int execute_command(const int commandnumber, const char* sender_of_message) {

char logentry[MAX_CHARS_PER_LINE_LOGFILE];  
char tempchar[10];
char tempstring[100];
char online_result[100]; 

int errorcode = NOERROR;		// returnvalue from a function called

	switch (commandnumber) {
			case messsagecommand_00_count : if (strlen(messsagecommand_00) != 0) {	
												if (system(messsagecommand_00) == 0) { 	// we could execute the command
													strcpy(logentry, "Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " was executed succesfully.\n");
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												} else { 	// we could not execute the command
													strcpy(logentry, "Error: Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " could not be executed!\n");
													write_logfile(logentry, LOG_ERROR);
													errorcode = EXECUTE_COMMAND_SYSTEM_ERROR;
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												}
											} else {	// the command is empty
												sprintf(logentry, "Error: Command %s to be executed is not specified in inifile!\n", configparameters[commandnumber]);
												write_logfile(logentry, LOG_ERROR);
												errorcode = EXECUTE_COMMAND_NO_COMMAND_ERROR;	
												// we send result back to outputchanel:
												create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
											}	
											break;
											
			case messsagecommand_01_count : if (strlen(messsagecommand_01) != 0) {	
												if (system(messsagecommand_01) == 0) { 	// we could execute the command
													strcpy(logentry, "Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " was executed succesfully.\n");
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												} else { 	// we could not execute the command
													strcpy(logentry, "Error: Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " could not be executed!\n");
													write_logfile(logentry, LOG_ERROR);
													errorcode = EXECUTE_COMMAND_SYSTEM_ERROR;
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												}
											} else {	// the command is empty
												sprintf(logentry, "Error: Command %s to be executed is not specified in inifile!\n", configparameters[commandnumber]);
												write_logfile(logentry, LOG_ERROR);
												errorcode = EXECUTE_COMMAND_NO_COMMAND_ERROR;	
												// we send result back to outputchanel:
												create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
											}	
											break;
											
			case messsagecommand_02_count : if (strlen(messsagecommand_02) != 0) {	
												if (system(messsagecommand_02) == 0) { 	// we could execute the command
													strcpy(logentry, "Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " was executed succesfully.\n");
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												} else { 	// we could not execute the command
													strcpy(logentry, "Error: Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " could not be executed!\n");
													write_logfile(logentry, LOG_ERROR);
													errorcode = EXECUTE_COMMAND_SYSTEM_ERROR;
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												}
											} else {	// the command is empty
												sprintf(logentry, "Error: Command %s to be executed is not specified in inifile!\n", configparameters[commandnumber]);
												write_logfile(logentry, LOG_ERROR);
												errorcode = EXECUTE_COMMAND_NO_COMMAND_ERROR;	
												// we send result back to outputchanel:
												create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
											}	
											break;
											
			case messsagecommand_03_count : if (strlen(messsagecommand_03) != 0) {	
												if (system(messsagecommand_03) == 0) { 	// we could execute the command
													strcpy(logentry, "Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " was executed succesfully.\n");
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												} else { 	// we could not execute the command
													strcpy(logentry, "Error: Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " could not be executed!\n");
													write_logfile(logentry, LOG_ERROR);
													errorcode = EXECUTE_COMMAND_SYSTEM_ERROR;
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												}
											} else {	// the command is empty
												sprintf(logentry, "Error: Command %s to be executed is not specified in inifile!\n", configparameters[commandnumber]);
												write_logfile(logentry, LOG_ERROR);
												errorcode = EXECUTE_COMMAND_NO_COMMAND_ERROR;	
												// we send result back to outputchanel:
												create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
											}	
											break;
											
			case messsagecommand_04_count : if (strlen(messsagecommand_04) != 0) {	
												if (system(messsagecommand_04) == 0) { 	// we could execute the command
													strcpy(logentry, "Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " was executed succesfully.\n");
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												} else { 	// we could not execute the command
													strcpy(logentry, "Error: Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " could not be executed!\n");
													write_logfile(logentry, LOG_ERROR);
													errorcode = EXECUTE_COMMAND_SYSTEM_ERROR;
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												}
											} else {	// the command is empty
												sprintf(logentry, "Error: Command %s to be executed is not specified in inifile!\n", configparameters[commandnumber]);
												write_logfile(logentry, LOG_ERROR);
												errorcode = EXECUTE_COMMAND_NO_COMMAND_ERROR;	
												// we send result back to outputchanel:
												create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
											}	
											break;
											
			case messsagecommand_05_count : if (strlen(messsagecommand_05) != 0) {	
												if (system(messsagecommand_05) == 0) { 	// we could execute the command
													strcpy(logentry, "Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " was executed succesfully.\n");
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												} else { 	// we could not execute the command
													strcpy(logentry, "Error: Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " could not be executed!\n");
													write_logfile(logentry, LOG_ERROR);
													errorcode = EXECUTE_COMMAND_SYSTEM_ERROR;
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												}
											} else {	// the command is empty
												sprintf(logentry, "Error: Command %s to be executed is not specified in inifile!\n", configparameters[commandnumber]);
												write_logfile(logentry, LOG_ERROR);
												errorcode = EXECUTE_COMMAND_NO_COMMAND_ERROR;	
												// we send result back to outputchanel:
												create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
											}	
											break;
											
			case messsagecommand_06_count : if (strlen(messsagecommand_06) != 0) {	
												if (system(messsagecommand_06) == 0) { 	// we could execute the command
													strcpy(logentry, "Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " was executed succesfully.\n");
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												} else { 	// we could not execute the command
													strcpy(logentry, "Error: Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " could not be executed!\n");
													write_logfile(logentry, LOG_ERROR);
													errorcode = EXECUTE_COMMAND_SYSTEM_ERROR;
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												}
											} else {	// the command is empty
												sprintf(logentry, "Error: Command %s to be executed is not specified in inifile!\n", configparameters[commandnumber]);
												write_logfile(logentry, LOG_ERROR);
												errorcode = EXECUTE_COMMAND_NO_COMMAND_ERROR;	
												// we send result back to outputchanel:
												create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
											}	
											break;
											
			case messsagecommand_07_count : if (strlen(messsagecommand_07) != 0) {	
												if (system(messsagecommand_07) == 0) { 	// we could execute the command
													strcpy(logentry, "Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " was executed succesfully.\n");
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												} else { 	// we could not execute the command
													strcpy(logentry, "Error: Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " could not be executed!\n");
													write_logfile(logentry, LOG_ERROR);
													errorcode = EXECUTE_COMMAND_SYSTEM_ERROR;
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												}
											} else {	// the command is empty
												sprintf(logentry, "Error: Command %s to be executed is not specified in inifile!\n", configparameters[commandnumber]);
												write_logfile(logentry, LOG_ERROR);
												errorcode = EXECUTE_COMMAND_NO_COMMAND_ERROR;	
												// we send result back to outputchanel:
												create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
											}	
											break;
											
			case messsagecommand_08_count : if (strlen(messsagecommand_08) != 0) {	
												if (system(messsagecommand_08) == 0) { 	// we could execute the command
													strcpy(logentry, "Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " was executed succesfully.\n");
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												} else { 	// we could not execute the command
													strcpy(logentry, "Error: Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " could not be executed!\n");
													write_logfile(logentry, LOG_ERROR);
													errorcode = EXECUTE_COMMAND_SYSTEM_ERROR;
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												}
											} else {	// the command is empty
												sprintf(logentry, "Error: Command %s to be executed is not specified in inifile!\n", configparameters[commandnumber]);
												write_logfile(logentry, LOG_ERROR);
												errorcode = EXECUTE_COMMAND_NO_COMMAND_ERROR;	
												// we send result back to outputchanel:
												create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
											}	
											break;
																						
			case messsagecommand_09_count : if (strlen(messsagecommand_09) != 0) {	
												if (system(messsagecommand_09) == 0) { 	// we could execute the command
													strcpy(logentry, "Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " was executed succesfully.\n");
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												} else { 	// we could not execute the command
													strcpy(logentry, "Error: Command ");
													strcat(logentry, configparameters[commandnumber]);
													strcat(logentry, " could not be executed!\n");
													write_logfile(logentry, LOG_ERROR);
													errorcode = EXECUTE_COMMAND_SYSTEM_ERROR;
													// we send result back to outputchanel:
													create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
												}
											} else {	// the command is empty
												sprintf(logentry, "Error: Command %s to be executed is not specified in inifile!\n", configparameters[commandnumber]);
												write_logfile(logentry, LOG_ERROR);
												errorcode = EXECUTE_COMMAND_NO_COMMAND_ERROR;	
												// we send result back to outputchanel:
												create_message_to_send(logentry, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
											}	
											break;
																						
			case WOL_MAC_00_count: errorcode = 	sendWOL(WOL_MAC_00_hex);  
									if (errorcode == NOERROR) {		// we could send out WOL-command, now we check if device is online or not:
										if (check_online_status_waittime_long != 0) {
											sleep(check_online_status_waittime_long);	// we have to wait for some time because the device might take some time to become online:
											if ( strlen( hostname_00) != 0) { 
												strcpy(tempstring, hostname_00); 
												check_onlinestatus(tempstring, online_result);
												} else 
													if ( strlen(ip_address_00) != 0) {
														strcpy(tempstring, ip_address_00); 
														check_onlinestatus(tempstring, online_result);  
													} 			
											// we send result back to outputchanel:
											create_message_to_send(online_result, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
										}	// if (check_online_status_waittime_long != 0) 
									} else {		// we could not send out WOL-command:
										sprintf(logentry, "Error: WOL could not be send out to %s!\n", WOL_MAC_00_hex);
										write_logfile(logentry, LOG_ERROR);
									}	
									break; 

			case WOL_MAC_01_count: errorcode = 	sendWOL(WOL_MAC_01_hex);  
									if (errorcode == NOERROR) {		// we could send out WOL-command, now we check if device is online or not:
										if (check_online_status_waittime_long != 0) {
											sleep(check_online_status_waittime_long);	// we have to wait for some time because the device might take some time to become online:
											if ( strlen( hostname_01) != 0) { 
												strcpy(tempstring, hostname_01); 
												check_onlinestatus(tempstring, online_result);
												} else 
													if ( strlen(ip_address_01) != 0) {
														strcpy(tempstring, ip_address_01); 
														check_onlinestatus(tempstring, online_result);  
													} 			
											// we send result back to outputchanel:
											create_message_to_send(online_result, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
										}	// if (check_online_status_waittime_long != 0) 
									} else {		// we could not send out WOL-command:
										sprintf(logentry, "Error: WOL could not be send out to %s!\n", WOL_MAC_01_hex);
										write_logfile(logentry, LOG_ERROR);
									}	
									break; 

			case WOL_MAC_02_count: errorcode = 	sendWOL(WOL_MAC_02_hex);  
									if (errorcode == NOERROR) {		// we could send out WOL-command, now we check if device is online or not:
										if (check_online_status_waittime_long != 0) {
											sleep(check_online_status_waittime_long);	// we have to wait for some time because the device might take some time to become online:
											if ( strlen( hostname_02) != 0) { 
												strcpy(tempstring, hostname_02); 
												check_onlinestatus(tempstring, online_result);
												} else 
													if ( strlen(ip_address_02) != 0) {
														strcpy(tempstring, ip_address_02); 
														check_onlinestatus(tempstring, online_result);  
													} 			
											// we send result back to outputchanel:
											create_message_to_send(online_result, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
										}	// if (check_online_status_waittime_long != 0) 
									} else {		// we could not send out WOL-command:
										sprintf(logentry, "Error: WOL could not be send out to %s!\n", WOL_MAC_02_hex);
										write_logfile(logentry, LOG_ERROR);
									}	
									break; 

			case WOL_MAC_03_count: errorcode = 	sendWOL(WOL_MAC_03_hex);  
									if (errorcode == NOERROR) {		// we could send out WOL-command, now we check if device is online or not:
										if (check_online_status_waittime_long != 0) {
											sleep(check_online_status_waittime_long);	// we have to wait for some time because the device might take some time to become online:
											if ( strlen( hostname_03) != 0) { 
												strcpy(tempstring, hostname_03); 
												check_onlinestatus(tempstring, online_result);
												} else 
													if ( strlen(ip_address_03) != 0) {
														strcpy(tempstring, ip_address_03); 
														check_onlinestatus(tempstring, online_result);  
													} 			
											// we send result back to outputchanel:
											create_message_to_send(online_result, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
										}	// if (check_online_status_waittime_long != 0) 
									} else {		// we could not send out WOL-command:
										sprintf(logentry, "Error: WOL could not be send out to %s!\n", WOL_MAC_03_hex);
										write_logfile(logentry, LOG_ERROR);
									}	
									break; 

			case WOL_MAC_04_count: errorcode = 	sendWOL(WOL_MAC_04_hex);  
									if (errorcode == NOERROR) {		// we could send out WOL-command, now we check if device is online or not:
										if (check_online_status_waittime_long != 0) {
											sleep(check_online_status_waittime_long);	// we have to wait for some time because the device might take some time to become online:
											if ( strlen( hostname_04) != 0) { 
												strcpy(tempstring, hostname_04); 
												check_onlinestatus(tempstring, online_result);
												} else 
													if ( strlen(ip_address_04) != 0) {
														strcpy(tempstring, ip_address_04); 
														check_onlinestatus(tempstring, online_result);  
													} 			
											// we send result back to outputchanel:
											create_message_to_send(online_result, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
										}	// if (check_online_status_waittime_long != 0) 
									} else {		// we could not send out WOL-command:
										sprintf(logentry, "Error: WOL could not be send out to %s!\n", WOL_MAC_04_hex);
										write_logfile(logentry, LOG_ERROR);
									}	
									break; 

			case WOL_MAC_05_count: errorcode = 	sendWOL(WOL_MAC_05_hex);  
									if (errorcode == NOERROR) {		// we could send out WOL-command, now we check if device is online or not:
										if (check_online_status_waittime_long != 0) {
											sleep(check_online_status_waittime_long);	// we have to wait for some time because the device might take some time to become online:
											if ( strlen( hostname_05) != 0) { 
												strcpy(tempstring, hostname_05); 
												check_onlinestatus(tempstring, online_result);
												} else 
													if ( strlen(ip_address_05) != 0) {
														strcpy(tempstring, ip_address_05); 
														check_onlinestatus(tempstring, online_result);  
													} 			
											// we send result back to outputchanel:
											create_message_to_send(online_result, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
										}	// if (check_online_status_waittime_long != 0) 
									} else {		// we could not send out WOL-command:
										sprintf(logentry, "Error: WOL could not be send out to %s!\n", WOL_MAC_05_hex);
										write_logfile(logentry, LOG_ERROR);
									}	
									break; 

			case WOL_MAC_06_count: errorcode = 	sendWOL(WOL_MAC_06_hex);  
									if (errorcode == NOERROR) {		// we could send out WOL-command, now we check if device is online or not:
										if (check_online_status_waittime_long != 0) {
											sleep(check_online_status_waittime_long);	// we have to wait for some time because the device might take some time to become online:
											if ( strlen( hostname_06) != 0) { 
												strcpy(tempstring, hostname_06); 
												check_onlinestatus(tempstring, online_result);
												} else 
													if ( strlen(ip_address_06) != 0) {
														strcpy(tempstring, ip_address_06); 
														check_onlinestatus(tempstring, online_result);  
													} 			
											// we send result back to outputchanel:
											create_message_to_send(online_result, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
										}	// if (check_online_status_waittime_long != 0) 
									} else {		// we could not send out WOL-command:
										sprintf(logentry, "Error: WOL could not be send out to %s!\n", WOL_MAC_07_hex);
										write_logfile(logentry, LOG_ERROR);
									}	
									break; 

			case WOL_MAC_07_count: errorcode = 	sendWOL(WOL_MAC_07_hex);  
									if (errorcode == NOERROR) {		// we could send out WOL-command, now we check if device is online or not:
										if (check_online_status_waittime_long != 0) {
											sleep(check_online_status_waittime_long);	// we have to wait for some time because the device might take some time to become online:
											if ( strlen( hostname_07) != 0) { 
												strcpy(tempstring, hostname_07); 
												check_onlinestatus(tempstring, online_result);
												} else 
													if ( strlen(ip_address_07) != 0) {
														strcpy(tempstring, ip_address_07); 
														check_onlinestatus(tempstring, online_result);  
													} 			
											// we send result back to outputchanel:
											create_message_to_send(online_result, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
										}	// if (check_online_status_waittime_long != 0) 
									} else {		// we could not send out WOL-command:
										sprintf(logentry, "Error: WOL could not be send out to %s!\n", WOL_MAC_07_hex);
										write_logfile(logentry, LOG_ERROR);
									}	
									break; 

			case WOL_MAC_08_count: errorcode = 	sendWOL(WOL_MAC_08_hex);  
									if (errorcode == NOERROR) {		// we could send out WOL-command, now we check if device is online or not:
										if (check_online_status_waittime_long != 0) {
											sleep(check_online_status_waittime_long);	// we have to wait for some time because the device might take some time to become online:
											if ( strlen( hostname_08) != 0) { 
												strcpy(tempstring, hostname_08); 
												check_onlinestatus(tempstring, online_result);
												} else 
													if ( strlen(ip_address_08) != 0) {
														strcpy(tempstring, ip_address_08); 
														check_onlinestatus(tempstring, online_result);  
													} 			
											// we send result back to outputchanel:
											create_message_to_send(online_result, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
										}	// if (check_online_status_waittime_long != 0) 
									} else {		// we could not send out WOL-command:
										sprintf(logentry, "Error: WOL could not be send out to %s!\n", WOL_MAC_08_hex);
										write_logfile(logentry, LOG_ERROR);
									}	
									break; 

			case WOL_MAC_09_count: errorcode = 	sendWOL(WOL_MAC_09_hex);  
									if (errorcode == NOERROR) {		// we could send out WOL-command, now we check if device is online or not:
										if (check_online_status_waittime_long != 0) {
											sleep(check_online_status_waittime_long);	// we have to wait for some time because the device might take some time to become online:
											if ( strlen( hostname_09) != 0) { 
												strcpy(tempstring, hostname_09); 
												check_onlinestatus(tempstring, online_result);
												} else 
													if ( strlen(ip_address_09) != 0) {
														strcpy(tempstring, ip_address_09); 
														check_onlinestatus(tempstring, online_result);  
													} 			
											// we send result back to outputchanel:
											create_message_to_send(online_result, sender_of_message, return_inbound_messagetype(), return_outbound_messagetype());	
										}	// if (check_online_status_waittime_long != 0) 
									} else {		// we could not send out WOL-command:
										sprintf(logentry, "Error: WOL could not be send out to %s!\n", WOL_MAC_09_hex);
										write_logfile(logentry, LOG_ERROR);
									}	
									break; 

			default: 	strcpy(logentry, "Error: Unknown command-# found: ");
						sprintf(tempchar, "%d", commandnumber); 
						strcat(logentry, tempchar);
						write_logfile(logentry, LOG_ERROR);

	} // end of switch-loop
	
return errorcode;	
} // end of function execute_command *****************************************************************


//*****************************************************************************************************************
// install_signal_function: it installs a function in the handler-mechanism
//  
// input: signal-#, function to install
// returncodes: 0 =  OK, 1 = any error
//*****************************************************************************************************************

typedef void (*sighandler_t)(int);
int install_signal_function(int signal_number, sighandler_t signalhandler) {

struct sigaction new_signal;

new_signal.sa_handler = signalhandler;
sigemptyset(&new_signal.sa_mask);
new_signal.sa_flags = SA_RESTART;
	
if (sigaction(signal_number, &new_signal, NULL) < 0) return 1; else return 0;
	
}	// end of function install_signal_function

//*****************************************************************************************************************
// check_signal: This function will be installed as the handler for signals we are receiving. Or in other words: When we get
//  a signal like ctrl-c then this function will called and we can react on ctrl-c or even kill.
// input: signal-#
// returncodes: none
//*****************************************************************************************************************
static void check_signal(int signal_nr) {

if (signal_nr == SIGINT) write_logfile("Info: signal SIGINT was received.\n", LOG_INFO);
if (signal_nr == SIGTERM) write_logfile("Info: signal SIGTERM was received.\n", LOG_INFO);

processingflag = FALSE; 	// we have to tell the function startprocessing that we have to stop now

} 	// end of function check_signal ****************************************************************

//*****************************************************************************************************************
// check_smssender: checks if the sender of the SMS is valid. It compares the phone number with all configured phone-numbers in ini-file.
// input: string which contains the phonenumber
// returncodes: SMS_SENDER_VALID = 0, SMS_SENDER_NOTVALID = 1
//*****************************************************************************************************************
int check_smssender(const char* smssender) {
	
int temp = 0;
int validflag = FALSE;  // this variable is per default FALSE and will either be changed to TRUE or it remains FALSE
	
	for (temp = cell_phone00_count; temp < cell_phone09_count; ++temp) {	// we step through all configured cell phone numbers
		if (strcmp(smssender, configvariables[temp]) == 0 ) { validflag = TRUE; temp = cell_phone09_count; }
	}
	if (validflag == TRUE) return SMS_SENDER_VALID; else return SMS_SENDER_NOTVALID;
	
}	// end of check_smssender **********************************************************************************

//*****************************************************************************************************************
// check_mailsender: checks if the sender of the mail is valid. It compares the mailaddress we received with all configured mail-addresses in ini-file.
// input: string which contains the mailaddress
// returncodes: MAIL_SENDER_VALID = 0, MAIL_SENDER_NOTVALID = 1
//*****************************************************************************************************************
int check_mailsender(const char* mailsender) {
	
int temp = 0;
int validflag = FALSE;  // this variable is per default FALSE and will either be changed to TRUE or it remains FALSE
	
	for (temp = mail_address_00_count; temp < mail_address_09_count; ++temp) {	// we step through all configured cell phone numbers
		if (strcmp(mailsender, configvariables[temp]) == 0 ) { validflag = TRUE; temp = mail_address_09_count; }
	}
	if (validflag == TRUE) return MAIL_SENDER_VALID; else return MAIL_SENDER_NOTVALID;
	
}	// end of check_mailsender **********************************************************************************

//*****************************************************************************************************************
// check_chat_id: checks if the sender of the mail is valid. It compares the mailaddress we received with all configured mail-addresses in ini-file.
// input: string which contains the mailaddress
// returncodes: MAIL_SENDER_VALID = 0, MAIL_SENDER_NOTVALID = 1
//*****************************************************************************************************************
int check_chat_id(const char* chatid) {
	
int temp = 0;
int validflag = FALSE;  // this variable is per default FALSE and will either be changed to TRUE or it remains FALSE
	
	for (temp = telegram_chat_id_00_count; temp < telegram_chat_id_09_count; ++temp) {	// we step through all configured cell phone numbers
		if (strcmp(chatid, configvariables[temp]) == 0 ) { validflag = TRUE; temp = telegram_chat_id_09_count; }
	}
	if (validflag == TRUE) return TELEGRAM_CHATID_VALID; else return TELEGRAM_CHATID_NOTVALID;
	
}	// end of check_chat_id **********************************************************************************


//*****************************************************************************************************************
// return_cellphonenumber_from_inifile: returns the first valid cell-phone-#. This number will be written in stop-message
// input: string which contains the phonenumber we will return
// returncodes: CELL_PHONE_FOUND = 0, CELL_PHONE_NOT_FOUND = 1
//*****************************************************************************************************************
int return_cellphonenumber_from_inifile(char* cellphonenumber) {
	
int temp = 0;
int validflag = FALSE;  // this variable is per default FALSE and will either be changed to TRUE or it remains FALSE
	
	for (temp = cell_phone00_count; temp < cell_phone09_count; ++temp) {	// we step through all configured cell phone numbers
		if (strlen(configvariables[temp]) != 0 ) { 
			validflag = TRUE; 
			strcpy(cellphonenumber, configvariables[temp]);
			temp = cell_phone09_count; 
		} // we have found a cell-phone-#, we stop searching
	}
	if (validflag == TRUE) return CELL_PHONE_FOUND; else { 
			strcpy(cellphonenumber, "DUMMY");		// we copy DUMMY to cell-phone-number because we do not have a valid cell-phone-#
			return CELL_PHONE_NOT_FOUND;
			}
	
return NOERROR;	
}	// end of return_cellphonenumber_from_inifile

//*****************************************************************************************************************
// return_mailaddress_from_inifile: returns the first valid mailaddress from ini-file. This mailaddress will be written in stop-message
// input: string which contains the mailaddress we will return
// returncodes: MAIL_ADDRESS_FOUND = 0, MAIL_ADDRESS_NOT_FOUND = 1
//*****************************************************************************************************************
int return_mailaddress_from_inifile(char* mailaddress) {
	
int temp = 0;
int validflag = FALSE;  // this variable is per default FALSE and will either be changed to TRUE or it remains FALSE
	
	for (temp = mail_address_00_count; temp < mail_address_09_count; ++temp) {	// we step through all configured mailaddresses
		if (strlen(configvariables[temp]) != 0 ) { 
			validflag = TRUE; 
			strcpy(mailaddress, configvariables[temp]);
			temp = mail_address_09_count; 
		} // we have found a mailaddress, we stop searching
	}
	if (validflag == TRUE) return MAIL_ADDRESS_FOUND; else { 
			strcpy(mailaddress, "DUMMY");		// we copy DUMMY to cell-phone-number because we do not have a valid cell-phone-#
			return MAIL_ADDRESS_NOT_FOUND;
			}
	
return NOERROR;	
}	// end of return_mailaddress_from_inifile

//*****************************************************************************************************************
// return_chatid_from_inifile: returns the first valid chat-id from ini-file. This chat-id will be written in stop-message
// input: string which contains the chat-id we will return
// returncodes: CHATID_ADDRESS_FOUND = 0, CHATID_ADDRESS_NOT_FOUND = 1
//*****************************************************************************************************************
int return_chatid_from_inifile(char* chatid) {
	
int temp = 0;
int validflag = FALSE;  // this variable is per default FALSE and will either be changed to TRUE or it remains FALSE
	
	for (temp = telegram_chat_id_00_count; temp < telegram_chat_id_09_count; ++temp) {	// we step through all configured mailaddresses
		if (strlen(configvariables[temp]) != 0 ) { 
			validflag = TRUE; 
			strcpy(chatid, configvariables[temp]);
			temp = telegram_chat_id_09_count; 
		} // we have found a chat-id, we stop searching
	}
	if (validflag == TRUE) return TELEGRAM_CHATID_FOUND; else { 
			strcpy(chatid, "DUMMY");		// we copy DUMMY to cell-phone-number because we do not have a valid cell-phone-#
			return TELEGRAM_CHATID_NOT_FOUND;
			}
	
return NOERROR;	
}	// end of return_chatid_from_inifile


//*****************************************************************************************************************
// get_command_from_sms: extracts the command from a sms we have received
// input: the file we have opened. The command will be written to commandstring)
// returncodes: 0 = no error
// 				1 = any error
//*****************************************************************************************************************
unsigned int get_command_from_sms(FILE *smsfilep, char *commandstring, char *smssender) {

/*
char from_toa[50];
char from_SMSC[20];
char send_date[17];     // format: <2 digits of year>-<month>-<date of month> hh:mm:11
char received_date[17]; 
char subject[10];
char modem[10]; 
char IMSI[20];
char IMEI[20];
char report[4];
char alphabet[10];
*/
char *charNL; 
unsigned int tempint; 
char logentry[MAX_CHARS_PER_LINE_LOGFILE] = "";		// one line we write to logfile

*commandstring = '\0';
fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, smsfilep); // save line #1 from file in temp
charNL = strchr(commandstring, '\n');
if (charNL == NULL) {	// we do not have a end of line marker
	write_logfile("Error: no end-of-line-marker in line of file with command!\n", LOG_ERROR);
	return MESSAGE_FILE_ERROR;
}
*charNL = '\0';  // terminate temp string
strcpy(smssender, commandstring + 6);   // we get "From: "
if (check_smssender(smssender) == SMS_SENDER_NOTVALID) { // we check if we have received a SMS from a valid cell phone
	sprintf(logentry, "Warning: invalid sender-phonenumber in incoming SMS: %s!\n", smssender);
	write_logfile(logentry, LOG_WARNING);
	
	// now we send to all valid cell-phones a warning message:
	for (tempint = cell_phone00_count; tempint < cell_phone09_count; tempint++) {
		if (strlen(configvariables[tempint]) != 0 ) // if we have a cell-phone configured we send to the # the warning
			create_message_to_send(logentry, configvariables[tempint], return_inbound_messagetype(), return_outbound_messagetype());  // we create a SMS with a message to a cell-phone-# 
	}	// end of for-loop
	// now we exit here because we have an invalid phonenumber:
	return SMS_SENDER_NOTVALID;
} else { // the sender-phone-# is valid, we continue to read the whole incoming file:
	// we will search now for the blank line which is the last line before the content of the SMS:
	
	do {
		fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, smsfilep); // save one line from file 
		
	} while (strlen (commandstring) != 1); 
	
	fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, smsfilep); // save one line from file which contains the command we have received with the SMS:
	
	/*
	fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, smsfilep); // save line #2 from file in temp
	charNL = strchr(commandstring, '\n');
	if (charNL == NULL) {	// we do not have a end of line marker
		write_logfile("Error: no end-of-line-marker in line of file with command!\n", LOG_ERROR);
		return MESSAGE_FILE_ERROR;
	}
	*charNL = '\0';  // terminate temp string
	strcpy(from_toa, commandstring + 10);	 // we skip "From_TOA: "			

	fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, smsfilep); // save line #3 from file in temp
	charNL = strchr(commandstring, '\n');
	if (charNL == NULL) {	// we do not have a end of line marker
		write_logfile("Error: no end-of-line-marker in line of file with command!\n", LOG_ERROR);
		return MESSAGE_FILE_ERROR;
	}
	*charNL = '\0';  // terminate temp string
	strcpy(from_SMSC, commandstring + 11);	 // we skip "From_SMSC: "			

	fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, smsfilep); // save line #4 from file in temp
	charNL = strchr(commandstring, '\n');
	if (charNL == NULL) {	// we do not have a end of line marker
		write_logfile("Error: no end-of-line-marker in line of file with command!\n", LOG_ERROR);
		return MESSAGE_FILE_ERROR;
	}
	*charNL = '\0';  // terminate temp string
	strcpy(send_date, commandstring + 6);	// we skip "Sent: "			

	fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, smsfilep); // save line #5 from file in temp
	charNL = strchr(commandstring, '\n');
	if (charNL == NULL) {	// we do not have a end of line marker
		write_logfile("Error: no end-of-line-marker in line of file with command!\n", LOG_ERROR);
		return MESSAGE_FILE_ERROR;
	}
	*charNL = '\0';  // terminate temp string
	strcpy(received_date, commandstring + 10);	// we skip "Received: "			

	fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, smsfilep); // save line #6 from file in temp
	charNL = strchr(commandstring, '\n');
	if (charNL == NULL) {	// we do not have a end of line marker
		write_logfile("Error: no end-of-line-marker in line of file with command!\n", LOG_ERROR);
		return MESSAGE_FILE_ERROR;
	}
	*charNL = '\0';  // terminate temp string
	strcpy(subject, commandstring + 9);		// we skip "Subject: "		

	fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, smsfilep); // save line #7 from file in temp
	charNL = strchr(commandstring, '\n');
	if (charNL == NULL) {	// we do not have a end of line marker
		write_logfile("Error: no end-of-line-marker in line of file with command!\n", LOG_ERROR);
		return MESSAGE_FILE_ERROR;
	}
	*charNL = '\0';  // terminate temp string
	strcpy(modem, commandstring + 7);		  	// we skip "Modem: "	

	fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, smsfilep); // save line #8 from file in temp
	charNL = strchr(commandstring, '\n');
	if (charNL == NULL) {	// we do not have a end of line marker
		write_logfile("Error: no end-of-line-marker in line of file with command!\n", LOG_ERROR);
		return MESSAGE_FILE_ERROR;
	}
	*charNL = '\0';  // terminate temp string
	strcpy(IMSI, commandstring + 6);		  	// we skip "IMSI: "	

	fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, smsfilep); // save line #8 from file in temp
	charNL = strchr(commandstring, '\n');
	if (charNL == NULL) {	// we do not have a end of line marker
		write_logfile("Error: no end-of-line-marker in line of file with command!\n", LOG_ERROR);
		return MESSAGE_FILE_ERROR;
	}
	*charNL = '\0';  // terminate temp string
	strcpy(IMEI, commandstring + 6);		  	// we skip "IMEI: "	


	fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, smsfilep); // save line #9 from file in temp
	if (charNL == NULL) {	// we do not have a end of line marker
		write_logfile("Error: no end-of-line-marker in line of file with command!\n", LOG_ERROR);
		return MESSAGE_FILE_ERROR;
	}
	charNL = strchr(commandstring, '\n');
	*charNL = '\0';  // terminate temp string
	strcpy(report, commandstring + 8);		  	// we skip "Report: "	

	fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, smsfilep); // save line #10 from file in temp
	charNL = strchr(commandstring, '\n');
	if (charNL == NULL) {	// we do not have a end of line marker
		write_logfile("Error: no end-of-line-marker in line of file with command!\n", LOG_ERROR);
		return MESSAGE_FILE_ERROR;
	}
	*charNL = '\0';  // terminate temp string
	strcpy(alphabet, commandstring + 8);		  	// we skip "Alphabet: "	
				
	fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, smsfilep); // save line #11 from file in temp
	charNL = strchr(commandstring, '\n');
	if (charNL == NULL) {	// we do not have a end of line marker
		write_logfile("Error: no end-of-line-marker in line of file with command!\n", LOG_ERROR);
		return MESSAGE_FILE_ERROR;
	}
	*charNL = '\0';  // terminate temp string
	strcpy(alphabet, commandstring + 8);		  	// we skip "Length: "	

	fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, smsfilep); // save line #12 from file in temp
	fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, smsfilep); // save line #13 from file in temp, this is the content where we expect to get the command from as written in the SMS file
	*/
	upcase_string(commandstring); 	// all chars to upper case
	#ifdef DEVSTAGE		
	sprintf(logentry, "Info: content of last line in SMS-file: %s\n", commandstring);
	write_logfile(logentry, LOG_INFO);
	#endif
	 
} 
	return NOERROR;
} // end of function get_command_from_sms ****************************************************************


//*****************************************************************************************************************
// get_command_from_mail: extracts the command from a mail we have received
// input: the file we have opened; the found command will be written to commandstring; the mailaddress of the sender
// returncodes: NOERROR = no error
// 				MAIL_SENDER_NOTVALID = we have received a mail from an account we do not know
//*****************************************************************************************************************
unsigned int get_command_from_mail(FILE *mailfilep, char *commandstring, char *mailsender) {

char subject[160] = ""; 
char mailaddress[160] = "";
char *charNL; 
char *char_smaleras; 	// smaleras = "<"
char *char_biggeras; 	// smaleras = ">"
unsigned int tempint;
char logentry[MAX_CHARS_PER_LINE_LOGFILE] = "";		// one line we write to logfile
	
	*commandstring = '\0';
	fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, mailfilep); // save line #1 from file in temp
	charNL = strchr(commandstring, '\n');	
	if (charNL == NULL) {	// we do not have a end of line marker
		write_logfile("Error: no end-of-line-marker in line of file with command!\n", LOG_ERROR);
		return MESSAGE_FILE_ERROR;
	}
	*charNL = '\0';  // terminate temp string
	strcpy(mailaddress, commandstring + 6);   // we get "From: ", but we have to get the value in angle-brackets:
	char_smaleras = strchr(mailaddress, '<');
	char_biggeras = strchr(mailaddress, '>');
	if ( (char_smaleras != NULL) && (char_biggeras != NULL)) {
		strncpy(mailsender, char_smaleras + 1, char_biggeras - char_smaleras - 1);
		mailsender[char_biggeras - char_smaleras - 1] = '\0'; 	// we terminate the string
		upcase_string(mailsender); 	// we change all chars to upcase
	} else { // we have either no '<' or no '>' in the mailaddress, we can stop with this file:
		sprintf(logentry, "Error: sending mailaddres is not valid: it must contain '<' and '>'!\n");
		write_logfile(logentry, LOG_WARNING);
		return MAIL_SENDER_NOTVALID;
	}
	if (check_mailsender(mailsender) == MAIL_SENDER_NOTVALID) { // we check if we have received a mail from a valid mailaddress
		sprintf(logentry, "Warning: invalid mailaddress in incoming mail: %s!\n", mailsender);
		write_logfile(logentry, LOG_WARNING);
		// now we send to all configured mails-addresses a warning message. First we count how many addresses we have:
		
		for (tempint = mail_address_00_count; tempint < mail_address_09_count; tempint++) {
			if (strlen(configvariables[tempint]) != 0 )  // if we have a mail-address configured we send to the mail-address the warning
				create_message_to_send(logentry, configvariables[mail_address_00_count + tempint], return_inbound_messagetype(), return_outbound_messagetype());  // we create a mail with a message to a mailaddress
		}	// end of for-loop

		// now we exit here because we have an invalid mail-address:
		return MAIL_SENDER_NOTVALID;
	} else { // the mailaddress is valid, we continue to read the whole incoming file:
		fgets(subject, MAX_CHARS_PER_LINE_CONFIGFILE, mailfilep); // save line #2 from file in temp
		strcpy(commandstring, subject + 9);	 // we skip "Subject: "	in the read line from the file
		if (strlen(commandstring) == 0) {	// if we have nothing in command-string:
			write_logfile("Error: no command found in 2nd line of mail!\n", LOG_ERROR);
			return MESSAGE_FILE_ERROR;
		}
		
		upcase_string(commandstring);	// we change all chars to upcase
	}

return NOERROR;
} // end of function get_command_from_mail ****************************************************************


//*****************************************************************************************************************
// get_command_from_telegram: extracts the command from a message we have received with telegram
// input: the file we have opened. The command will be written to commandstring)
// returncodes: 0 = no error
// 				else = any error
//*****************************************************************************************************************
unsigned int get_command_from_telegram(FILE *telegramfilep, char *commandstring, char *chatid) {

char name[160] = ""; 		// the name of the sender
char message[160] = "";		// the message itself
//char *getstringfromfile;
char *charNL; 
char *char_nl;			// new line
unsigned int tempint;
char logentry[MAX_CHARS_PER_LINE_LOGFILE] = "";		// one line we write to logfile

	*commandstring = '\0';
	fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, telegramfilep); // save line #1 from file in temp

	charNL = strchr(commandstring, '\n');		// we search for end of line marker
	if (charNL == NULL) {	// we do not have a end of line marker
		write_logfile("Error: no end-of-line-marker in line of file with command!\n", LOG_ERROR);
		return MESSAGE_FILE_ERROR;
	}
	*charNL = '\0';  // terminate temp string
	strcpy(name, commandstring + 6);	// we skip "Name: " in the line
	tempint = strcmp(name, "message_from_chat");
	if( tempint != 0)	{	// we do not have "message_from_chat" as name
		sprintf(logentry, "Error: Name of telegram sender is not valid: it \"must be message from chat\"!\n");
		write_logfile(logentry, LOG_WARNING);
		return TELEGRAM_SENDER_NOTVALID;
	}
	
	fgets(commandstring, MAX_CHARS_PER_LINE_CONFIGFILE, telegramfilep); // save line #1 from file in temp
	charNL = strchr(commandstring, '\n');		// we search for end of line marker
	if (charNL == NULL) {	// we do not have a end of line marker
		write_logfile("Error: no end-of-line-marker in line of file with command!\n", LOG_ERROR);
		return MESSAGE_FILE_ERROR;
	}
	*charNL = '\0';  // terminate temp string
	strcpy(chatid, commandstring + 9);	// we skip "Chat-ID: " in the line

	if (check_chat_id(chatid) == TELEGRAM_CHATID_NOTVALID) { // we check if we have received a telegram from a valid chat-id
		sprintf(logentry, "Warning: invalid chat-id in incoming telegram messages: %s!\n", chatid);
		write_logfile(logentry, LOG_WARNING);
		// now we send to all configured chat-ids a warning message. First we count how many chat-ids we have:
		
		for (tempint = telegram_chat_id_00_count; tempint < telegram_chat_id_09_count; tempint++) {
			if (strlen(configvariables[tempint]) != 0 )  // if we have a chat-id configured we send to the chat the warning
				create_message_to_send(logentry, configvariables[telegram_chat_id_00_count + tempint], return_inbound_messagetype(), return_outbound_messagetype());  // we create a message to the outbound channel
		}	// end of for-loop

		// now we exit here because we have an invalid chat-id:
		return TELEGRAM_CHATID_NOTVALID;
	} else { // the chat-id valid, we continue to read the whole incoming file:
		fgets(message, MAX_CHARS_PER_LINE_CONFIGFILE, telegramfilep); // save line #2 from file in temp
		strcpy(commandstring, message + 9);	 // we skip "Message: "	in the read line from the file
		char_nl = strchr(commandstring, '\n');
		if (char_nl != NULL) {	// if we have a new-line char than we terminate here the string
			*char_nl = '\0';	// we replace the newline-char with a '\0'
		}		
		upcase_string(commandstring);	// we change all chars to upcase
	}
	 
return NOERROR;
} // end of function get_command_from_telegram ****************************************************************

//*****************************************************************************************************************
// send_signal_keyboard_hit: checks if user hit a key on the keyboard
// input: process-id of own program
// returncodes: # of allowed parameters
//*****************************************************************************************************************

void send_signal_keyboard_hit(int *process_id) {

int readchar;
int buffered_charackter = 0;

	while (buffered_charackter == 0) {
		ioctl(STDIN_FILENO, FIONREAD, &buffered_charackter);	// we check if we have something in the buffer because user hit the keyboard
		usleep(1000*100);	// 1 second = 1000 * 1000 mikroseconds
	}

readchar = getchar(); // we get the sign from the buffer so that buffer is empty when program ends
if (readchar == 'c') readchar = 'c';	// this is needed to suppress a warning from compiler like "warning: unused variable readchar"

kill(*process_id, SIGTERM); 	// now we send the signal to end the program; Note: c-function raise did not work. And I don't know why.

} // end of check_keyboard_hit *****************************************************

//*****************************************************************************************************************
// garbage_collection: searches in all paths for processed files and searches for old files and deletes them.
// input: type of message we have as inbound, flag for first search true or false, date of the search
// returncodes: 0 = no error
// 				else = any error
//*****************************************************************************************************************
unsigned int garbage_collection(const int inbound_type, unsigned int *initial_garbage_collection, time_t *garbage_collection_current_time) {

time_t	timestamp_time; 	// the time we have right now
struct tm *timestamp_current_time, *timestamp_garbage_collection_time;
char date[10] = "\0";		// current date
char garbage_collection_date[10] = "\0";	// date of last garbage collection
DIR *processed_files;	// the directory for the processed files we have to search in for old files (and will be deleted)
struct dirent *directoryentry;	// struct to save detailed information of the content of the directory 
struct stat fileattributes; 	// struct to save the attributes of the the file
unsigned int processingflag = TRUE;	// flag that signs that we have to continue
char fullpath_processed_file[MAX_CHARS_FOR_PATH + MAX_CHARS_FOR_FILENAME] = "";	// the string to hold the complete filename
char temp_string[MAX_CHARS_FOR_PATH + MAX_CHARS_FOR_FILENAME] = "";
int tempint;
char logentry[MAX_CHARS_PER_LINE_LOGFILE] = "";
double age_in_days = 0;

	time(&timestamp_time);	// we get current time
	timestamp_current_time = localtime( &timestamp_time);		// we convert current time
	strftime(date, 11, "%d.%m.%Y", timestamp_current_time);	// we get only day, month and year of current time
	
	timestamp_garbage_collection_time = localtime(garbage_collection_current_time);
	strftime(garbage_collection_date, 11, "%d.%m.%Y", timestamp_garbage_collection_time);	// we get only day, month and year of the last call of this function
	if ( (strcmp(date, garbage_collection_date) != 0) || (*initial_garbage_collection == TRUE) )	{	// only if we have not the same date or we search for first time: we have to search files in processed files
		*initial_garbage_collection = FALSE;	// we set the flag for first search to FALSE
		
		write_logfile("Info: We start to collect garbage (delete files which are older than specified in ini-file).\n", LOG_INFO);
		memcpy(garbage_collection_current_time, &timestamp_time, sizeof(time_t));	// we copy back the # of seconds so that we can recognize if we had a change of day
		// we search now for old files to be deleted in the processed files for incoming messages:
		switch (inbound_type) {
			case MESSAGE_TYPE_SMS: processed_files = opendir(path_processed_SMS); strcpy(temp_string, path_processed_SMS); break;			
			case MESSAGE_TYPE_MAIL: processed_files = opendir(path_processed_mail); strcpy(temp_string, path_processed_mail); break;
			case MESSAGE_TYPE_TELEGRAM: processed_files = opendir(path_processed_telegram); strcpy(temp_string, path_processed_telegram); break;
		}	// end of switch
		
		while (processingflag == TRUE) {
			directoryentry = readdir(processed_files); 	// we read the directory
			if (directoryentry != 0) {	// we could read in the directory
				strcpy(fullpath_processed_file, temp_string);
				strcat(fullpath_processed_file, directoryentry->d_name);
				tempint = stat(fullpath_processed_file, &fileattributes); 	// we get attributes of the file we have
				tempint = S_ISREG(fileattributes.st_mode);				// we check if the file is a regular file: value unequal 0 means we have a regular file
				if (tempint != 0) {	// we have a file and not a directory
					#ifdef DEVSTAGE
					sprintf(logentry, "Info: found file for garbage collecting: %s\n", fullpath_processed_file);
					write_logfile(logentry, LOG_INFO);
					#endif
					age_in_days = difftime(timestamp_time, fileattributes.st_mtime) / 3600 / 24;	// we convert # of seconds to # of days
					if (age_in_days >= garbage_collection_days_long) {	// if age of the file is greater or equal the maximum age the file is deleted
						tempint = remove(fullpath_processed_file);
						if (tempint == 0) { // we could delete the file
							sprintf(logentry, "Info: File was deleted due to garbage collection: %s\n", fullpath_processed_file);
							write_logfile(logentry, LOG_INFO);
						} else {	// we could not delete the file, we will log this
							sprintf(logentry, "Error: File could not be deleted due to garbage collection: %s\n", fullpath_processed_file);
							write_logfile(logentry, LOG_ERROR);
						}
					} 
				}
			} else {	// we could not read in the diretory, we stop processing now
				processingflag = FALSE;
			} 
		}		
		
		closedir(processed_files);	// we close directory again
		
	}	// we have the same date for last search as current date or this is not first search, we do not search at all


return NOERROR; 	
}	// end of garbage_collection

//*****************************************************************************************************************
// start_processing_message: starts to process incoming SMS-messages and mails. Function loops until file with stop-message will be 
// found or signal to terminate will be received
// input: type of message (SMS, mail, telegram ...) for inbound and for outbound
// returncodes: 0 = no error
// 				else = any error
//*****************************************************************************************************************
int start_processing_message(const int inbound) {
		
DIR *incomingdirectoryp;

struct dirent *directoryentryp; 
struct stat fileattributes; 

FILE *messagefilep; // file which contains the message (SMS, mail, pushbullet ...)
FILE *processedmessagefilep; // file in "processed" directory
FILE *processidfilep; 	// file-handler which contains the process-id of our programm
int processid; 			// the ID of our processs

char fullpathincomingfile[MAX_CHARS_PER_LINE_CONFIGFILE]; 
char messagefilename[MAX_CHARS_PER_LINE_CONFIGFILE] = ""; 

char tempstring[MAX_CHARS_PER_LINE_CONFIGFILE];
int tempint; 
char *tempsms; // this pointer will be used to allocate memory for the copy-job of the sms-file
int commandfoundflag = FALSE; 	// indicates that we have found the parameter in the SMS in our list of commands
char logentry[MAX_CHARS_PER_LINE_LOGFILE] = "";		// one line we write to logfile
char online_result[200] = ""; 	// the result of the online-check is saved here
unsigned int loopcounter = 0; 	// counts how many times we were waiting for next message since start of program
unsigned int processed_sms_counter = 0; // counts # of processed SMS-files
unsigned int processed_mail_counter = 0; // counts # of processed mail-files
unsigned int processed_telegram_counter = 0; // counts # of processed telegram-files
pthread_t keyboardcheck_thread;	// pointer to the thread which checks if user hit the keyboard
char messagesender[50] = "";	// holds the sender like phone-# for sms and mailaddress
static struct termios temp_terminal;		// temporary terminal settings
static struct termios current_terminal;	// current terminal setting
char help_text[MAX_SIZE_HELP_TEXT] = "";	// this array will save the text which is send as a reply when we receive a help-command
char config_text[MAX_SIZE_CONFIG_TEXT] = "";	// this array will save the text which is send as a reply when we receive a config-command
unsigned int initial_garbage_collection_flag = TRUE; 	// flags if we do garbage collection for first time
time_t start_time;			// the time we started with this function
time_t garbage_collection_time;	// the time we have started with garbage collection

	if (read_config_file(config_filename) != NOERROR) return CONFIG_FILE_READ_ERROR;  // if anything went wrong with reading the configfile we stop now, error-messages are printed on screen
	
	time(&start_time);	// we get # of seconds since 1970
	garbage_collection_time = start_time;	// we set the time for garbage collection to the local time
	
	write_logfile("Info: we start processing now.\n", LOG_INFO);
	printf("Press any readable key (ESC or spacebar) to stop program.\n");
	processid = getpid();	// we get our process-ID so that we can write it to a file and start our keyboard thread
	if (daemon_flag == DAEMON_FALSE) {	// we start thread for listening to keyboard only when we do not run in daemon-mode
		tempint = pthread_create(&keyboardcheck_thread, NULL, (void *)&send_signal_keyboard_hit, &processid);
		if(tempint != 0) {	// we start the thread which listens to keyboard
			printf ("Error: Thread for listening to keyboard could not be started!\n");
			return KEYBOARD_THREAD_ERROR;
		}	
	}
	// now we save the process-id to a file. This is used to send to this process the "SIGTERM" signal
	switch (inbound) {
		case MESSAGE_TYPE_SMS: processidfilep = fopen(fullpath_smsfile_PID, "wb" ); break;  
		case MESSAGE_TYPE_MAIL: processidfilep = fopen(fullpath_mailfile_PID, "wb" ); break;  
		case MESSAGE_TYPE_TELEGRAM: processidfilep = fopen(fullpath_telegramfile_PID, "wb" ); break;  
	} // end of switch-loop

	if (processidfilep != 0) { 		
		fwrite(&processid, sizeof(processid), 1, processidfilep);
		fclose(processidfilep);
		sprintf(logentry, "Info: own process-id: %d\n", processid);												
		write_logfile(logentry, LOG_INFO);

		if (install_signal_function(SIGTERM, check_signal) == 1 ) write_logfile("Error: signalhandler for SIGTERM could not be installed!\n", LOG_ERROR); 
			else write_logfile("Info: signalhandler was installed for signal SIGTERM.\n", LOG_INFO);
		
		switch (inbound) {
			case MESSAGE_TYPE_SMS: incomingdirectoryp = opendir(configvariables[path_incoming_SMS_count]); // we open the directory where we expect the incoming SMS-textfile to be
									strcpy(fullpathincomingfile, configvariables[path_incoming_SMS_count]); 
									strcpy(tempstring, configvariables[path_processed_SMS_count]);
									tempstring[strlen(configvariables[path_processed_SMS_count])] = '\0';  // we have to replace the '\n' with '\0'
									break;  
			case MESSAGE_TYPE_MAIL: incomingdirectoryp = opendir(configvariables[path_incoming_mail_count]); // we open the directory where we expect the incoming mails to be 
									strcpy(fullpathincomingfile, configvariables[path_incoming_mail_count]); 
									strcpy(tempstring, configvariables[path_processed_mail_count]);
									tempstring[strlen(configvariables[path_processed_mail_count])] = '\0';  // we have to replace the '\n' with '\0'
									break;   
			case MESSAGE_TYPE_TELEGRAM: incomingdirectoryp = opendir(configvariables[path_incoming_telegram_count]); // we open the directory where we expect the incoming mails to be 
									strcpy(fullpathincomingfile, configvariables[path_incoming_telegram_count]); 
									strcpy(tempstring, configvariables[path_processed_telegram_count]);
									tempstring[strlen(configvariables[path_processed_telegram_count])] = '\0';  // we have to replace the '\n' with '\0'
									break;   	 
		} // end of switch-loop
		
		if ( incomingdirectoryp != 0) { // we can open the directory for reading in it
			if (daemon_flag == DAEMON_FALSE) {	// we change terminal-settings only when we do not run in daemon-mode:
				tempint = tcgetattr (STDIN_FILENO, &current_terminal); 	// we save terminal-settings in variable current_terminal

				if (tempint == -1) write_logfile("Error: terminalsettings could not be determined!\n", LOG_ERROR);
				temp_terminal = current_terminal;	// we save the current settings to temp-variable
				// we change some flags on the temporary terminalsettings:
				temp_terminal.c_iflag = temp_terminal.c_iflag & ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
				temp_terminal.c_oflag = temp_terminal.c_iflag & ~(OPOST);
				temp_terminal.c_cflag = temp_terminal.c_cflag & ~(CSIZE | PARENB);
				temp_terminal.c_lflag = temp_terminal.c_lflag & ~(ECHO|ICANON|IEXTEN|ISIG);
				temp_terminal.c_cflag = temp_terminal.c_cflag | CS8;
				temp_terminal.c_cc[VMIN]  = 1;
				temp_terminal.c_cc[VTIME] = 0;
				tempint = tcsetattr (STDIN_FILENO, TCSAFLUSH, &temp_terminal);	// we change now our terminal-settings
				if (tempint == -1) write_logfile("Error: terminalsettings could not be set to new values!\n", LOG_ERROR);
			}
			// now we setup the text for the help-message:
			tempint = setup_help_text(help_text);
			if (tempint != NOERROR) {
				sprintf(logentry, "Error: Helptext is too large for internal processing. Maximum allowed size is %i bytes! You have to increase size of internal buffer!\n", MAX_SIZE_HELP_TEXT);
				write_logfile( logentry, LOG_ERROR);
				processingflag = FALSE;	// we stop the program because we may get a segmentation fault
			}
			// now we setup the text for printing out the configuration
			tempint = setup_config_text(config_text);
			if (tempint != NOERROR) {
				sprintf(logentry, "Error: Configtext is too large for internal processing. Maximum allowed size is %i bytes! You have to increase size of internal buffer!\n", MAX_SIZE_CONFIG_TEXT);
				write_logfile( logentry, LOG_ERROR);
				processingflag = FALSE;	// we stop the program because we may get a segmentation fault
			}
			
			while (processingflag == TRUE) {

				directoryentryp = readdir(incomingdirectoryp); 
				
				if (directoryentryp != 0) {
					
					switch (inbound) { 
						case MESSAGE_TYPE_SMS: strcpy(fullpathincomingfile, configvariables[path_incoming_SMS_count]);
												strcpy(tempstring, configvariables[path_processed_SMS_count]); 
												tempstring[strlen(configvariables[path_processed_SMS_count])] = '\0';  // we have to replace the '\n' with '\0'
												break;
						case MESSAGE_TYPE_MAIL: strcpy(fullpathincomingfile, configvariables[path_incoming_mail_count]); 
												strcpy(tempstring, configvariables[path_processed_mail_count]); 
												tempstring[strlen(configvariables[path_processed_mail_count])] = '\0';  // we have to replace the '\n' with '\0'
												break;
						case MESSAGE_TYPE_TELEGRAM: strcpy(fullpathincomingfile, configvariables[path_incoming_telegram_count]); 
												strcpy(tempstring, configvariables[path_processed_telegram_count]); 
												tempstring[strlen(configvariables[path_processed_telegram_count])] = '\0';  // we have to replace the '\n' with '\0'
												break;
					} // end of switch-loop
					
					strcpy(messagefilename, directoryentryp->d_name);
					strcat(fullpathincomingfile, messagefilename);
					tempint = stat(fullpathincomingfile, &fileattributes); 	// we get attributes of the file we have
					tempint = S_ISREG(fileattributes.st_mode);				// we check if the file is a regular file: value unequal 0 means we have a regular file
					if (tempint != 0) {		// if the found file is a really a file and not something else		
						
						sprintf(logentry, "Info: incoming filename: %s\n", fullpathincomingfile);
						write_logfile(logentry, LOG_INFO);
						loopcounter = 0;
						messagefilep = fopen(fullpathincomingfile, "r"); 

						if (messagefilep == 0) {
							sprintf(logentry, "Error: file %s could not be opened for reading!\n", fullpathincomingfile);
							write_logfile(logentry, LOG_ERROR);
							fclose(messagefilep);
						} else { 	// we start to read the incoming file:
							strcat(tempstring, messagefilename);
							sprintf(logentry, "Info: new filename for saving processed file: %s\n", tempstring);
							write_logfile(logentry, LOG_INFO); 
					
							processedmessagefilep = fopen(tempstring, "w");  // we open file for saving content in new file
							if (processedmessagefilep == 0) { 
								sprintf(logentry, "Error: file %s could not be opened for saving content in new file!\n", tempstring);
								write_logfile(logentry, LOG_ERROR);
								// we stop now with processing:
								processingflag = FALSE;
							} else { 	// we will get the command from the message. Each message looks different: 
								switch (inbound) {
								case MESSAGE_TYPE_SMS: tempint = get_command_from_sms(messagefilep, tempstring, messagesender); ++processed_sms_counter;
									break;  
								case MESSAGE_TYPE_MAIL: tempint = get_command_from_mail(messagefilep, tempstring, messagesender); ++processed_mail_counter;
									break;   
								case MESSAGE_TYPE_TELEGRAM: tempint = get_command_from_telegram(messagefilep, tempstring, messagesender); ++processed_telegram_counter;					
									break;   	 
								} // end of switch-loop								
								
								if (tempint == NOERROR) { 	
									// now we have to find out which command we have. We compare at first step the found command with our build-in-commands:
									if ( strstr(tempstring, message2action_stop_command) != 0) { // we found the parameter, we can stop for searching
										sprintf(logentry, "Info: Command found in message-file: %s\n", tempstring);
										write_logfile(logentry, LOG_INFO);
										commandfoundflag = TRUE;
										processingflag = FALSE;
										write_logfile("Info: programm will be stopped now.\n", LOG_INFO);
										create_message_to_send("Info: programm will be stopped now.\n", messagesender, return_inbound_messagetype(), return_outbound_messagetype());
								    }	
								 
									if ( strstr(tempstring, reboot_pi_command) != 0) { // we found the parameter, we can stop for searching
										sprintf(logentry, "Info: Command found in message-file: %s\n", tempstring);
										write_logfile(logentry, LOG_INFO);
										commandfoundflag = TRUE;
										processingflag = FALSE;
										reboot_piflag = TRUE;
										write_logfile("Info: computer will be rebooted now.\n", LOG_INFO);
										create_message_to_send("Info: computer will be rebooted now.\n", messagesender, return_inbound_messagetype(), return_outbound_messagetype());
										sync();
										system("shutdown -r now");
								    }
								 
									if ( strstr(tempstring, shutdown_pi_command) != 0) { // we found the parameter, we can stop for searching
										sprintf(logentry, "Info: Command found in message-file: %s\n", tempstring);
										write_logfile(logentry, LOG_INFO);
										commandfoundflag = TRUE;
										processingflag = FALSE;
										shutdown_piflag = TRUE;
										write_logfile("Info: computer will be shutdown now.\n", LOG_INFO);
										create_message_to_send("Info: computer will be shutdownn now.\n", messagesender, return_inbound_messagetype(), return_outbound_messagetype());
										sync();
										system("shutdown -h now");
								    }

									if ( strstr(tempstring, status_pi_command) != 0) { // we found the parameter, we can stop for searching
										sprintf(logentry, "Info: Command found in message-file: %s\n", tempstring);
										write_logfile(logentry, LOG_INFO);
										commandfoundflag = TRUE;
										processingflag = TRUE;
										shutdown_piflag = FALSE;
										sprintf(logentry, "Info: # of processed telegrams: %d, SMS: %d, mails: %d, errors: %d, warnings: %d\n", processed_telegram_counter, processed_sms_counter, processed_mail_counter, errorcounter, warningcounter);
										write_logfile("Info: we send status back to sender.\n", LOG_INFO);
										write_logfile(logentry, LOG_INFO);
										create_message_to_send(logentry, messagesender, return_inbound_messagetype(), return_outbound_messagetype());  	// we send message to sender with the status : it depends on the outbound
								    }
								    
									if ( (strstr(tempstring, message2action_help_command) != 0) || (strstr(tempstring, message2action_questionmark_command) != 0) ) { // we found the parameter, we can stop for searching
										sprintf(logentry, "Info: Command found in message-file: %s\n", tempstring);
										write_logfile(logentry, LOG_INFO);
										commandfoundflag = TRUE;
										processingflag = TRUE;
										shutdown_piflag = FALSE;
										// we setup a text which contains help-info for the sender:
										create_message_to_send(help_text, messagesender, return_inbound_messagetype(), return_outbound_messagetype());  	// we send message to sender with the status : it depends on the outbound
								    }
								    
								    if ( strstr(tempstring, message2action_config_command) != 0) { // we found the parameter, we can stop for searching
										sprintf(logentry, "Info: Command found in message-file: %s\n", tempstring);
										write_logfile(logentry, LOG_INFO);
										commandfoundflag = TRUE;
										processingflag = TRUE;
										shutdown_piflag = FALSE;
										// we setup a text which contains help-info for the sender:
										create_message_to_send(config_text, messagesender, return_inbound_messagetype(), return_outbound_messagetype());  	// we send message to sender with the status : it depends on the outbound
								    }

									if  (strstr(tempstring, online_status_command_suffix) != 0) { // we found parameter for checking onlinestatus, we have to check now, if we have one of our WOL-addresses
										if ( strncmp("WOL", tempstring, 3) == 0) {	// if first 3 chars are equal to "WOL" we have a request for checking online-status:
										sprintf(logentry, "Info: Command found in message-file: %s\n", tempstring);
										write_logfile(logentry, LOG_INFO);
										commandfoundflag = TRUE;
										processingflag = TRUE;
										shutdown_piflag = FALSE;
										tempstring[5] = '\0'; // now we terminate the received command before the status-command
										// now we check the online-status of the device. We have to search the IP-address or hostname: 
										tempint = atoi(tempstring + 3);	// we convert the last 2 digits of the command in an integer
										// now we know which hostname or IP-address we have to check.
										if ( strlen( configvariables[tempint + hostname_00_count]) != 0) { // if we have a hostname then we check online status
											strcpy(tempstring, configvariables[tempint + hostname_00_count]); 
											tempint = check_onlinestatus(tempstring, online_result);
											} else 
												if ( strlen(configvariables[tempint + ip_address_00_count]) != 0) {		// if we have an IP-address then we check online status
													strcpy(tempstring, configvariables[tempint + ip_address_00_count]); 
													tempint = check_onlinestatus(tempstring, online_result);  
												} else { // we have an error: neither IP-address nor hostname is in ini-file
											 sprintf(logentry, "Error: neither IP-address nor hostname in ini-file for %s\n", tempstring);
											 write_logfile(logentry, LOG_ERROR);	
											 strcpy(online_result, logentry);	// we want to send the error-message back to sender
											}			
										// we send result back to outputchanel:
										create_message_to_send(online_result, messagesender, return_inbound_messagetype(), return_outbound_messagetype());	
										} // if (strncmp("WOL", tempstring, 3) == 0)
								    }	// if (strstr(online_status_command_suffix, tempstring) != 0)
		
		
								    
									if (commandfoundflag == FALSE) {  // we search for the command only when we did not recognize an internal command:
										for (tempint = messsagecommand_00_count; tempint < configparametercount; tempint++) {

											if ( strstr(tempstring, configparameters[tempint]) != 0) { // we found the parameter, we can stop for searching
											sprintf(logentry, "Info: Command number %d to be executed: %s\n", tempint, configparameters[tempint]);
												write_logfile(logentry, LOG_INFO);
												execute_command(tempint, messagesender); // now we execute the command we have found
												tempint = configparametercount; // we end here with the for-loop
												commandfoundflag = TRUE;
											
											} else { // we did not find the command 
												if ( (commandfoundflag == FALSE) && (tempint == configparametercount -1 )) {
													sprintf(logentry, "Error: unknown command in message-file: %s\n", tempstring);
													write_logfile(logentry, LOG_ERROR);
												}
											}
										}	 // end of for loop
									}	// end of (commandfoundflag == FALSE)
								} else { // end of if (tempint == NOERROR) 
									write_logfile("Error: no command found in messagefile!\n", LOG_ERROR);
								}
								// we copy complete file-content from the incoming directory to the processed directory, we don't need the file anymore: 
								#ifdef DEVSTAGE
									sprintf(logentry, "Info: Copy of received message-file starts now.\n");
									write_logfile(logentry, LOG_INFO);
								#endif
								
								rewind(messagefilep);
								
								#ifdef DEVSTAGE
									sprintf(logentry, "Info: filesize of message-file: %i\n", (int)fileattributes.st_size);
									write_logfile(logentry, LOG_INFO);
								#endif
								tempsms = malloc(fileattributes.st_size + 1); // we need memory to read the whole sms-file into
								sprintf(logentry, "Info: number of chars read: %d\n", fread(tempsms, 1, fileattributes.st_size, messagefilep)); 
								#ifdef DEVSTAGE
									write_logfile(logentry, LOG_INFO);
								#endif
								tempsms[fileattributes.st_size] = '\0';
								
								sprintf(logentry, "Info: number of chars written: %d\n", fwrite(tempsms, 1, fileattributes.st_size, processedmessagefilep));
								#ifdef DEVSTAGE
									write_logfile(logentry, LOG_INFO);
								#endif
								free(tempsms);
								sprintf(logentry, "Info: returncode for closing processed message-file: %d\n", fclose(processedmessagefilep)); 
								#ifdef DEVSTAGE
									write_logfile(logentry, LOG_INFO);
								#endif
								
								fclose(messagefilep);  // we close the sourcefile 
								// strcat(remove_command, sudo_rm_const); // we initialize the command-value
								// strcat(remove_command, fullpathincomingfile); // we add the filename of the incoming SMS-file to the rm-command
													
								if (remove(fullpathincomingfile) != 0) { // now we remove the source-file because we have a copy in processed-directory
									sprintf(logentry, "Error: received message-file could not be deleted!\n");
									write_logfile(logentry, LOG_ERROR); 
									} else 
									write_logfile("Info: received message-file was deleted successfully.\n", LOG_INFO); 
									
								commandfoundflag = FALSE; // we reset the flag
							
							} 	// end of if (processedsmsfilep == 0)
					
						}  // if (smsfilep == 0)
					} 	// end of if concerning "." and ".." in filename
				} else { // we are at end of directory
					closedir(incomingdirectoryp);
					
					switch (inbound) {
						case MESSAGE_TYPE_SMS : incomingdirectoryp = opendir(configvariables[path_incoming_SMS_count]); 
												sprintf(logentry, "Info: wait-cycle # %d, wait until next sms is received: %lu seconds\n", ++loopcounter, wait_time_next_sms_long);
												write_logfile( logentry, LOG_INFO); 
												sleep(wait_time_next_sms_long);	// we wait for some seconds until the next message comes in:
												break;  // we open the directory again	
						case MESSAGE_TYPE_MAIL : incomingdirectoryp = opendir(configvariables[path_incoming_mail_count]); 
												sprintf(logentry, "Info: wait-cycle # %d, wait until next mail is received: %lu seconds\n", ++loopcounter, wait_time_next_mail_long);
												write_logfile( logentry, LOG_INFO); 
												sleep(wait_time_next_mail_long);	// we wait for some seconds until the next message comes in:
												break; 	// we open the directory again	
						case MESSAGE_TYPE_TELEGRAM : incomingdirectoryp = opendir(configvariables[path_incoming_telegram_count]); 
														sprintf(logentry, "Info: wait-cycle # %d, wait until next telegram is received: %lu seconds\n", ++loopcounter, wait_time_next_telegram_long);
														write_logfile( logentry, LOG_INFO); 
														sleep(wait_time_next_telegram_long);	// we wait for some seconds until the next message comes in:
														break; 	// we open the directory again	
					} 	// end of switch-loop
				if (garbage_collection_days_long != 0)	// we start garbage collection only if we have a valid value for the # of days 	
					tempint = garbage_collection(inbound, &initial_garbage_collection_flag, &garbage_collection_time);	
					
				} // we finished waiting for next message
				
			}	// end of while-loop for checking processingflag == TRUE
		if (daemon_flag == DAEMON_FALSE) // only when we do not run in daemon-mode we set terminal to old mode:										
			tempint = tcsetattr (STDIN_FILENO, TCSANOW, &current_terminal);	

		} else { // we could not open the incoming directory for reading in it
			sprintf(logentry, "Error: directory %s could not be opened for reading incoming message-files!\n", configvariables[path_incoming_SMS_count]);
			write_logfile(logentry, LOG_ERROR);			
		}
	
		// we are at end of the loop and we will stop now, we have to delete the file containing the process-id:
		switch (inbound) {
			case MESSAGE_TYPE_SMS: tempint = remove(fullpath_smsfile_PID); strcpy(tempstring, fullpath_smsfile_PID); break;  
			case MESSAGE_TYPE_MAIL: tempint = remove(fullpath_mailfile_PID); strcpy(tempstring, fullpath_mailfile_PID); break;  
			case MESSAGE_TYPE_TELEGRAM: tempint = remove(fullpath_telegramfile_PID); strcpy(tempstring, fullpath_telegramfile_PID); break;  
		} // end of switch-loop

		if (tempint != 0) { 
			sprintf(logentry, "Error: file for saving process-ID could not be deleted: %s\n", tempstring);
			write_logfile(logentry, LOG_ERROR);
		}
	} else { // we could not create PID file for saving process-ID
		// printf("Error: PID-file could not be created!\n");
		strcpy(logentry, "Error: PID-file could not be created!\n");
		write_logfile(logentry, LOG_ERROR);		
	}
	write_logfile("Info: we stop processing now.\n", LOG_INFO);
	
	return NOERROR;
} // end of function start_processing_message ****************************************************************


//*****************************************************************************************************************
// stop_processing_message: stops processing incoming messages. It simply creates SMS-message which contains the command for stopping. 
// or in other words: it emulates that we received a message with the stop-command
// input: none
// returncodes: 0 = no error
// 				1 = any error
//*****************************************************************************************************************
int stop_processing_message(const unsigned int inboundtype) {
	
FILE *filep;   // file which contains the SMS we will fill with content
char stopfile[MAX_CHARS_PER_LINE_CONFIGFILE]; // filename of the file which will contain the stop message
char temp_line[100] = "";
char logentry[MAX_CHARS_PER_LINE_LOGFILE] = "";
FILE *processidfilep;
int processid = 0;
int killreturnvalue;  	// return value of function kill

if (read_config_file(config_filename) == NOERROR) {  // we read from config-file the cell phone number which is allowed to send SMS

// first we distinguish between our inbounds (sms, mail, ...) and then we distinguish between our stoppping method (file, signal)
switch (inboundtype) {
case MESSAGE_TYPE_SMS : 

if ( stopfile_flag == STOPFILE_TRUE ) {
	strcpy(stopfile, configvariables[path_incoming_SMS_count]);  // we get the path were the incoming SMS will be written in
	strcat(stopfile, configvariables[message2action_stop_filename_count]);
	sprintf(logentry, "Info: filename for STOP-SMS: %s\n", stopfile);
	write_logfile(logentry, LOG_INFO);
	filep = fopen(stopfile, "w+");
	if ( filep != 0 ) {
		return_cellphonenumber_from_inifile(logentry);	// we get first valid cell-phone-# from ini-file
		sprintf(temp_line, "From: %s", logentry);
		#ifdef DEVSTAGE
			sprintf(logentry, "Info: 1. line in STOP-SMS-file: %s\n", temp_line); 
			write_logfile(logentry, LOG_INFO);
		#endif
		
		fputs(temp_line, filep);
		// the following string is constant, none of the values will be read:
		fputs("\nFrom_TOA: DUMMY\nFrom_SMSC: DUMMY\nSent: DUMMY\nSubject: DUMMY\nModem: DUMMY\nIMSI: DUMMY\nReport: DUMMY\nAlphabet: DUMMY\nLength: DUMMY\n\n", filep);
		fputs(configvariables[message2action_stop_command_count], filep); // we take the stop-command from the ini-file
		#ifdef DEVSTAGE
			write_logfile("Info: dummy-line written to STOP-SMS-file.\n", LOG_INFO);
		#endif
		
		fclose(filep);
		write_logfile("Info: STOP-SMS-file successfully written and closed.\n", LOG_INFO);
	} else { // we could not open file for creating stop-sms-file
		sprintf(logentry, "Error: file %s could not be opened for writing STOP-SMS-file!\n", stopfile);
		write_logfile(logentry, LOG_ERROR);
	}
}	// end of if (stoppingmethod == STOPFILE_TRUE) 
	
	
if (stopsignal_flag == STOPSIGNAL_TRUE) {	
	processidfilep = fopen(fullpath_smsfile_PID, "rb" );
	if (processidfilep != NULL) { // we could open the pid-file
		fread(&processid, sizeof(processid), 1, processidfilep);
		fclose(processidfilep);
		sprintf(logentry, "Info: read process-ID: %d\n", processid);
		write_logfile(logentry, LOG_INFO);
		killreturnvalue = kill(processid, SIGTERM);
		if (killreturnvalue == 0) { // we could send signal to process
			sprintf(logentry, "Info: SIGTERM was send to process # %d\n", processid); 
			write_logfile(logentry, LOG_INFO);
		} else { // we could not send signal to process
			sprintf(logentry, "Error: SIGTERM could not be send to process # %d!\n", processid);
			write_logfile(logentry, LOG_ERROR);
		} 	
	} else {
		sprintf(logentry, "Error: file with process-ID could not be opened: %s!\n", fullpath_smsfile_PID);
		write_logfile(logentry, LOG_ERROR);
	}
}	// end of if if (stopsignal_flag == STOPSIGNAL_TRUE)


break; // break for case MESSAGE_TYPE_SMS

case MESSAGE_TYPE_MAIL : 

if ( stopfile_flag == STOPFILE_TRUE ) {
	strcpy(stopfile, configvariables[path_incoming_mail_count]);  // we get the path were the incoming mail will be written in
	strcat(stopfile, configvariables[message2action_stop_filename_count]);
	sprintf(logentry, "Info: filename for STOP-mail: %s\n", stopfile);
	write_logfile(logentry, LOG_INFO);
	filep = fopen(stopfile, "w+");
	if ( filep != 0 ) {
		return_mailaddress_from_inifile(logentry);	// we get first valid mailaddress from ini-file
		sprintf(temp_line, "From: <%s>", logentry);
		#ifdef DEVSTAGE
			sprintf(logentry, "Info: 1. line in STOP-mail-file: %s\n", temp_line); 
			write_logfile(logentry, LOG_INFO);
		#endif
		
		fputs(temp_line, filep);
		// the following string is constant, none of the values will be read:
		sprintf(logentry, "\nSubject: %s", configvariables[message2action_stop_command_count]); // we take the stop-command from the ini-file
		fputs(logentry, filep); 
		
		#ifdef DEVSTAGE
			write_logfile("Info: dummy-line written to STOP-mail-file.\n", LOG_INFO);
		#endif
		
		fclose(filep);
		write_logfile("Info: STOP-mail-file successfully written and closed.\n", LOG_INFO);
	} else { // we could not open file for creating stop-mail-file
		sprintf(logentry, "Error: file %s could not be opened for writing STOP-mail-file!\n", stopfile);
		write_logfile(logentry, LOG_ERROR);
	}	
}	// end of if (stoppingmethod == STOPFILE_TRUE) 
	
	
if (stopsignal_flag == STOPSIGNAL_TRUE) {	
	processidfilep = fopen(fullpath_mailfile_PID, "rb" );
	if (processidfilep != NULL) { // we could open the pid-file
		fread(&processid, sizeof(processid), 1, processidfilep);
		fclose(processidfilep);
		sprintf(logentry, "Info: read process-ID: %d\n", processid);
		write_logfile(logentry, LOG_INFO);
		killreturnvalue = kill(processid, SIGTERM);
		if (killreturnvalue == 0) { // we could send signal to process 
			sprintf(logentry, "Info: SIGTERM was send to process # %d\n", processid); 
			write_logfile(logentry, LOG_INFO);
		} else { // we could not send signal to process
			sprintf(logentry, "Error: SIGTERM could not be send to process # %d!\n", processid);
			write_logfile(logentry, LOG_ERROR);
		} 	
	} else {
		sprintf(logentry, "Error: file with process-ID could not be opened: %s!\n", fullpath_mailfile_PID);
		write_logfile(logentry, LOG_ERROR);
	}

}	// end of if if (stopsignal_flag == STOPSIGNAL_TRUE)
break;

case MESSAGE_TYPE_TELEGRAM :  
if ( stopfile_flag == STOPFILE_TRUE ) {
	strcpy(stopfile, configvariables[path_incoming_telegram_count]);  // we get the path were the incoming telegram will be written in
	strcat(stopfile, configvariables[message2action_stop_filename_count]);
	sprintf(logentry, "Info: filename for STOP-telegram: %s\n", stopfile);
	write_logfile(logentry, LOG_INFO);
	filep = fopen(stopfile, "w+");
	if ( filep != 0 ) {		
				
		fputs("Name: message_from_chat\nChat-ID: ", filep);
		return_chatid_from_inifile(logentry);	// we get first valid chat-id from ini-file
		// the following string is constant, none of the values will be read:
		sprintf(temp_line, "%s\nMessage: %s", logentry, configvariables[message2action_stop_command_count]); 
		fputs(temp_line, filep); 				
		fclose(filep);
		write_logfile("Info: STOP-telegram-file successfully written and closed.\n", LOG_INFO);
	} else { // we could not open file for creating stop-telegram-file
		sprintf(logentry, "Error: file %s could not be opened for writing STOP-telegram-file!\n", stopfile);
		write_logfile(logentry, LOG_ERROR);
	}	
}	// end of if (stoppingmethod == STOPFILE_TRUE) 
	
	
if (stopsignal_flag == STOPSIGNAL_TRUE) {	
	processidfilep = fopen(fullpath_telegramfile_PID, "rb" );
	if (processidfilep != NULL) { // we could open the pid-file
		fread(&processid, sizeof(processid), 1, processidfilep);
		fclose(processidfilep);
		sprintf(logentry, "Info: read process-ID: %d\n", processid);
		write_logfile(logentry, LOG_INFO);
		killreturnvalue = kill(processid, SIGTERM);
		if (killreturnvalue == 0) { // we could send signal to process 
			sprintf(logentry, "Info: SIGTERM was send to process # %d\n", processid); 
			write_logfile(logentry, LOG_INFO);
		} else { // we could not send signal to process
			sprintf(logentry, "Error: SIGTERM could not be send to process # %d!\n", processid);
			write_logfile(logentry, LOG_ERROR);
		} 	
	} else {
		sprintf(logentry, "Error: file with process-ID could not be opened: %s!\n", fullpath_telegramfile_PID);
		write_logfile(logentry, LOG_ERROR);
	}
}	// end of if if (stopsignal_flag == STOPSIGNAL_TRUE)

break;
} // end of switch
	
} else {	// we had a problem to read the configfile
	return CONFIG_FILE_READ_ERROR;
}
	
	return NOERROR;
} // end of function stop_processing_sms ****************************************************************


// *************************************
int signaltest() {
	
printf("Wir üben Signale...\n");
read_config_file(config_filename); 	// we have to read the configfile because we need the paths to some files

if (install_signal_function(SIGINT, check_signal) == 1 ) printf("Fehler beim installieren des Signalhandlers: SIGINT\n"); 
	else printf("Signalhandler wurde installiert.\n");

if (install_signal_function(SIGTERM, check_signal) == 1 ) printf("Fehler beim installieren des Signalhandlers: SIGTERM!\n"); 
	else printf("Signalhandler wurde installiert.\n");
	
	printf("Nun warten wir 30 Sekunden...\b\n");
	sleep(30);
	
return 0;	
} // end of signaltest	  


//*****************************************************************************************************************
// count_commandline_parameters: counts the number of commandlineparameters we have specified 
// input: none 
// returncodes: # of allowed parameters
//*****************************************************************************************************************
int count_commandline_parameters (void) {
int counter;
	for (counter = 0; ; counter++) {
		if (cmdline_parameters[counter] == NULL) return counter;
	}
} // end of count_commandline_parameters **************************************************************************


//*****************************************************************************************************************
// main: The program starts here 
// input: number of argument, the arguments itself 
// exitcodes: 0 for normal end; 1 for error
//*****************************************************************************************************************
int main(int argc, char **argv)
{
int cmdlinecounter = 0;
int valid_cmdline_counter = count_commandline_parameters(); 
int argumentcounter = 0;

	printf(version_of_program);
	if (argc > 1) { // we have commandline-parameters
		// we try to find out which parameters we have. First we go through list with only one 1 parameter:

		if (argc == 2) { // we have in total 1 parameter:
			if (strcmp(argv[1], "config") == 0) { cmdlinecounter = 1; print_configfile(); }
			if (strcmp(argv[1], "?") == 0) { cmdlinecounter = 1; printf("For documentation please see file message2action.pdf or message2action.txt.\n"); }
			if (cmdlinecounter == 0) printf("Error: invalid argument: %s\nCall program without parameters for help\n", argv[1]);
		}
		
		if (argc > 2) { // we have more than 2 parameters, now we step through all parameters: We take each parameter given to the program and check if the parameter is one of the allowed ones
			for (argumentcounter = 1; argumentcounter < argc; argumentcounter++) {
				valid_parameter_found_flag = FALSE;
				for (cmdlinecounter = 2; cmdlinecounter < valid_cmdline_counter; cmdlinecounter++) {
					if ( (strcmp(argv[argumentcounter], cmdline_parameters[cmdlinecounter]) == 0) || (strncmp(argv[argumentcounter], cmdline_parameters[cmdlinecounter], strlen(cmdline_parameters[cmdlinecounter])) == 0) ) { 
						#ifdef DEVSTAGE		
						// printf("Parameter found from commandline: %s\n", argv[argumentcounter]);
						#endif
						valid_parameter_found_flag = TRUE;
						switch(cmdlinecounter) {
							case stopsignal_counter : stopsignal_flag = STOPSIGNAL_TRUE; cmdlinecounter = valid_cmdline_counter; break;
							case stopfile_counter : stopfile_flag = STOPFILE_TRUE; cmdlinecounter = valid_cmdline_counter; break;
							case display_yes_counter : display_flag = DISPLAY_TRUE; cmdlinecounter = valid_cmdline_counter; break;
							case display_no_counter : display_flag = DISPLAY_FALSE; cmdlinecounter = valid_cmdline_counter; break;
							case log_yes_counter : logging_flag = LOGGING_TRUE; cmdlinecounter = valid_cmdline_counter; break;
							case log_no_counter : logging_flag = LOGGING_FALSE; cmdlinecounter = valid_cmdline_counter; break;
							case daemon_yes_counter : daemon_flag = DAEMON_TRUE; cmdlinecounter = valid_cmdline_counter; break;
							case daemon_no_counter : daemon_flag = DAEMON_FALSE; cmdlinecounter = valid_cmdline_counter; break;
							case outbound_sms_counter : outbound_sms_flag = TRUE; cmdlinecounter = valid_cmdline_counter; break;
							case outbound_mail_counter : outbound_mail_flag = TRUE; cmdlinecounter = valid_cmdline_counter; break;
							case outbound_telegram_counter : outbound_telegram_flag = TRUE;  cmdlinecounter = valid_cmdline_counter; break;
							case inbound_sms_counter : inbound_flag = TRUE; inbound_sms_flag = TRUE; cmdlinecounter = valid_cmdline_counter; break;
							case inbound_mail_counter : inbound_flag = TRUE; inbound_mail_flag = TRUE;  cmdlinecounter = valid_cmdline_counter; break;
							case inbound_telegram_counter : inbound_flag = TRUE; inbound_telegram_flag = TRUE; cmdlinecounter = valid_cmdline_counter; break;
							case inifile_counter : cmdlinecounter = valid_cmdline_counter; break;
							default: printf("Error: unknown parameter: %s\n", argv[argumentcounter]);	
						} 	// end of switch-loop
					} 	// end of if-clause
				}	// end of for-loop
				if (valid_parameter_found_flag == FALSE) { printf("Error: unknown parameter: %s\n", argv[argumentcounter]); exit(1);} 
			}	// end of for-loop	
			// now we know which parameters were given to the program. We have to decide now what has to be done:
			if (stopsignal_flag + stopfile_flag > 1) { printf("Error: more than 1 stoppingmethod is not allowed!\n"); exit(1);  }
			if (inbound_sms_flag + inbound_mail_flag + inbound_telegram_flag > 1)  { printf("Error: more than 1 inbound is not allowed!\n"); exit(1); }
			if (inbound_sms_flag + inbound_mail_flag + inbound_telegram_flag == 0)  { printf("Error: no inbound specified!\n"); exit(1); }
			if (outbound_sms_flag + outbound_mail_flag + outbound_telegram_flag > 1) { printf("Error: more than 1 outbound is not allowed!\n"); exit(1); }
			if (outbound_sms_flag + outbound_mail_flag + outbound_telegram_flag == 0) { // no outbound specified; maybe the cause is that we have to stop
				if ((stopsignal_flag == TRUE ) || (stopfile_flag == TRUE)) {
					stop_processing_message(return_inbound_messagetype()); 
					exit(0);
				} else {
					printf("Error: no outbound specified!\n"); 
					exit(1); 
				}
			}
						
			// now we start with processing:
			start_processing_message(return_inbound_messagetype()); 						
		}	// end of if (argc > 2)				
	} else { // we have no commandline-parameters, we print short help and do nothing

		
		printf("No parameters given, these parameters are possible:\n");
		printf("? : print a little help on screen.\n");
		printf("config : print content of ini-file and do checks on content %s\n", config_filename);
		printf("in=<sms|mail|telegram> out=<sms|mail|telegram> : use as inbound sms or mail or telegram and as outbound sms or mail or telegram without logging and in quiet-mode.\n");
		printf("in=<sms|mail|telegram> out=<sms|mail|telegram> display=y: choose inbound and outbound, display status on monitor.\n");
		printf("in=<sms|mail|telegram> out=<sms|mail|telegram> log=y: choose inound and outbound, enable logging.\n");
		printf("in=<sms|mail|telegram> out=<sms|mail|telegram> display=y log=y daemon=y: choose inbound and outbound, display status messages, run in daemon-mode.\n");
		printf("in=<sms|mail|telegram> stopsignal : send a signal to stop program listening on inbound <sms|mail|telegram>\n");
		printf("in=<sms|mail|telegram> stopfile : create a file which contains message to stop program listening on inbound <sms|mail|telegram>\n");
	}
	
	exit(0);
}
