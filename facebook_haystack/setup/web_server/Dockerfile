FROM node:latest

RUN mkdir -p /user/src/app
WORKDIR /usr/src/app

COPY package.json /usr/src/app
RUN npm install

COPY server.js /usr/src/app
COPY index.html /usr/src/app

EXPOSE 8080

CMD [ "npm", "start" ]
