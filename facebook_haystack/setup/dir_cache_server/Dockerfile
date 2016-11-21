FROM python:latest
#FROM ubuntu:latest

#RUN apt-get update
#RUN apt-get install -y python-dev gcc g++ make binutils python-pip
RUN pip install Flask flask-api pymemcache cassandra-driver

RUN mkdir -p /usr/src/app

WORKDIR /usr/src/app

COPY dirCacheConfig.txt /usr/src/app
COPY haystackCacheControllerDirectory.py /usr/src/app
COPY cache_directory.py /usr/src/app

EXPOSE 80

CMD python /usr/src/app/cache_directory.py

#RUN /bin/bash
