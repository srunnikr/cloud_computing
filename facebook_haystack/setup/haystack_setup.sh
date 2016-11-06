#!/bin/sh

num_server=1
num_store=1
num_cache=1
num_dir=1
i=0

# clean existing images and containers
echo "****************************"
echo "*  CLEANUP EXISTING STUFF  *"
echo "****************************"
while [ $i -lt $num_store ]
do
	name=$(printf "%s%d" "store" "$i")
	docker rm -f $name
	i=`expr $i + 1`
done

i=0
while [ $i -lt $num_cache ]
do
	name=$(printf "%s%d" "cache" "$i")
	docker rm -f $name
	i=`expr $i + 1`
done

i=0
while [ $i -lt $num_dir ]
do
	name=$(printf "%s%d" "dir" "$i")
	docker rm -f $name
	i=`expr $i + 1`
done

i=0
while [ $i -lt $num_server ]
do
	name=$(printf "%s%d" "server" "$i")
	docker rm -f $name
	i=`expr $i + 1`
done

docker rm -f balancer

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
i=0
while [ $i -lt $num_store ]
do
	name=$(printf "%s%d" "store" "$i")
	docker run -itd --name $name haystack_store
	i=`expr $i + 1`
done

i=0
while [ $i -lt $num_cache ]
do
	name=$(printf "%s%d" "cache" "$i")
	docker run -itd --name $name haystack_cache -m 128	# 128MB cache
	i=`expr $i + 1`
done

i=0
while [ $i -lt $num_dir ]
do
	name=$(printf "%s%d" "dir" "$i")
	docker run -itd --name $name haystack_dir
	i=`expr $i + 1`
done

i=0
while [ $i -lt $num_server ]
do
	name=$(printf "%s%d" "server" "$i")
	docker run -p 8080:8080 -itd --name $name web_server # use the ip address you get from "docker-machine ip default" (ip_address:8080)
	i=`expr $i + 1`
done

docker run -itd --name balancer load_balancer
