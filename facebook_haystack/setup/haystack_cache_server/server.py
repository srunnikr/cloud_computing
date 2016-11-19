from flask import Flask, render_template
from haystackCacheController import haystackCacheController
from haystackStoreController import haystackStoreController
from flask.ext.api import status
import json

app = Flask(__name__)

cache_controller = haystackCacheController()
store_controller = haystackStoreController()

@app.route('/<machine_id>/<logical_vol>/<photo_id>?cookie=<cookie_id>', methods=['GET'])
def getPhoto(photo_id, cookie_id):
    print "request to get photo with id: ", photo_id, " and cookie id: ", cookie_id
    queryResult = cache_controller.queryMemcache(str(photo_id)+cookie_id)
    if queryResult == None:
        print "Memcache miss, searching on store"
        queryResult = store_controller.queryStore(photo_id, cookie_id)
    if queryResult == None:
        print "Detected inconsistency, photo ", photo_id, " not found in cache or store"
        resp = Response(status=403)
        return resp
    resp = Response(queryResult,status=200, mimetype='image/jpeg')
    return resp

# http://memcachedip/machineid/logicalvomume/photoname.jpg?cookie=xxx
@app.route('/<machine_id>/<logical_vol>/<photo_id>?cookie=<cookie_id>', methods=['POST'])
def writePhoto(photo_id, cookie_id):
    print "Write request should go to the store directly"
    return status.HTTP_400_BAD_REQUEST

if __name__ == '__main__':
    # Create a controller instance
    app.run()
