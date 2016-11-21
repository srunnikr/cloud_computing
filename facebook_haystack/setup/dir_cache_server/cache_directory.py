from flask import Flask, render_template
from haystackCacheControllerDirectory import haystackCacheControllerDirectory
from flask.ext.api import status
from flask import request
from flask import Response
import json
import os

app = Flask(__name__)
cache_controller = haystackCacheControllerDirectory()
lb_env = os.environ["CACHE_LB_IP"]

#called from app.get/photos/photo_id first check cache
@app.route('/photos/<photo_id>', methods=['GET'])
def getUrl(photo_id):
    photo_id = photo_id.split(".")[0]
    print ("request to get url for photo with id: "+photo_id)
    queryResult = cache_controller.queryMemcache(photo_id)
    if queryResult == None:
        resp = Response("",status=200, mimetype='text/html')
        return resp
    else:
        print("Cache hit for url: " + photo_id)
        resp = Response(queryResult,status=200, mimetype='text/html')
        return resp


# POST request to this URL will add photo_id and url mapping to directory cache
# URL type : /cacheit/photo_id/url
#called from addMetadataToHaystackDir
@app.route('/cacheit/<machine_id>/<logical_volume_id>/<photo_id>/<cookie>', methods=['POST'])
def cachePhoto(machine_id,logical_volume_id,photo_id,cookie):
    photo_id = photo_id.split(".")[0]
    url = "http://" + lb_env + "/" + machine_id + "/" + logical_volume_id + "/" + photo_id + ".jpg?cookie=" + cookie
    print ("Caching request for photo: ",photo_id, " url: ", url)
    print ("Writing to cache")
    cache_controller.writeMemcache(photo_id, url)
    return Response(status=200)

if __name__ == '__main__':
    # Create a controller instance
    app.run(host='0.0.0.0',port=80)
