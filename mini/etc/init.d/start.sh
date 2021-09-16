#!bin/sh

ip link set eth0 down
ip link set lo down 
ip addr add 10.0.2.15/24 dev eth0
ip link set lo up
ip link set eth0 up

./websocketd --port=3000 --staticdir=./server/public ./nc 127.0.0.1 4000 & \
./server/server && fg
