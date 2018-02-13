# send_message_to_bot.sh : script for sending out all message which were setup by message2action to a Telegram bot. Release 0.2 from 06.01.2018
#!/bin/bash
commandlineparameter=$1
stopfile=/tmp/sendmessagetobot.stop	# This is the file which will be created when this script is called with parameter "stop". The existence of the file is used as a signal to tell the script that it has to stop
processingflag=1 	# 0 = false, we have to stop; 1 = true, we have to continue (this is default)
path_to_transport_program=/usr/bin/curl # the path to the program which is used to transfer messages (mail, curl, telegram-client ...)
path_to_messages=/home/pi/telegram/outgoing_to_bot	# the path were the files are saved which were created by message2action; they contain the mails, messages for Telegram. This path is specified in ini-file of message2action with parameter path_outgoing_mail

send_to_Telegrambot() {
        $path_to_transport_program \
        -X POST \
        https://api.telegram.org/bot$token/sendMessage \
        -d text="$message" \
        -d chat_id=$messagerecipient
}

if [ "$commandlineparameter" = "" ] 
then 
	echo "No parameter given, allowed parameters:"
	echo "start : starts sending out all messages to a Telegram bot"
	echo "stop: stops the other instance of this script" 
	echo "We stop now."
fi

if [ "$commandlineparameter" = "stop" ] 	# we have to stop and we will create a file for sending a signal to stop:
then 
	echo "stop" > $stopfile
	echo "We created stopfile and we stop now."
fi

if [ "$commandlineparameter" = "start" ] 
then # we have to start processing.
	# set -x	# this helps for debugging
	while [ $processingflag = 1 ]	# we will continue with processing as long as this flag is true
	do
		if [ -f $stopfile ]	# if the stopfile does exist we have to stop
		then
			echo "We are stopping now."
			processingflag=0	# we set processingflag to false	
			rm $stopfile		# the stopfile has to be deleted in order to clear the flag
		else
			for file in $path_to_messages/*.* ; do	# we step through all files in the mentioned directory
				full_path_to_messages=$path_to_messages/*.*
				#echo Path to messages: $full_path_to_messages
				if [ "$file" != "$full_path_to_messages" ]	# if directory is not empty
				then	# now we extract from the file the content:
					completeline=$(grep Chat-id: $file) 
					#echo $completeline
					messagerecipient=${completeline:9}
					echo "Chat-id:" $messagerecipient
					completeline=$(grep Token: $file)
					token=${completeline:7}
					echo "Token: "$token
					completeline=$(grep "Message to bot" $file -A1024)	# we need all lines after the matching because we can have more than 1 line to send to the chat
					message=${completeline:15}
					echo "Message to bot: " $message
					send_to_Telegrambot
					rm -f $file	# we delete the file
				fi
			done # 	for all files in outgoing directory
		fi
		sleep 1	# we sleep 1 second in order to save processor-time before we search next time for files
	done	# while [ $processingflag = 1 ]
	
fi

# END OF SCRIPT
