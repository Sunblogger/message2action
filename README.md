# message2action
German version [below](#deutsche-version).
## What is message2action?
The program processes incoming messages such as SMS, emails, Telegram chat messages and responds to the content in the messages. Occasionally, this is also called "server control".

The program has been developed for the Raspberry Pi (hereinafter just "Pi") - or better: Raspbian OS - in C++. It has never been tested on other platforms.

The message sent to the Pi can contain commands such as "stop" or "status" and you can specify in a configuration file (ini file) what should happen when such a command is detected.

In the current version, message2action uses a file interface. In other words, files are read in directories and then these files are evaluated. It is not an online program.

The reason why I developed the program was the desire to start my PC but without the PC being accessible via the Internet. The original idea was to send an SMS to the Pi and it sends a WOL (wake on LAN) to the PC. So message2action avoids the need to use dyndns or port forwarding (in the router). And the PC is still not accessible via the Internet.

With this idea, I thought it would be a good idea to have a standardized interface on a Pi. The interface can also be used for other purposes, such as status queries from devices in the home network, such as temperature sensors or similar.

## What does the program do?
- It searches for text files in directories and reads them (mails, SMS and Telegram). Commands are searched for in the text files and then executed. The receipt of these text files must be done by other programs.
- The search in the directories is done in parallel for all input channels.
- Text files are generated (mails, SMS and Telegram) when a reply is to be sent back to the sender (for example, in response to a status request). Sending the answers to the recipient must be done by other programs.
- Information, warnings and errors are recorded in a log file so that the program can be traced afterwards. This is helpful in troubleshooting.
- The maximum size of the log file can be set.
- The incoming and outgoing text files are stored on the Pi and a maximum age can be specified for the files. Files that are older will be deleted.
- A maximum of 10 email addresses, 10 mobile numbers and 10 chat IDs for Telegram can be specified as contact partners.
- Alert messages are sent when a message is detected by an unknown sender. The unknown sender does not receive any information.
- A maximum of 10 commands (scripts, programs) can be configured.
- Internal commands are available to stop the program, restart or shut down the Pi.

## Documentation
Detailed documentation can be found here: [message2action_en.pdf](https://github.com/Sunblogger/message2action/blob/master/message2action_en.pdf) (english) or [message2action_de.pdf](https://github.com/Sunblogger/message2action/blob/master/message2action_de.pdf) (german).

## Sourcecode
The program is made up of several files, which are as follows:\
```message2action.cpp```: Contains '''main'''.\
```m2a_ini.cpp```: Class for processing the ini file.\
```m2a_ini.hpp```: Definition of the class for processing the ini file.\
```m2a_message.cpp```: Class for processing messages (SMS, Mail, Telegram).\
```m2a_message.hpp```: Definition of the class for processing the messages (SMS, Mail, Telegram).\
\
Some of the files are derived from the C++ files found at [cpp_sources](https://github.com/Sunblogger/cpp_sources). These files are the base classes.
```makefile```: The makefile to create the program.

## Create the program
Before the program can be created, these adjustments must be made and prerequisites must be met:\
The above-mentioned repository [cpp_sources](https://github.com/Sunblogger/cpp_sources) must be downloaded from the user's home directory with\
```git clone https://github.com/Sunblogger/cpp_sources```

To create the program, this must be entered: ```make```\
The parameter ```clean``` can be used to delete all object files and thus force the creation of all binaries.

<a name="deutsche-version"></a>
## Deutsche Version
## Was ist message2action?
Das Programm verarbeitet eingehende Nachrichten wie SMS, Emails, Telegram-Chat-Nachrichten und reagiert auf den Inhalt in den Nachrichten. Gelegentlich wird dies auch „Server-Steuerung“ genannt.
Das Programm ist für den Raspberry Pi (im folgenden nur „Pi“) - oder besser: Raspbian OS – in C++ entwickelt worden. Es wurde nie auf anderen Plattformen getestet. 
In der Nachricht, die an den Pi gesendet wird, können Kommandos wie „stop“ oder „status“ stehen und man kann in einer Konfigurationsdatei (ini-Datei) festlegen, was passieren soll, wenn ein solches Kommando erkannt wird.
In der aktuellen Fassung nutzt message2action eine Dateischnittstelle. In anderen Worten: es werden in Verzeichnissen Dateien gelesen und dann werden diese Dateien ausgewertet. Es ist kein Online-Programm.
Der Auslöser, warum ich das Programm entwickelt habe, war der Wunsch, meinen PC zu starten aber ohne, dass der PC über das Internet erreichbar ist. Die Ursprungsidee war, eine SMS an den Pi zu senden und dieser sendet ein WOL (wake on LAN) an den PC. Mit message2action wird also vermieden, dass Dyndns oder Portforwarding (im Router) verwendet werden muss. Und der PC ist nach wie vor nicht über das Internet erreichbar.
Mit dieser Idee dachte ich, es wäre eine gute Idee, ein standardisiertes Interface auf einem Pi zu haben. Das Interface kann auch für andere Zwecke genutzt werden wie zum Beispiel Statusanfragen von Geräten im Heimnetz wie Temperatursensoren oder ähnlich.

## Was kann das Programm?
- Es werden in Verzeichnissen nach Textdateien gesucht und diese auch gelesen (Mails, SMS und Telegram). In den Textdateien wird nach Kommandos gesucht und dann ausgeführt. Den Erhalt dieser Textdateien müssen andere Programme übernehmen.
- Das Suchen in den Verzeichnissen geschieht für alle Eingangskanäle parallel.
- Es werden Textdateien erzeugt (Mails, SMS und Telegram) wenn eine Antwort an den Absender zurückgesendet werden soll (zum Beispiel als Antwort auf eine Statusanfrage). Das Versenden der Antworten an den Empfänger müssen andere Programme übernehmen.
- In einer Logdatei werden Informationen, Warnungen und Fehler mitgeschrieben, damit man den Programmverlauf auch im Nachhinein nachvollziehen kann. Dies ist bei der Fehlersuche hilfreich.
- Die maximale Größe der Logdatei kann eingestellt werden.
- Die ein- und ausgegangenen Textdateien werden auf dem Pi gespeichert und es kann ein maximales Alter für die Dateien angegeben werden. Dateien, die älter sind, werden gelöscht.
- Maximal 10 Mailadressen, 10 Mobilnummern und 10 Chat-IDs für Telegram können als Kontaktpartner angegeben werden.
- Es werden Warnungs-Nachrichten versendet, wenn eine Nachricht von einem unbekannten Absender erkannt wird. Der unbekannte Absender erhält keine Information.
- Maximal 10 Kommandos (Skripte, Programme) können konfiguriert werden.
- Interne Kommandos sind vorhanden, um das Programm zu stoppen, den Pi neu zu starten oder auch herunterzufahren.

## Dokumentation
Eine ausführliche Dokumentation ist hier zu finden: [message2action_de.pdf](https://github.com/Sunblogger/message2action/blob/master/message2action_de.pdf) (deutsch) oder [message2action_en.pdf](https://github.com/Sunblogger/message2action/blob/master/message2action_en.pdf) (englisch).

## Quellcode
Das Programm ist aus mehreren Dateien aufgebaut, diese lauten wie folgt:\
```message2action.cpp```: Enthält ```main```.\
```m2a_ini.cpp```: Klasse für das Verarbeiten der ini-Datei.\
```m2a_ini.hpp```: Definition der Klasse für das Verarbeiten der ini-Datei.\
```m2a_message.cpp```: Klasse für das Verarbeiten der Nachrichten (SMS, Mail, Telegram).\
```m2a_message.hpp```: Definition der Klasse für das Verarbeiten der Nachrichten (SMS, Mail, Telegram).\
\
Manche der Dateien sind abgeleitet aus den C++-Dateien, die unter [cpp_sources](https://github.com/Sunblogger/cpp_sources) zu finden sind. Diese Dateien sind die Basisklassen.
```makefile```: Das Makefile zum Erstellen des Programmes.\

## Erstellen des Programmes
Bevor das Programm erstellt werden kann, müssen diese Anpassungen vorgenommen und Voraussetzungen erfüllt werden:\
Das oben erwähnte Repository [cpp_sources](https://github.com/Sunblogger/cpp_sources) muss im Home-Verzeichnis des eigenen Users heruntergeladen werden mit\
```git clone https://github.com/Sunblogger/cpp_sources```

Um das Programm zu erstellen, muss dies eingegeben werden: ```make```\
Mit dem Parameter ```clean``` können alle Objectdateien gelöscht werden und damit das Erstellen aller Binärdateien erzwungen werden.