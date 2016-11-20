from flask import Flask, render_template
from haystackCacheController import haystackCacheController
from haystackStoreController import haystackStoreController
from flask.ext.api import status
from flask import request
from flask import Response
import json

app = Flask(__name__)

cache_controller = haystackCacheController()
store_controller = haystackStoreController()

@app.route('/<machine_id>/<logical_vol>/<photo_id>', methods=['GET'])
def getPhoto(machine_id, logical_vol, photo_id):
    cookie_id = str(request.args.get('cookie'))
    photo_id = photo_id.split(".")[0]
    print ("Cookie: "+str(cookie_id))
    print ("request to get photo with id: "+photo_id+ " and cookie id: "+cookie_id)
    queryResult = cache_controller.queryMemcache(photo_id+cookie_id)
    if queryResult == None:
        print ("Memcache miss, searching on store")
        queryResult = store_controller.queryStore(photo_id, cookie_id)
        if queryResult == None:
            print ("Detected inconsistency, photo "+photo_id + " not found in cache or store")
            resp = Response("DAMMMMMNNNNNN....Invalid URL", status=404)
            return resp
        cache_controller.writeMemcache(photo_id+cookie_id, queryResult)
        resp = Response("<html><body><img src=\""+ queryResult +"\"/></body></html>",status=200, mimetype='text/html')
        return resp
    else:
        print("Cache hit: " + photo_id)
        resp = Response("<html><body><img src=\""+ queryResult.decode('ascii') +"\"/></body></html>",status=200, mimetype='text/html')
        return resp


# http://memcachedip/machineid/logicalvomume/photoname.jpg?cookie=xxx
@app.route('/<machine_id>/<logical_vol>/<photo_id>?cookie=<cookie_id>', methods=['POST'])
def writePhoto(photo_id, cookie_id):
    print ("Write request should go to the store directly")
    return status.HTTP_400_BAD_REQUEST

# POST request to this URL will take the photo from store and cache it on memserver
# URL type : /cacheit/phpto1.jpg?cookie=cookie_id
@app.route('/cacheit/<photo_id>', methods=['POST'])
def cachePhoto(photo_id):
    photo_id = photo_id.split(".")[0]
    cookie_id = str(request.args.get('cookie'))
    print ("Caching request for photo: ",photo_id, " cookie: ", cookie_id)
    photo = store_controller.queryStore(photo_id, cookie_id)
    if photo == None:
        print ("Photo not found in store, skipping the caching part")
        resp = Response("Error while caching, not found in store",status=404)
        return resp
    print ("Writing to cache")
    cache_controller.writeMemcache(photo_id+cookie_id, photo)
    return Response(status=200)

if __name__ == '__main__':
    # Create a controller instance
    app.run(host='0.0.0.0',port=8080)
