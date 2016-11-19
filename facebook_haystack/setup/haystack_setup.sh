#!/bin/bash

num_server=1
num_store=1
num_cache=1
num_dir=1

SUBNET_BASE="192.168.%d.%d"
SUBNET_MASK=16

MGMT_IP_BLOCK=0         # IP Range: 192.168.0.X
STORE_IP_BLOCK=1        # IP Range: 192.168.1.X
CACHE_IP_BLOCK=2        # IP Range: 192.168.2.X
DIR_IP_BLOCK=3          # IP Range: 192.168.3.X
SERVER_IP_BLOCK=4       # IP Range: 192.168.4.X
BALANCER_IP_BLOCK=5     # IP Range: 192.168.5.X
CACHE_SVR_IP_BLOCK=6	# IP Range: 192.168.6.X

STORE_BASE_DIR='./haystack_store'
CACHE_BASE_DIR='./haystack_cache'
CACHE_SERVER_DIR='./haystack_cache_server'
DIR_BASE_DIR='./haystack_dir'
SERVER_BASE_DIR='./web_server'
BALANCER_BASE_DIR='./load_balancer'

remove () {
    # clean existing images and containers
    echo "****************************"
    echo "*  CLEANUP EXISTING STUFF  *"
    echo "****************************"

    i=0
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
	docker rm -f cache_server

    docker rmi -f haystack_store
    docker rmi -f haystack_cache 
    docker rmi -f haystack_dir
    docker rmi -f web_server
    docker rmi -f load_balancer
	docker rmi -f haystack_cache_server # takes too long, comment this after the first build

    docker network rm haynet
    
    rm $BALANCER_BASE_DIR/nginx.conf
}


build () {
    # create custom network off 'bridge'
    echo "********************************"
    echo "*   CREATE A CUSTOM NETWORK    *"
    echo "********************************"

    subnet=$(printf "$SUBNET_BASE/%d" 0 0 $SUBNET_MASK)
    gateway=$(printf "$SUBNET_BASE" $MGMT_IP_BLOCK 1)
    docker network create -d bridge --subnet=$subnet --gateway=$gateway haynet

    # build all the images
    echo "*******************************"
    echo "*    BUILD ALL THE IMAGES     *"
    echo "*******************************"
    docker build -t haystack_store $STORE_BASE_DIR
    docker build -t haystack_cache $CACHE_BASE_DIR
	docker build -t haystack_cache_server $CACHE_SERVER_DIR
    docker build -t haystack_dir $DIR_BASE_DIR
    docker build -t web_server $SERVER_BASE_DIR
    #docker build -t load_balancer $BALANCER_BASE_DIR #defer load balancer image creation for now until web server containers are not created

    # Start all the container instances
    echo "**************************************"
    echo "*    START ALL CONTAINER INSTANCES   *"
    echo "**************************************"
    i=0
    while [ $i -lt $num_store ]
    do
        name=$(printf "%s%d" "store" "$i")
        ip=$(printf "$SUBNET_BASE" $STORE_IP_BLOCK $(expr $i + 1))
        docker run -itd --network=haynet --ip=$ip --name $name haystack_store
        i=`expr $i + 1`
    done
	
	i=0
    while [ $i -lt $num_dir ]
    do
        name=$(printf "%s%d" "dir" "$i")
        ip=$(printf "$SUBNET_BASE" $DIR_IP_BLOCK $(expr $i + 1))
        docker run -itd --network=haynet --ip=$ip --name $name haystack_dir
        i=`expr $i + 1`
    done

    # Temporary workaround (no longer needed, see connection retry in webserver/server.js)
    #echo "Waiting for Haystack Store & directory to initialize..."
    #sleep 120

	ip=$(printf "$SUBNET_BASE" $CACHE_SVR_IP_BLOCK 1)
	docker run -itd --network=haynet --ip=$ip --name cache_server haystack_cache_server

    i=0
    while [ $i -lt $num_cache ]
    do
        name=$(printf "%s%d" "cache" "$i")
        ip=$(printf "$SUBNET_BASE" $CACHE_IP_BLOCK $(expr $i + 1))
        docker run -itd --network=haynet --ip=$ip --name $name haystack_cache -m 128	# 128MB cache
        i=`expr $i + 1`
    done

    i=0
    web_server_ips=( )
    while [ $i -lt $num_server ]
    do
        name=$(printf "%s%d" "server" "$i")
        ip=$(printf "$SUBNET_BASE" $SERVER_IP_BLOCK $(expr $i + 1))
        docker run -itd --network=haynet --ip=$ip --name $name web_server
        web_server_ips=(${web_server_ips[@]} $ip)
        i=`expr $i + 1`
    done

    # prepare nginx.conf with web server ips
    server_list_pattern="/#WEB_SERVER_LIST/a\\"
    for server_ip in ${web_server_ips[@]}; do
        server_list_pattern="${server_list_pattern}\tserver ${server_ip}:8080;\n"
    done
    sed "$server_list_pattern" $BALANCER_BASE_DIR/nginx.conf.template > $BALANCER_BASE_DIR/nginx.conf
    
    #Finally build load_balancer image
    docker build -t load_balancer $BALANCER_BASE_DIR

    ip=$(printf "$SUBNET_BASE" $BALANCER_IP_BLOCK 1)
    docker run -itd --network=haynet --ip=$ip --name balancer load_balancer

    printf "\nNOTE: Use Load Balancer IP $ip to fetch photos. Eg: curl http://$ip/\n\n"
}


usage () {
    echo "$1 [build | remove]"
    return 0
}



case $1 in
    build)
        build
        ;;

    remove)
        remove
        ;;

    *)
        usage $0
        exit 1
esac

exit 0
