#!/bin/bash

num_store=2
num_directory=1
num_cache=1
num_cache_server=1
num_web_server=2

SUBNET_BASE="192.168.%d.%d"
SUBNET_MASK=16

MGMT_IP_BLOCK=0         # IP Range: 192.168.0.X
STORE_IP_BLOCK=1        # IP Range: 192.168.1.X
CACHE_IP_BLOCK=2        # IP Range: 192.168.2.X
DIR_IP_BLOCK=3          # IP Range: 192.168.3.X
WEB_SERVER_IP_BLOCK=4   # IP Range: 192.168.4.X
BALANCER_IP_BLOCK=5     # IP Range: 192.168.5.X
CACHE_SERVER_IP_BLOCK=6	# IP Range: 192.168.6.X

STORE_BASE_DIR='./haystack_store'
CACHE_BASE_DIR='./haystack_cache'
CACHE_SERVER_DIR='./haystack_cache_server'
DIR_BASE_DIR='./haystack_dir'
SERVER_BASE_DIR='./web_server'
BALANCER_BASE_DIR='./load_balancer'

CACHE_SERVER_PORT=8080
WEB_SERVER_PORT=8080

remove () {
    # clean existing images and containers
    echo "****************************"
    echo "*  CLEANUP EXISTING STUFF  *"
    echo "****************************"

    i=0
    while [ $i -lt $num_store ]
    do
        name=$(printf "%s-%d" "photo-store" "$i")
        #docker rm -f $name
        i=`expr $i + 1`
    done

    i=0
    while [ $i -lt $num_cache ]
    do
        name=$(printf "%s-%d" "haystack-cache" "$i")
        docker rm -f $name
        i=`expr $i + 1`
    done

    i=0
    while [ $i -lt $num_cache_server ]
    do
        name=$(printf "%s-%d" "haystack-cache-server" "$i")
        docker rm -f $name
        i=`expr $i + 1`
    done

    i=0
    while [ $i -lt $num_directory ]
    do
        name=$(printf "%s-%d" "directory" "$i")
        #docker rm -f $name
        i=`expr $i + 1`
    done

    i=0
    while [ $i -lt $num_web_server ]
    do
        name=$(printf "%s-%d" "web-server" "$i")
        docker rm -f $name
        i=`expr $i + 1`
    done

    docker rm -f "cache_load_balancer"
    docker rm -f "web_load_balancer"

    #docker rmi -f haystack_store
    docker rmi -f haystack_cache
    #docker rmi -f haystack_directory
    docker rmi -f web_server
    #docker rmi -f web_load_balancer
    #docker rmi -f cache_load_balancer
				#docker rmi -f haystack_cache_server # takes too long, comment this after the first build

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
    docker build -t haystack_directory $DIR_BASE_DIR
    docker build -t web_server $SERVER_BASE_DIR

    #defer load balancer image creation for now until web server containers are not created
    #docker build -t web_load_balancer $BALANCER_BASE_DIR
    #docker build -t cache_load_balancer $BALANCER_BASE_DIR

    # Start all the container instances
    echo "**************************************"
    echo "*    START ALL CONTAINER INSTANCES   *"
    echo "**************************************"

    #############################################
    #               Haystack Store              #
    #############################################

    # initiate Cassandra instances for photo stores
    store_ips=( )
    i=0
    while [ $i -lt $num_store ]
    do
        name=$(printf "%s-%d" "photo-store" "$i")
        ip=$(printf "$SUBNET_BASE" $STORE_IP_BLOCK $(expr $i + 1))
        docker run -itd --net=haynet --ip=$ip --name $name haystack_store
        store_ips=(${store_ips[@]} $ip)
        i=`expr $i + 1`
    done

    # Temporary workaround (no longer needed, see connection retry in webserver/server.js)
    #echo "Waiting for Haystack Store to initialize..."
    #sleep 120


    #############################################
    #               Haystack Cache              #
    #############################################

    # initiate Memcache instances for photo caching
    i=0
    cache_ips=( )
    while [ $i -lt $num_cache ]
    do
        name=$(printf "%s-%d" "haystack-cache" "$i")
        ip=$(printf "$SUBNET_BASE" $CACHE_IP_BLOCK $(expr $i + 1))
        docker run -itd --network=haynet --ip=$ip --name $name haystack_cache -m 128 "-I 32M"	# 128MB cache with 32MB max object size
        cache_ips=(${cache_ips[@]} $ip)
        i=`expr $i + 1`
    done


    # initiate Haystack Cache's web server instances
    i=0
    cache_server_ips=( )
    while [ $i -lt $num_cache_server ]
    do
        name=$(printf "%s-%d" "haystack-cache-server" "$i")
	    ip=$(printf "$SUBNET_BASE" $CACHE_SERVER_IP_BLOCK 1)

        cache_env="${cache_ips[0]}"
        for cache_ip in ${cache_ips[@]:1}; do
            cache_env=`echo "$cache_env,$cache_ip"`
        done

        stores_env="${store_ips[0]}"
        for store_ip in ${store_ips[@]:1}; do
            stores_env=`echo "$stores_env,$store_ip"`
        done

	    docker run -itd --net=haynet --ip=$ip --env CACHE_IPS="$cache_env" --env STORE_IPS="$stores_env" --name $name haystack_cache_server
        cache_server_ips=(${cache_server_ips[@]} $ip)
        i=`expr $i + 1`
    done


    # initiate Nginx instance for load balancing Cache Servers

    # prepare nginx.conf with cache server ips
    server_list_pattern="/#WEB_SERVER_LIST/a\\"
    for server_ip in ${cache_server_ips[@]}; do
        server_list_pattern="${server_list_pattern}\tserver ${server_ip}:$CACHE_SERVER_PORT;\n"
    done
    sed "$server_list_pattern" $BALANCER_BASE_DIR/nginx.conf.template > $BALANCER_BASE_DIR/nginx.conf

    #Finally build load_balancer image
    docker build -t cache_load_balancer $BALANCER_BASE_DIR

    ip=$(printf "$SUBNET_BASE" $BALANCER_IP_BLOCK 1)
    docker run -itd --net=haynet --ip=$ip --name "cache_load_balancer" cache_load_balancer
    cache_load_balancer_ip=$ip



    #############################################
    #           Haystack Directory              #
    #############################################

    # initiate Cassandra instances for Directory entry stores
    directory_ips=( )
	i=0
    while [ $i -lt $num_directory ]
        do
            name=$(printf "%s-%d" "directory" "$i")
            ip=$(printf "$SUBNET_BASE" $DIR_IP_BLOCK $(expr $i + 1))
            docker run -itd --net=haynet --ip=$ip --name $name haystack_directory
            directory_ips=(${directory_ips[@]} $ip)
            i=`expr $i + 1`
        done



    web_lb_ip=$(printf "$SUBNET_BASE" $BALANCER_IP_BLOCK 2)

    # initiate Directory Web servers instances
    i=0
    web_server_ips=( )
    while [ $i -lt $num_web_server ]
    do
        name=$(printf "%s-%d" "web-server" "$i")
        ip=$(printf "$SUBNET_BASE" $WEB_SERVER_IP_BLOCK $(expr $i + 1))

        directory_env="${directory_ips[0]}"
        for directory_ip in ${directory_ips[@]:1}; do
            directory_env=`echo "$directory_env,$directory_ip"`
        done

        stores_env="${store_ips[0]}"
        for store_ip in ${store_ips[@]:1}; do
            stores_env=`echo "$stores_env,$store_ip"`
        done

        docker run -itd --net=haynet --ip=$ip --env WEB_LB_IP="$web_lb_ip" --env CACHE_LB_IP="$cache_load_balancer_ip" --env DIRECTORY_IPS="$directory_env" --env STORE_IPS="$stores_env" --name $name web_server
        web_server_ips=(${web_server_ips[@]} $ip)
        i=`expr $i + 1`
    done


    # initiate Nginx instance for load balancing Web Servers

    # prepare nginx.conf with web server ips
    server_list_pattern="/#WEB_SERVER_LIST/a\\"
    for server_ip in ${web_server_ips[@]}; do
        server_list_pattern="${server_list_pattern}\tserver ${server_ip}:$WEB_SERVER_PORT;\n"
    done
    sed "$server_list_pattern" $BALANCER_BASE_DIR/nginx.conf.template > $BALANCER_BASE_DIR/nginx.conf

    #Finally build load_balancer image
    docker build -t web_load_balancer $BALANCER_BASE_DIR

    #ip=$(printf "$SUBNET_BASE" $BALANCER_IP_BLOCK 2)
    docker run -itd --net=haynet --ip=$web_lb_ip --name "web_load_balancer" web_load_balancer

    printf "\nNOTE: Use Load Balancer IP $web_lb_ip to fetch photos. Eg: curl http://$web_lb_ip/\n\n"
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
