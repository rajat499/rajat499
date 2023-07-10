DEBUG_OPTS=-O3
INSTALL_DIR=/tmp

all: ${INSTALL_DIR}/tcpclient ${INSTALL_DIR}/tcpserver 

clean:
	rm -f ${INSTALL_DIR}/tcpclient ${INSTALL_DIR}/tcpserver 

$(INSTALL_DIR)/tcpclient: tcpclient.cpp
	g++ $(DEBUG_OPTS) tcpclient.cpp -o ${INSTALL_DIR}/tcpclient -lpthread

${INSTALL_DIR}/tcpserver: tcpserver.cpp
	g++ $(DEBUG_OPTS) tcpserver.cpp -o ${INSTALL_DIR}/tcpserver -lpthread

