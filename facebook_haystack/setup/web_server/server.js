'use strict';

const express = require('express');
const request = require('request');
const uuid = require('node-uuid');
const sleep = require('sleep');
const XMLHttpRequest = require('xmlhttprequest').XMLHttpRequest;

const app = express();
const PORT = 8080;
app.listen(PORT);
console.log('Web server is running on port: ' + PORT);

//const cacheserver = '172.17.0.1:8081';
const cacheserver = process.env['CACHE_LB_IP'];
const webserver = process.env['WEB_LB_IP'];

/* ######## store & directory setup   ######## */
const cassandra = require('cassandra-driver');
//const dataStore = '192.168.1.1';
const dataStore = process.env['STORE_IPS'].split(',');
var storeClient = [];
for (var i = 0; i < dataStore.length; i++) {
    storeClient[i] = new cassandra.Client({ contactPoints: [dataStore[i]] });
}
//const storeClient = new cassandra.Client({ contactPoints: dataStore });

//const directory = '192.168.3.1';
const directory = process.env['DIRECTORY_IPS'].split(',');
const dircache	= process.env['DIR_CACHE_SERVER_IPS'].split(',');

const directoryClient = new cassandra.Client({ contactPoints: directory });

const createStoreKeyspace = 'CREATE KEYSPACE IF NOT EXISTS haystack_store_db WITH replication = {\'class\': \'SimpleStrategy\', \'replication_factor\' : 1}';
const createIndexTable = 'CREATE TABLE IF NOT EXISTS haystack_store_db.index_data(photo_id text PRIMARY KEY, cookie text, delete_flag boolean, blob_id text, needle_offset int)';
const createBlobTable = 'CREATE TABLE IF NOT EXISTS haystack_store_db.blob_data(blob_id text PRIMARY KEY, photo1 text, photo2 text, photo3 text, size int)';
const createDirectoryKeyspace = 'CREATE KEYSPACE IF NOT EXISTS haystack_dir_db WITH replication = {\'class\': \'SimpleStrategy\', \'replication_factor\' : 1}';
const createDirectoryTable = 'CREATE TABLE IF NOT EXISTS haystack_dir_db.photo_metadata(photo_id text PRIMARY KEY, cookie text, machine_id int, logical_volume_id int, alt_key int, delete_flag boolean)';

for (var i = 0; i < dataStore.length; i++) {
    storeConnect(i);
}
// storeConnect();

var dir_connect = false;
var tries = 1;
function storeConnect(index) {
	storeClient[index].connect(function (err) {
		if (err) {
			if (tries <= 20) {
				console.log("Failed to connect to Cassandra store. Trying again in 5 seconds: " + tries);
				tries++;
				sleep.sleep(5);
				storeConnect(index);
			} else {
				return console.error("Failed to connect to Cassandra store after 20 tries: " + err);
			}
		} else {
			console.log('Connected to store with %d host(s): %j', storeClient[index].hosts.length, storeClient[index].hosts.keys());
			build(storeClient[index], createStoreKeyspace, createIndexTable, "store");
			tries = 1;
            if (!dir_connect) {
			    directoryConnect(); // now connect to the directory
                dir_connect = true;
            }
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

		// very ugly way of creating the second table
		if (type == "store") {
			createTable(client, createBlobTable, type);
		}
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
	//First check cache
	//get /cache_directory/photos/phot0_id
	//if response is not NULL res.writeHead(301 with url)
	//if response is null then call createURL
	var cache_dir_url = 'http://' + dircache + '/photos/'+req.params.photo_id;
	var xhr = new XMLHttpRequest();
	xhr.open('GET', cache_dir_url, true);
	xhr.send();

    xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
            var url_from_cache = xhr.responseText;
        	if(url_from_cache != undefined){
        		res.writeHead(301,"Redirect", {'Location': url_from_cache});
        		res.end();
        	}
        	else {
        		createURL(req.params.photo_id, function (url) {
        			if (url == "") {
        				res.writeHead(404, "NOT FOUND");
        				res.end();
        				return;
        			}
        			console.log("Sending Redirect response");
        			res.writeHead(301, "Redirect", { 'Location': url });
        			res.end();
        		});
        	}
        }
    };


});

