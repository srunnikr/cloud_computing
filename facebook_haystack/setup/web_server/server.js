'use strict';

const express = require('express');
const request = require('request');
const sleep = require('sleep');
const app = express();
const PORT = 8080;

/* ######## store & directory setup   ######## */
const cassandra = require('cassandra-driver');
const dataStore = '192.168.1.1';
const storeClient = new cassandra.Client({ contactPoints: [dataStore] });
const directory = '192.168.3.1';
const directoryClient = new cassandra.Client({ contactPoints: [directory] });

const createStoreKeyspace = 'CREATE KEYSPACE IF NOT EXISTS haystack_store_db WITH replication = {\'class\': \'SimpleStrategy\', \'replication_factor\' : 1}';
const createStoreTable = 'CREATE TABLE IF NOT EXISTS haystack_store_db.photo_data(photo_id text PRIMARY KEY, cookie text, delete_flag boolean, data text)';
const createDirectoryKeyspace = 'CREATE KEYSPACE IF NOT EXISTS haystack_dir_db WITH replication = {\'class\': \'SimpleStrategy\', \'replication_factor\' : 1}';
const createDirectoryTable = 'CREATE TABLE IF NOT EXISTS haystack_dir_db.photo_metadata(photo_id text PRIMARY KEY, cookie text, machine_id int, logical_volume_id int, alt_key int, delete_flag boolean)';

storeConnect();
var tries = 1;
function storeConnect() {
	storeClient.connect(function (err) {
	  if (err) {
		  if (tries <= 20) {
			 console.log("Failed to connect to Cassandra store. Trying again in 5 seconds: " + tries);
			 tries++;
			 sleep.sleep(5);
			 storeConnect();
		  } else {
			  return console.error("Failed to connect to Cassandra store after 20 tries: " + err); 
		  }
	  } else {
		 console.log('Connected to store with %d host(s): %j', storeClient.hosts.length, storeClient.hosts.keys());
		 build(storeClient, createStoreKeyspace, createStoreTable, "store");
		 tries = 1;
		 directoryConnect(); // now connect to the directory
	  }
	});
}

function directoryConnect() {
	directoryClient.connect(function (err) {
	  if (err) {
		  if (tries <= 20) {
			 console.log("Failed to connect to Cassandra directory. Trying again in 5 seconds: " + tries);
			 tries++;
			 sleep.sleep(5);
			 directoryConnect();
		  } else {
			  return console.error("Failed to connect to Cassandra directory after 20 tries: " + err); 
		  }
	  } else {
		 console.log('Connected to directory with %d host(s): %j', directoryClient.hosts.length, directoryClient.hosts.keys());
		 build(directoryClient, createDirectoryKeyspace, createDirectoryTable, "directory");
	  }
	});
}

function build(client, keyspace, table, type) {
	const q = "";
	client.execute(keyspace, function (err, result) {
		if (err) {
			return console.error(err);
		} else {
			console.log("Created the keyspace for " + type);
		    createTable(client, table, type);			
		}
	});
}

function createTable(client, q, type) {
	client.execute(q, function (err, result) {
		if (err) {
			return console.error(err);
		} else {
			console.log("Created the table for " + type);
		}
	});
}
/* ######## end of store & directory setup   ######## */


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
	const q = 'UPDATE cse291.photos SET deleted=true WHERE id=?';
	client.execute(q, [req.params.photo_id], function (err, result) {
		if (err) {
			console.error(err);
			res.status(400).send(err);
		}
		console.log("Deleted photo with id: " + req.params.photo_id);
		res.status(202);
	});
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
