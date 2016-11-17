'use strict';

const express = require('express');
const request = require('request');
const directory = 'http://localhost:8080'; // PLACEHOLDER
const app = express();
const PORT = 8080;
const MAX_VOLUME_ID = 8;
const MEMCACHED_IP = '192.168.1.3';
var uuid = require('node-uuid');

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
const q = "CREATE KEYSPACE IF NOT EXISTS cse291 WITH replication = {\'class\': \'SimpleStrategy\', \'replication_factor\' : 1}";
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


/* ########  haystack directory setup   ######## */
const haystack_dir_ip = '127.0.0.1';
const cassandra_driver = require('cassandra-driver');
const client_haystack_dir = new cassandra_driver.Client({ contactPoints: [haystack_dir_ip] });

// connect to the database
client_haystack_dir.connect(function (err) {
  if (err) return console.error(err);
  console.log('Connected to cluster with %d host(s): %j', client_haystack_dir.hosts.length, client_haystack_dir.hosts.keys());
});

// create the keyspace
var create_keyspace_query = "CREATE KEYSPACE IF NOT EXISTS haystack_dir_db WITH replication = {\'class\': \'SimpleStrategy\', \'replication_factor\' : 1}";
client_haystack_dir.execute(create_keyspace_query, function (err, result) {
    if (err) return console.error(err);
    console.log("Created the keyspace");
});

// create the photo_metadata table
client_haystack_dir.execute('CREATE TABLE IF NOT EXISTS haystack_dir_db.photo_metadata(photo_id int PRIMARY KEY, cookie text, logical_volume_id int, alt_key int, delete_flag boolean)', function (err, result) {
    if (err) return console.error(err);
    console.log("Created the table");
});

//create the logicalto machineID mapping
client_haystack_dir.execute('CREATE TABLE IF NOT EXISTS haystack_dir_db.logical_to_machine(logical_id int PRIMARY KEY, machine_id', function (err, result) {
    if (err) return console.error(err);
    console.log("Created the table");
});

/* ######## end of haystack directory setup ######## */


function isReadOnly(volume_id) {
	// TODO
  // check used space of the volume and then mark accordingly
	return true;
}

function getVolumeId() {
	// get random volume
  var m_id = Math.floor(Math.random() * MAX_VOLUME_ID);
	
  // check if it is read only
  while (!isReadOnly(m_id))
	{
		m_id = Math.floor(Math.random() * MAX_VOLUME_ID);
	}
	return m_id;
}

function createAltKey(photo_id) {
	//TODO
	var m_alt_key = Math.floor(Math.random() * 2147483648);
	return m_alt_key;
}

function createCookie() {
	var result = '';
	var length = 8;
	var chars = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
  for (var i = length; i > 0; --i) 
    result += chars[Math.floor(Math.random() * chars.length)];
  return result;
}

//TODO Functions to maintain and update logical to machine ID mapping

function addMetadataToHaystackDir(photo_id) {
	var insert_metadata_query = 'INSERT INTO haystack_dir_db.photo_metadata (photo_id, cookie, logical_volume_id, alt_key, delete_flag) VALUES (:photo_id, :cookie, :logical_volume_id, :alt_key, :delete_flag)';
	var m_cookie = createCookie();
	var m_logical_volume_id = getVolumeId();
	var m_alt_key = createAltKey(photo_id);
	var params = { photo_id: photo_id, cookie: m_cookie, logical_volume_id: m_logical_volume_id, alt_key: m_alt_key, delete_flag: false};
	
  client_haystack_dir.execute(insert_metadata_query, params, { prepare: true }, 
    function (err, result) {
      if (err) return console.error(err);
      console.log("Added metadata to photo_metadata");
    }
  );

  /*
  client_haystack_dir.execute('SELECT * FROM haystack_dir_db.photo_metadata', 
    function (err, result) {
      if (err) return console.error(err);
      for (var i = 0; i < result.rows.length; i++)
        console.log(result.rows[i].photo_id + " " + result.rows[i].cookie + " " +result.rows[i].logical_volume_id);
    }
  );
  */
}

function deleteMetadataFromHaystackDir(photo_id) {
	var update_delete_flag_query = 'UPDATE haystack_dir_db.photo_metadata SET delete_flag=true WHERE photo_id = :photo_id';
  var params = {photo_id: photo_id};
	client_haystack_dir.execute(query, params, { prepare: true }, 
    function (err, result) {
      if (err) return console.error(err);
      console.log("Updated delete_flag for " + photo_id);
    });
}

function createURL(photo_id, callback) {
  var get_metadata_query = 'SELECT * FROM haystack_dir_db.photo_metadata WHERE photo_id = :photo_id';
  var params = {photo_id: photo_id};
  var url = 'http://' + MEMCACHED_IP;
  client_haystack_dir.execute(get_metadata_query, params, { prepare: true }, 
    function (err, result) {
      if (err) return console.error(err);
      console.log("Retrieved metadata for " + photo_id);
      if (result.rows.length == 0) {
        callback(url);
        return;
      }
      var result_row = result.rows[0];
	  
	  var get_machine_ID_query = 'SELECT machine_id FROM haystack_dir_db.logical_to_machine WHERE logical_id = :logical_id'
	  var params_mid = {logical_id: result_row.logical_volume_id};
	  var machineID = '';
	  client_haystack_dir.execute(get_machine_ID_query,params_mid, { prepare: true},
		function (err,res){
			if (err) return console.error(err);
			console.log("Retrieved machine ID for " + result_row.logical_volume_id);
			if (res.rows.length == 0) {
				callback(machineID);
				return;
			}
			machineID += res.rows[0].machine_id;
		});
	  
      url += "/" + machineID + "/" + result_row.logical_volume_id + "/" + photo_id 
      + ".jpg?cookie=" + result_row.cookie;
      console.log("URL for " + photo_id + ": " + url);
      callback(url);
    });
}

app.get('/', function (req, res) {
  res.send('Haystack photos\n');
});

/* Photo retrieval */
app.get('/photos/:photo_id', function(req, res) {
  // check for existence of this photo_id in memcached
  // TODO

  // get the URL from the directory
  createURL(req.params.photo_id, 
    function(url) {
      res.send(url);
    });
  
  // using the parameters in this URL, query the Haystack Store
  // TODO
});

app.delete('/photos/:photo_id/delete', function(req, res) {
  // TODO
  // call deleteMetadataFromHaystackDir here
  res.send("Photo id (DELETE): " + req.params.photo_id);
 });

app.post('/photos', function(req, res) {
  //console.log(req.params.photo_id);
  var assign_photo_id = uuid.v1();
  console.log(assign_photo_id);
  addMetadataToHaystackDir(assign_photo_id);
  // TODO: post to dataStore
  res.send("Photo upload (POST)");
});

app.get('/placeholder', function(req, res) {
  // place holder for the directory URL
  res.send('http://cache/machine_id/logical_id/photo');
});

app.listen(PORT);
console.log('Web server is running on port: ' + PORT);