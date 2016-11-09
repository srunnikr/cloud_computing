'use strict';

const express = require('express');
const request = require('request');
const directory = 'http://localhost:8080'; // PLACEHOLDER
const app = express();
const PORT = 8080;

/* ######## store setup   ######## */
const dataStore = '192.168.1.1';
const cassandra = require('cassandra-driver');
const client = new cassandra.Client({ contactPoints: [dataStore] });

// connect to the database
client.connect(function (err) {
  if (err) return console.error(err);
  console.log('Connected to cluster with %d host(s): %j', client.hosts.length, client.hosts.keys());
});

// create the keyspace
const q = "CREATE KEYSPACE IF NOT EXISTS cse291 WITH replication = {\'class\': \'SimpleStrategy\', \'replication_factor\' : 1}"
client.execute(q, function (err, result) {
    if (err) return console.error(err);
    console.log("Created the keyspace")
});

// create the table (temporary schema)
client.execute('CREATE TABLE IF NOT EXISTS cse291.photos(id int PRIMARY KEY, data blob)', function (err, result) {
    if (err) return console.error(err);
    console.log("Created the table");
});
/* ######## end of store setup   ######## */

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
console.log('Web server is running on port: ' + PORT);
