'use strict';

const express = require('express');
const cassandra = require('cassandra-driver');

const PORT = 8080;
const app = express();

app.get('/', function (req, res) {
  res.send('Haystack store\n');
});

// handle GET requests from cache
app.get('/photos/:photo_id', function(req, res) {
  // 1. lookup the meta data in the in-memory cache
  
  // 2. query the database for the image
  
  // 3. return the image
  
});

// handle DELETE requests from the web server
app.delete('/photos/:photo_id/delete', function(req, res) {
  // 1. Set the delete flag for the photo stored in the database
  
  // 2. return success status
  
});

// handle POST requests from the web server
app.post('/photos', function(req, res) {
  // 1. create an object for the photo
  
  // 2. store the photo in the database
  
  // 3. return success status
  
});


app.listen(PORT);
console.log('Store running on http://localhost:' + PORT);
