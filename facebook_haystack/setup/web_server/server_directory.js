'use strict';

const express = require('express');
const request = require('request');
const directory = 'http://localhost:8080'; // PLACEHOLDER
const app = express();
const PORT = 8080;
const MAX_VOLUME_ID = 8;

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

/* ######## directory setup   ######## */
const directoryIP = '192.168.1.2';
const cassandra_dir = require('cassandra-driver');
const client_dir = new cassandra_dir.Client({ contactPoints: [directoryIP] });
var logical_id_availability
// connect to the database
client_dir.connect(function (err) {
  if (err) return console.error(err);
  console.log('Connected to cluster with %d host(s): %j', client_dir.hosts.length, client_dir.hosts.keys());
});

// create the keyspace
const q = "CREATE KEYSPACE IF NOT EXISTS haystackDirectoryDB WITH replication = {\'class\': \'SimpleStrategy\', \'replication_factor\' : 1}"
client_dir.execute(q, function (err, result) {
    if (err) return console.error(err);
    console.log("Created the keyspace")
});

// create the table (temporary schema)
client_dir.execute('CREATE TABLE IF NOT EXISTS haystackDirectoryDB.photoDirectoryTable(photo_id int PRIMARY KEY, cookie text, logical_volume_id int, alt_key int, delete_flag boolean)', function (err, result) {
    if (err) return console.error(err);
    console.log("Created the table");
});
/* ######## end of directory setup   ######## */


function isReadOnly(volume_id){
	//TODO
	return true;
}
function getVolumeId(){
	var m_id = Math.floor(Math.random() * MAX_VOLUME_ID);
	while (!isReadOnly(m_id))
	{
		m_id = Math.floor(Math.random() * MAX_VOLUME_ID);
	}
	return m_id;
}
function createAltKey(photo_id){
	//TODO
	var m_alt_key = Math.floor(Math.random() * 2147483648);
	return m_alt_key;
}

function createCookie(){
	var result = '';\
	var length = 8;
	var chars[] = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
    for (var i = length; i > 0; --i) result += chars[Math.floor(Math.random() * chars.length)];
    return result;
}

function addMappingToDirectory(photo_id_key){
	
	var query = 'INSERT INTO haystackDirectoryDB.photoDirectoryTable (photo_id, cookie, logical_volume_id, alt_key, delete_flag) VALUES (:photo_id, :cookie, :logical_volume_id, :alt_key, :delete_flag)';
	var m_cookie = createCookie();
	var m_logical_volume_id = getVolumeId();
	var m_alt_key = createAltKey(photo_id_key);
	var params = { photo_id: photo_id_key, cookie: m_cookie, logical_volume_id: m_logical_volume_id, alt_key: m_alt_key, delete_flag: false};
	client.execute(query, params, { prepare: true }, callback);
}


function deleteMappingToDirectory(photo_id_key){
	
	var query = ' UPDATE haystackDirectoryDB.photoDirectoryTable SET delete_flag=true WHERE photo_id = photo_id_key;';
	client.execute(query, { prepare: true }, callback);
	
}

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
  // call deleteMappingToDirectory here
    res.send("Photo id (DELETE): " + req.params.photo_id);
 });

app.post('/photos', function(req, res) {
  // TODO: post to dataStore
  //call addMappingToDirectory here
  res.send("Photo upload (POST)");
});

app.get('/placeholder', function(req, res) {
  // place holder for the directory URL
  res.send('http://cache/machine_id/logical_id/photo');
});

app.listen(PORT);
console.log('Web server is running on port: ' + PORT);
