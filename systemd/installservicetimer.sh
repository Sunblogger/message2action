# Releasedate: 04-july-2017
# We install the service- and timer-files for message2action, getmails and sendmails

#!/bin/bash
echo "stop timer getmails ..."
sudo systemctl stop getmails.timer

echo "stop services: message2action, getmails, sendmails ..."
sudo systemctl stop message2action.service
sudo systemctl stop getmails.service
sudo systemctl stop sendmails.service
sudo systemctl stop send_telegram_to_bot.service
sudo systemctl stop telegram-bot.service

echo "disable services: message2action, getmails, sendmails ..."
sudo systemctl disable message2action.service
sudo systemctl disable getmails.service
sudo systemctl disable sendmails.service
sudo systemctl disable send_telegram_to_bot.service

echo "disable timer: getmails"
sudo systemctl disable getmails.timer

echo "copy service-files to /etc/systemd/system: message2action, getmails, sendmails ..."
sudo cp /home/pi/message2action/systemd/message2action.service /etc/systemd/system
sudo cp /home/pi/message2action/systemd/getmails.service /etc/systemd/system
sudo cp /home/pi/message2action/systemd/sendmails.service /etc/systemd/system
sudo cp /home/pi/message2action/systemd/send_telegram_to_bot.service /etc/systemd/system
sudo cp /home/pi/message2action/systemd/telegram-bot.service /etc/systemd/system

echo "copy timer-files to /etc/systemd/system: getmails ..."
sudo cp /home/pi/message2action/systemd/getmails.timer /etc/systemd/system

#echo "enable services for message2action, getmails, sendmails..."
sudo systemctl enable message2action.service
#sudo systemctl enable sendmails.service
#sudo systemctl enable getmails.service
sudo systemctl enable send_telegram_to_bot.service
sudo systemctl enable telegram-bot.service

#echo "enable timer for getmails ..."
#sudo systemctl enable getmails.timer
