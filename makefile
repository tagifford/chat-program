all: chat.cpp
			g++ -g -Wall -o chat chat.cpp

clean:
	 	$(RM) chat
