# sendmails.sh : script for sending out all mails which were setup by message2action. Release 0.4 from 23.12.2023
#!/bin/bash
commandlineparameter=$1
stopfile=/tmp/sendmails.stop		# This is the file which will be created when this script is called with parameter "stop". The existence of the file is used as a signal to tell the script that it has to stop
processingflag=1 			# 0 = false, we have to stop; 1 = true, we have to continue (this is default)
pathtomsmtp=/usr/bin/msmtp		# The path to the program ssmtp (is used to send mails out)
pathtomails=/home/marcus/mail/outgoing	# the path were the files are saved which were created by message2action; they contain the mailcontent. This path is specified in ini-file of message2action with parameter path_outgoing_mail

#echo "Given parameter in commandline:" $commandlineparameter

if [ "$commandlineparameter" = "" ] 
then 
	echo "No parameter given, allowed parameters:"
	echo "start : starts sending out all mails"
	echo "stop: stops the other instance of sendmails.sh" 
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
			for file in $pathtomails/*.* ; do	# we step through all files in the mentioned directory
				# echo "Name of file with mailcontent: $file"
				fullpathtomails=$pathtomails/*.*
				# echo "Path to mails: $fullpathtomails"
				if [ "$file" != "$fullpathtomails" ]	# if directory is not empty
				then
					completeline=$(grep To: $file) 
					# echo $completeline
					mailrecipient=${completeline:3}	# here we have the mailaddress to which we want to send the mail
					# echo $mailrecipient
					$pathtomsmtp --tls=on --debug --from legacyhardware@t-online.de -t $mailrecipient < $file	
					# echo We delete file $file.
					rm -f $file
				fi
			done 
		fi
		sleep 1	# we sleep 1 second in order to save processor-time
	done	# while [ $processingflag = 1 ]	
fi

# END OF SCRIPT
