FROM flask_cache_server:latest
#FROM ubuntu:latest

RUN mkdir -p /usr/src/app

WORKDIR /usr/src/app

COPY haystackCacheConfig.txt /usr/src/app/haystackCacheConfig.txt
COPY haystackCacheController.py /usr/src/app/haystackCacheController.py
COPY haystackCassandraConfig.txt /usr/src/app/haystackCassandraConfig.txt
COPY haystackStoreController.py /usr/src/app/haystackStoreController.py
COPY server.py /usr/src/app/server.py

EXPOSE 8080

CMD python /usr/src/app/server.py

#RUN /bin/bash