function createURL(photo_id, callback) {
	var get_metadata_query = 'SELECT * FROM haystack_dir_db.photo_metadata WHERE photo_id = :photo_id AND delete_flag=false ALLOW FILTERING';
	var params = { photo_id: photo_id };
	var url = "http://" + cacheserver;
	directoryClient.execute(get_metadata_query, params, { prepare: true }, function (err, result) {
		if (err) {
			return console.error(err);
		}

		console.log("Retrieved metadata for " + photo_id);
		if (result.rows.length == 0) {
			callback("");
			return;
		}

		var result_row = result.rows[0];
		url += "/" + result_row.machine_id + "/" + result_row.logical_volume_id + "/" + photo_id + ".jpg?cookie=" + result_row.cookie;
		console.log("URL for " + photo_id + ": " + url);
		callback(url);
	});
}

app.delete('/photos/:photo_id/delete', function (req, res) {
	const q = 'UPDATE haystack_store_db.index_data SET delete_flag=true WHERE photo_id=:photo_id';
	var parameters = { photo_id: req.params.photo_id };

	// set the flag in the store
	storeClient[0].execute(q, parameters, { prepare: true }, function (err, result) {
		if (err) {
			console.error(err);
			res.status(400).send(err);
		}
		console.log("Store: deleted photo with id: " + req.params.photo_id);

		// set the flag in the directory
		const q2 = 'UPDATE haystack_dir_db.photo_metadata SET delete_flag=true WHERE photo_id=:photo_id';
		directoryClient.execute(q2, parameters, { prepare: true }, function (err, result) {
			if (err) {
				console.error(err);
				res.status(400).send(err);
			}
			console.log("Directory: deleted photo with id: " + req.params.photo_id);
			res.status(202);
			res.send();
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
			var blobId;
			var index;

			// 1. find a blob with less than 3 photos
			storeClient[params.machine_id].execute('SELECT * FROM haystack_store_db.blob_data WHERE size < 3 ALLOW FILTERING', function (err, result) {
				if (err) {
					return console.error(err);
				}

				if (result.rows.length == 0) {
					// create a new blob
					blobId = uuid.v1();
					index = 0;
					storeClient[params.machine_id].execute('INSERT INTO haystack_store_db.blob_data (blob_id) VALUES(:blobId)', { blobId: blobId }, { prepare: true }, function (error, result) { });
					console.log("created a new blob");
				} else {
					var result_row = result.rows[0];
					blobId = result_row.blob_id;
					index = result_row.size;
				}

				// 2. add photo to blob (index + 1) will be photo1, photo2,..etc
				var insert_blob = 'UPDATE haystack_store_db.blob_data SET photo' + (index + 1) + '=:photo_data, size=:new_size WHERE blob_id=:blob_id';
				var parameters = { blob_id: blobId, new_size: (index + 1), photo_data: fullBody };
				storeClient[params.machine_id].execute(insert_blob, parameters, { prepare: true }, function (err, result) {
					if (err) return console.error(err);
					console.log("Added photo data");
				});

				// 3. create an index for the photo
				var insert_index = "INSERT INTO haystack_store_db.index_data (photo_id, cookie, delete_flag, blob_id, needle_offset) VALUES (:photo_id, :cookie, :delete_flag, :blob_id, :needle_offset)";
				var parameters = { photo_id: params.photo_id, cookie: params.cookie, delete_flag: false, blob_id: blobId, needle_offset: (index + 1) };
				storeClient[params.machine_id].execute(insert_index, parameters, { prepare: true }, function (error, result) {
                    var xhr = new XMLHttpRequest();
                    console.log("http://"+cacheserver+"/cacheit/"+params.photo_id+".jpg?cookie="+params.cookie);
                    xhr.open("POST", "http://"+cacheserver+"/cacheit/"+params.machine_id+"/"+params.photo_id+".jpg?cookie="+params.cookie, true);
                    xhr.send();
                });



			});

			res.writeHead(200, "OK", { 'Content-Type': 'text/html' });

			// send the photo blob to store
			var photo_data = "http://" + webserver + "/photos/" + params.photo_id;
			res.write(photo_data);
			res.end();
		});
	});
});

const MAX_MACHINE_ID = dataStore.length;
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
        var cache_dir_url = 'http://' + dircache + "/cacheit/" + params.machine_id + "/" + params.logical_volume_id + "/" + params.photo_id + "/" + params.cookie;

		var xhr = new XMLHttpRequest();
		xhr.open("POST", cache_dir_url, false);
		xhr.send();
		console.log(xhr.responseText+" "+xhr.status);
		callback(params);
	});
}
