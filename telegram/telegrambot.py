#python script that reads the chat of your bot and saves message as a textfile. 14-july-2017

import sys
import time
import telepot
import os
from pathlib import Path

def handle(msg):
    chat_id = msg['chat']['id']
    command = msg['text']

    # print 'Got command from chat: %s' % command

    if command == 'ping':
       bot.sendMessage(chat_id, 'ping is received.')
    else: # we want to get the message only when it is not "ping":   
       os.system("/home/pi/message2action/telegram/telegramparser.sh message_from_chat " + str(chat_id) + " \"" + command + "\"")

bot = telepot.Bot('<insert your token of your bot here>')
bot.message_loop(handle)

print 'I am listening...'

stopfile = Path("/home/pi/message2action/telegram/stopfile")
if stopfile.is_file(): 
	os.remove("/home/pi/message2action/telegram/stopfile")
	print 'stopfile was deleted.'

while 1:
	stopfile = Path("/home/pi/message2action/telegram/stopfile")
	if stopfile.is_file(): 
#		 print "We have a stopfile."
		 break
#	else: 
#		 print "We do not have a stopfile." 
	time.sleep(2)
