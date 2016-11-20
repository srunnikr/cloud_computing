from cassandra.cluster import Cluster
import os

class haystackStoreController():

    def __init__(self):
        # Configure the cluster
        print "Starting haystackStoreController instance"
        self.cassandraServers = self.getCassandraServers("haystackCassandraConfig.txt")
        print self.cassandraServers
        self.cluster = Cluster(self.cassandraServers, port=9042)

        print "Testing connections"
        self.cluster.connect_timeout = 120
        self.session = self.cluster.connect()
        self.session.set_keyspace("haystack_store_db")

    def queryStore(self, key, cookie):
        rows = self.session.execute('SELECT data FROM haystack_store_db.photo_data WHERE photo_id='+str(key)+'AND cookie='+str(cookie)+' ALLOW FILTERING')
        # Assuming there are no duplicate photo ids
        return rows[0].data

    def writeStore(self, key, data):
        # Write to store done by the webserver, not the web proxy to cache
        print "Write function for store not supported from web server on cache"

    def getCassandraServers(self, configFile):
        # returns a list of IP addresses of the servers. Port and load balancing
        # are the default values
        stores = os.environ["STORE_IPS"]strip().split(",")

        #s = []
        #with open(configFile, "r") as f:
        #    data = f.readlines()
        #    for line in data:
        #        s.append(line.strip())
        #return s
        return stores
