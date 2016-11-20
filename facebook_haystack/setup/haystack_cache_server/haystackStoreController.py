from cassandra.cluster import Cluster
import time

class haystackStoreController():

    def __init__(self):
        # Configure the cluster
        print ("Starting haystackStoreController instance")
        self.cassandraServers = self.getCassandraServers("haystackCassandraConfig.txt")
        print (self.cassandraServers)
        self.cluster = Cluster(self.cassandraServers, port=9042)

        print ("Testing connections")
        self.cluster.connect_timeout = 120
        # time.sleep(60)

        self.session = self.cluster.connect()
        self.session.set_keyspace("haystack_store_db")

    def queryNeedle(self, blob_id, offset):
        print ("Querying for the needle")
        rows = self.session.execute('SELECT * FROM haystack_store_db.blob_data WHERE blob_id=\''+blob_id+'\' ALLOW FILTERING ')
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

    def queryStore(self, key, cookie):
        # Query the index file store to get blob id and offset
        print ("Querying the store")
        rows = self.session.execute('SELECT * FROM haystack_store_db.index_data WHERE photo_id=\''+key+'\' AND cookie=\''+cookie+'\' ALLOW FILTERING ')
        # Assuming there are no duplicate photo ids
        for row in rows:
            blob_id = row.blob_id
            needle_offset = row.needle_offset
            print ("blob_id: ",blob_id," and needle_offset: ",needle_offset)
            photo = self.queryNeedle(blob_id, needle_offset)
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
        s = []
        with open(configFile, "r") as f:
            data = f.readlines()
            for line in data:
                s.append(line.strip())
        return s
