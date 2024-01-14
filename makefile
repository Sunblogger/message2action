# Makefile for message2action
# Date: 02.04.2023
CXXFLAGS = -g -I$(HOME)/cpp_sources/inifile -I$(HOME)/cpp_sources/logfile -I$(HOME)/cpp_sources/stopfile -I$(HOME)/message2action -Wall -Wextra -std=c++20

message2action:	message2action.cpp message2action.o $(HOME)/message2action/m2a_ini.o $(HOME)/cpp_sources/stopfile/stopfile.o $(HOME)/cpp_sources/logfile/logfile.o $(HOME)/cpp_sources/inifile/inifile.o $(HOME)/message2action/m2a_message.o	
	g++ $(CXXFLAGS) -pthread $(HOME)/message2action/m2a_ini.o $(HOME)/cpp_sources/stopfile/stopfile.o $(HOME)/cpp_sources/logfile/logfile.o $(HOME)/cpp_sources/inifile/inifile.o $(HOME)/message2action/m2a_message.o message2action.cpp -o message2action -lstdc++fs

m2a_message.o: $(HOME)/message2action/m2a_message.cpp $(HOME)/message2action/m2a_message.hpp
	g++ $(CXXFLAGS) -c $(HOME)/message2action/m2a_message.cpp -o $(HOME)/message2action/m2a_message.o -lstdc++fs
	
m2a_ini.o: 	$(HOME)/message2action/m2a_ini.cpp $(HOME)/message2action/m2a_ini.hpp
	g++ $(CXXFLAGS) -c $(HOME)/message2action/m2a_ini.cpp -o $(HOME)/message2action/m2a_ini.o -lstdc++fs
	
stopfile.o: $(HOME)/cpp_sources/stopfile/stopfile.cpp $(HOME)/cpp_sources/stopfile/stopfile.hpp
	g++ $(CXXFLAGS) -c $(HOME)/cpp_sources/stopfile/stopfile.cpp -o $(HOME)/cpp_sources/stopfile/stopfile.o -lstdc++fs

logfile.o: $(HOME)/cpp_sources/logfile/logfile.cpp $(HOME)/cpp_sources/logfile/logfile.hpp
	g++ $(CXXFLAGS) -c $(HOME)/cpp_sources/logfile/logfile.cpp -o $(HOME)/cpp_sources/logfile/logfile.o -lstdc++fs
	
inifile.o: $(HOME)/cpp_sources/inifile/inifile.cpp $(HOME)/cpp_sources/inifile/inifile.hpp
	g++ $(CXXFLAGS) -c $(HOME)/cpp_sources/inifile/inifile.cpp -o $(HOME)/cpp_sources/inifile/inifile.o -lstdc++fs
	
clean:
	/usr/bin/find $(HOME) -name '*.o' -delete
