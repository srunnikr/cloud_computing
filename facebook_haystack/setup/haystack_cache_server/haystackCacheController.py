from pymemcache.client.hash import HashClient
#from pymemcache.client.base import Client
import socket
import os


MEMCACHE_PORT = 11211

class haystackCacheController():

    def __init__(self):
        print ("Creating instance of haystackCacheController")
        # Create connections to the memcache cluster
        self.memcacheServers = self.getMemcacheServers("haystackCacheConfig.txt")
        print (self.memcacheServers)
        print ("Creating memcache instance")
        # TODO : We need a tuple for one cache or otherwise we need a list of tuples
        #self.client = Client(self.memcacheServers[0])
        self.client = HashClient(self.memcacheServers)

    def queryMemcache(self, key):
        photo = self.client.get(key)
        return photo

    def writeMemcache(self, key, value):
        self.client.set(key, value)

    def getMemcacheServers(self, configFile):
        # returns a list of tuples of client and port

        caches = []
        cache_servers_env = os.environ["CACHE_IPS"].strip().split(",")
        for server in cache_servers_env:
            caches.append((server, MEMCACHE_PORT))
        """
        s = []
        with open(configFile, "r") as f:
            data = f.readlines()
            for line in data:
                serverIp = line.split("=")[1].split(":")[0].strip()
                serverPort = int(line.split("=")[1].split(":")[1].strip())
                s.append((serverIp, serverPort))
        return s
        """
        return caches
