'use strict';

const express = require('express');
const request = require('request');
const sleep = require('sleep');
const app = express();
const PORT = 8080;
app.listen(PORT);
console.log('Web server is running on port: ' + PORT);

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
		}

		console.log("Created the keyspace for " + type);
		createTable(client, table, type);
	});
}

function createTable(client, q, type) {
	client.execute(q, function (err, result) {
		if (err) {
			return console.error(err);
		}

		console.log("Created the table for " + type);
	});
}
/* ######## end of store & directory setup   ######## */


app.get('/', function (req, res) {
	res.sendFile('index.html', { root: '.' });
});

app.get('/photos/:photo_id', function (req, res) {
	createURL(req.params.photo_id, function (url) {
		console.log("Sending Redirect response");
		res.writeHead(301, "Redirect", { 'Location': url });
		res.end();
	});
});

function createURL(photo_id, callback) {
	var get_metadata_query = 'SELECT * FROM haystack_dir_db.photo_metadata WHERE photo_id = :photo_id';
	var params = { photo_id: photo_id };
	var url = "";
	directoryClient.execute(get_metadata_query, params, { prepare: true }, function (err, result) {
		if (err) {
			return console.error(err);
		}

		console.log("Retrieved metadata for " + photo_id);
		if (result.rows.length == 0) {
			callback(url);
			return;
		}

		var result_row = result.rows[0];
		url += "/" + result_row.machine_id + "/" + result_row.logical_volume_id + "/" + photo_id + ".jpg?cookie=" + result_row.cookie;
		console.log("URL for " + photo_id + ": " + url);
		callback(url);
	});
}

app.delete('/photos/:photo_id/delete', function (req, res) {
	const q = 'UPDATE haystack_store_db.photos SET delete_flag=true WHERE photo_id=?';
	// set the flag in the store
	storeClient.execute(q, [req.params.photo_id], function (err, result) {
		if (err) {
			console.error(err);
			res.status(400).send(err);
		}
		console.log("Store: deleted photo with id: " + req.params.photo_id);

		// set the flag in the directory
		const q2 = 'UPDATE haystack_dir_db.photo_metadata SET delete_flag=true WHERE photo_id=?';
		directoryClient.execute(q2, [req.params.photo_id], function (err, result) {
			if (err) {
				console.error(err);
				res.status(400).send(err);
			}
			console.log("Directory: deleted photo with id: " + req.params.photo_id);
			res.status(202);
		});

	});
});

app.post('/photos', function (req, res) {
	console.log("Received general POST");
	console.log("[200] " + req.method + " to " + req.url);
	var fullBody = '';
	req.on('data', function (chunk) {
		// append the current chunk of data to the fullBody variable
		fullBody += chunk.toString();
	});

	req.on('end', function () {
		// request ended -> do something with the data
		var assign_photo_id = uuid.v1();
		console.log(assign_photo_id);
		addMetadataToHaystackDir(assign_photo_id, function (params) {
			var insert_data_query = "INSERT INTO haystack_store_db.photo_data (photo_id, cookie, delete_flag, data) VALUES (:photo_id, :cookie, :delete_flag, :data)";
			var parameters = { photo_id: params.photo_id, cookie: params.cookie, delete_flag: false, data: fullBody };

			// based on machine id / logical volume
			storeClient.execute(insert_data_query, parameters, { prepare: true }, function (err, result) {
				if (err) return console.error(err);
				console.log("Added photo data");
			});

			res.writeHead(200, "OK", { 'Content-Type': 'text/html' });

			// send the photo blob to store
			var photo_data = params.photo_id + " " + params.machine_id + " " + params.cookie + " " + params.logical_volume_id;
			res.write(photo_data);
			res.end();
		});
	});
});

const MAX_MACHINE_ID = 8;
const MAX_VOLUME_ID = 4;
const MEMCACHED_IP = '128.54.243.120:8080';

function isReadOnly(volume_id) {
	// TODO: check used space of the volume and then mark accordingly
	return true;
}

function getMachineId() {
	return Math.floor(Math.random() * MAX_MACHINE_ID);
}

function getVolumeId() {
	// get random volume
	var m_id = Math.floor(Math.random() * MAX_VOLUME_ID);

	// check if it is read only
	while (!isReadOnly(m_id)) {
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
	for (var i = length; i > 0; --i) {
		result += chars[Math.floor(Math.random() * chars.length)];
	}
	return result;
}

//TODO Functions to maintain and update logical to machine ID mapping
function addMetadataToHaystackDir(photo_id, callback) {
	var insert_metadata_query = 'INSERT INTO haystack_dir_db.photo_metadata (photo_id, cookie, machine_id, logical_volume_id, alt_key, delete_flag) VALUES (:photo_id, :cookie, :machine_id, :logical_volume_id, :alt_key, :delete_flag)';

	var m_cookie = createCookie();
	var m_logical_volume_id = getVolumeId();
	var m_alt_key = createAltKey(photo_id);
	var m_machine_id = getMachineId();
	var params = { photo_id: photo_id, cookie: m_cookie, machine_id: m_machine_id, logical_volume_id: m_logical_volume_id, alt_key: m_alt_key, delete_flag: false };

	directoryClient.execute(insert_metadata_query, params, { prepare: true }, function (err, result) {
		if (err) return console.error(err);
		console.log("Added metadata to photo_metadata");
		callback(params);
	});
}
