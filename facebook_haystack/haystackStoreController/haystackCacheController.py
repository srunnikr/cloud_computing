from pymemcache.client.base import Client
import socket

class haystackCacheController():

    def __init__(self):
        print "Creating instance of haystackCacheController"
        # Create connections to the memcache cluster
        self.memcacheServers = self.getMemcacheServers("haystackCacheConfig.txt")
        print self.memcacheServers
        print "Creating memcache instance"
        # TODO : We need a tuple for one cache or otherwise we need a list of tuples
        self.client = Client(self.memcacheServers[0])

    def queryMemcache(self, key):
        result = self.client.get(key)
        print "query result: ", result
        if result == None:
            print "Writing so that we dont miss next time"
            self.writeMemcache(key, "value")
        return result

    def writeMemcache(self, key, value):
        self.client.set(key, value)

    def getMemcacheServers(self, configFile):
        # returns a list of tuples of client and port
        s = []
        with open(configFile, "r") as f:
            data = f.readlines()
            for line in data:
                serverIp = line.split("=")[1].split(":")[0].strip()
                serverPort = int(line.split("=")[1].split(":")[1].strip())
                s.append((serverIp, serverPort))
        return s
