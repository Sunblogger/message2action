#Stop message2action and all services and timers
#date: 05.05.2021
#!/bin/bash
echo "we stop getmails.timer ..."
sudo systemctl stop getmails.timer

echo "We stop sendmails.service ..."
sudo systemctl stop sendmails.service

echo "We stop telegram_check_updates.service ..."
sudo systemctl stop telegram_check_updates.service

echo "We stop send_telegram_to_bot.service ..."
sudo systemctl stop send_telegram_to_bot.service

echo "we stop message2action.service ..."
sudo systemctl stop message2action.service

echo "Finished!"
