#!/bin/sh

# clean existing images and containers
echo "****************************"
echo "*  CLEANUP EXISTING STUFF  *"
echo "****************************"
docker rm -f store
docker rm -f cache
docker rm -f dir
docker rm -f balancer
docker rm -f server1

docker rmi -f haystack_store
docker rmi -f haystack_cache
docker rmi -f haystack_dir
docker rmi -f load_balancer
docker rmi -f web_server

docker network rm haynet

# create custom network off 'bridge'
echo "********************************"
echo "*   CREATE A CUSTOM NETWORK    *"
echo "********************************"

docker network create -d bridge haynet

# build all the images
echo "*******************************"
echo "*    BUILD ALL THE IMAGES     *"
echo "*******************************"
docker build -t haystack_store ./haystack_store/
docker build -t haystack_cache ./haystack_cache/
docker build -t haystack_dir ./haystack_dir/
docker build -t load_balancer ./load_balancer/
docker build -t web_server ./web_server/

# Start all the container instances
echo "**************************************"
echo "*    START ALL CONTAINER INSTANCES   *"
echo "**************************************"
docker run -itd --name store haystack_store
docker run -itd --name cache haystack_cache
docker run -itd --name dir haystack_dir
docker run -p 8080:8080 -itd --name server1 web_server # use the ip address you get from "docker-machine ip default" (ip_address:8080)
docker run -itd --name balancer load_balancer

