#!/bin/bash
# Parserscript for creating text files when Telegram messages are received, date: 10.06.2017
FilenameUniqueId=$(date +"%Y%m%d_%H%M%S_%N")
OutputFileText="/home/pi/telegram/"$FilenameUniqueId".txt"

echo "Name: "$1 > $OutputFileText 

echo "Chat-ID: "$2 >> $OutputFileText 

echo "Message: "$3 >> $OutputFileText 

chmod a+wr $OutputFileText
