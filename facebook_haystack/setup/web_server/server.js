'use strict';

const express = require('express');
const request = require('request');

const directory = 'http://localhost:8080'; // PLACEHOLDER
const dataStore = 'http://localhost:8080'; // PLACEHOLDER
const PORT = 8080;
const app = express();

app.get('/', function (req, res) {
  res.send('Haystack photos\n');
});

app.get('/photos/:photo_id', function(req, res) {
  // get the URL from the directory
  request(dataStore + '/placeholder', function (error, response, body) {
    if (!error && response.statusCode == 200) {
        var url = body;

	// return the response to the browser
	res.send(url);
    }
  });
});

app.delete('/photos/:photo_id/delete', function(req, res) {
  // TODO
    res.send("Photo id (DELETE): " + req.params.photo_id);
 });

app.post('/photos', function(req, res) {
  // TODO: post to dataStore
  res.send("Photo upload (POST)");
});

app.get('/placeholder', function(req, res) {
  // place holder for the directory URL
  res.send('http://cache/machine_id/logical_id/photo');
});

app.listen(PORT);
console.log('Running on http://localhost:' + PORT);
