# message2action
The program processes messages like SMS, mails and Telegram messages and reacts on the content of the message. Sometimes this functionality is called "server control". It is a program running on Raspberry Pi (following simply "Pi") or better Raspian and is developed in programming language C. The program was never tested on other platforms.  In the message you send to your Pi you can write commands like "stop" or "start" and you can specify in the configuration file (ini-file) of message2action what should be done when the message has been received by your Pi. General spoken message2action uses a file-based interface. Or in other words: it reads all files in specified directories and processes the files. It is not an online-program.
For further details see documentation message2action.pdf.

Compiling & linking
In order to comppile an link from source enter

gcc -g –Wall -Wextra -o message2action message2action.c -lpthread
