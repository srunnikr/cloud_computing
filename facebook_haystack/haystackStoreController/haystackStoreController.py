from cassandra.cluster import Cluster

class haystackStoreController():

    def __init__(self):
        # Configure the cluster
        print "Starting haystackStoreController instance"
        self.cassandraServers = self.getCassandraServers("haystackCassandraConfig.txt")
        print self.cassandraServers
        # TODO: Need to make sure if the tuple is just one element or a list
        # cassandra we just need the IP, port and load balancing are default
        self.cluster = Cluster(self.cassandraServers)

    def queryStore(self, key):
        session = self.cluster.connect()
        # Add query here for cassandra
        # session.execute('SELECT id, photo FROM phtots WHERE id = key')
        return None

    def writeStore(self, key, data):
        # Add code to write to cassandra server
        pass

    def getCassandraServers(self, configFile):
        # returns a list of IP addresses of the servers. Port and load balancing
        # are the default values
        s = []
        with open(configFile, "r") as f:
            data = f.readlines()
            for line in data:
                s.append(line)
        return s