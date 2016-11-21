FROM flask_cache_server:latest
#FROM ubuntu:latest

RUN mkdir -p /usr/src/app

WORKDIR /usr/src/app

COPY dirCacheConfig.txt /usr/src/app
COPY haystackCacheControllerDirectory.py /usr/src/app
COPY cache_directory.py /usr/src/app

EXPOSE 80

CMD python /usr/src/app/cache_directory.py

#RUN /bin/bash
