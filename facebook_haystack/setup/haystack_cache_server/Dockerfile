FROM python:latest
#FROM ubuntu:latest

#RUN apt-get update
#RUN apt-get install -y python-dev gcc g++ make binutils python-pip
RUN pip install Flask flask-api pymemcache cassandra-driver

RUN mkdir -p /usr/src/app

WORKDIR /usr/src/app

COPY haystackCacheConfig.txt /usr/src/app/haystackCacheConfig.txt
COPY haystackCacheController.py /usr/src/app/haystackCacheController.py
COPY haystackCassandraConfig.txt /usr/src/app/haystackCassandraConfig.txt
COPY haystackStoreController.py /usr/src/app/haystackStoreController.py
COPY server.py /usr/src/app/server.py

CMD python /usr/src/app/server.py

#RUN /bin/bash
