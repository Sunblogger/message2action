#!/bin/bash
# parser script which saves the mail in a text message, 13-july-2017
FilenameUniqueId=$(date +"%Y%m%d_%H%M%S_%N")
OutputFile="/home/pi/mail/"$FilenameUniqueId
OutputFileText="/home/pi/mail/"$FilenameUniqueId".txt"
echo "" > $OutputFileText
echo "" > $OutputFile
while read x
do 
#echo $x
echo $x >> $OutputFile
done
/bin/grep "From: " $OutputFile > $OutputFileText
/bin/grep "Subject: " $OutputFile >> $OutputFileText
chmod a+wrx $OutputFile
rm $OutputFile
chmod a+wr $OutputFileText

