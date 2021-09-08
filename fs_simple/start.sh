#!bin/sh

echo 'starting websocket server...'

./websocketd --port=3000 --staticdir=./canvas-c-socket-server/public ./nc 127.0.0.1 4000 & \
./canvas-c-socket-server/server && fg
