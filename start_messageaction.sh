#Start message2action and all services and timers
#date: 05.05.2021
#!/bin/bash
echo "We start getmails.timer ..."
sudo systemctl start getmails.timer

echo "We start sendmails.service ..."
sudo systemctl start sendmails.service

echo "We start telegram_check_updates.service ..."
sudo systemctl start telegram_check_updates.service

echo "We start send_telegram_to_bot.service ..."
sudo systemctl start send_telegram_to_bot.service

#echo "We start message2action.service ..."
#sudo systemctl start message2action.service

echo "Finished!"
