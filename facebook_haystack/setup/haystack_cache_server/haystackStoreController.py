from cassandra.cluster import Cluster
import cassandra
import os
import time

class haystackStoreController():

    def __init__(self):
        # Configure the cluster
        self.count = 0
        print ("Starting haystackStoreController instance")
        self.cassandraServers = self.getCassandraServers("haystackCassandraConfig.txt")
        print (self.cassandraServers)
        #self.cluster = Cluster(self.cassandraServers, port=9042)

        print ("Connecting to Haystack Store")
        self.sessions = []

        #Changes to support multiple stores
        for store in self.cassandraServers:
            self.count = 0
            print ("Initiaing cassandra connection to store: ", store)
            session = self.retry_connect_store(store)
            self.sessions.append(session)

        #self.session = self.retry_connect()

    def retry_connect_store(self, store):
        try:
            self.count += 1
            print ("Trying to connect : ", self.count)
            cluster = Cluster([store], port=9042)
            session = cluster.connect()
            session.set_keyspace("haystack_store_db")
            print ("Connected to store: ",store)
            return session
        except Exception as e:
            if self.count >= 20:
                print ("Connection to Cassandra store ", store," failed after 20 retries")
                print(e)
                return None
            print ("Failed to connect to Cassandra server ",store,", waiting for 5s to retry")
            time.sleep(5)
            return self.retry_connect_store(store)

    def retry_connect(self):
        try:
            self.count += 1
            print ("Trying to connect : ", self.count)
            self.cluster = Cluster(self.cassandraServers, port=9042)
            self.session = self.cluster.connect()
            self.session.set_keyspace("haystack_store_db")
            return self.session
        except Exception as e:
            if self.count > 20:
                print ("Connection to Cassandra failed after 20 retries")
                print(e)
                return None
            print ("Failed to connect, waiting for 5s to retry")
            time.sleep(5)
            return self.retry_connect()

    def queryNeedle(self, blob_id, offset, machine_id):
        print ("Querying for the needle")
        session = self.sessions[int(machine_id)]
        rows = session.execute('SELECT * FROM haystack_store_db.blob_data WHERE blob_id=\''+blob_id+'\' ALLOW FILTERING ')
        for row in rows:
            if offset == 1:
                return row.photo1
            elif offset == 2:
                return row.photo2
            elif offset == 3:
                return row.photo3
            else:
                print("Invalid needle_offset")
                return None
        return None

    def queryStore(self, key, cookie, machine_id):
        # Query the index file store to get blob id and offset
        session = self.sessions[int(machine_id)]
        print ("Querying the store")
        rows = session.execute('SELECT * FROM haystack_store_db.index_data WHERE photo_id=\''+key+'\' AND cookie=\''+cookie+'\' ALLOW FILTERING ')
        # Assuming there are no duplicate photo ids
        for row in rows:
            blob_id = row.blob_id
            needle_offset = row.needle_offset
            print ("blob_id: ",blob_id," and needle_offset: ",needle_offset)
            photo = self.queryNeedle(blob_id, needle_offset, machine_id)
            if photo == None:
                return None
            return photo
        return None

    def writeStore(self, key, data):
        # Write to store done by the webserver, not the web proxy to cache
        print ("Write function for store not supported from web server on cache")

    def getCassandraServers(self, configFile):
        # returns a list of IP addresses of the servers. Port and load balancing
        # are the default values
        stores = os.environ["STORE_IPS"].strip().split(",")

        #s = []
        #with open(configFile, "r") as f:
        #    data = f.readlines()
        #    for line in data:
        #        s.append(line.strip())
        #return s
        return stores
