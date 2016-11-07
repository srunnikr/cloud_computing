FROM cassandra:latest

RUN apt-get update
RUN apt-get install -y curl
RUN curl -sL https://deb.nodesource.com/setup_7.x | bash -
RUN apt-get install -y nodejs

RUN mkdir -p /user/src/app
WORKDIR /usr/src/app

COPY package.json /usr/src/app
RUN npm install

COPY server.js /usr/src/app

EXPOSE 8080

CMD [ "npm", "start" ]
