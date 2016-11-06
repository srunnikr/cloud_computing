from flask import Flask, render_template
from haystackCacheController import haystackCacheController
from flask.ext.api import status
import json

app = Flask(__name__)

controller = haystackCacheController()

@app.route('/photo/<photo_id>', methods=['GET'])
def getPhoto(photo_id):
    print "request to get photo with id: ", photo_id
    queryResult = controller.queryMemcache(photo_id)
    print "Trying to get from cache, returned: ", queryResult
    return queryResult

@app.route('/photo/<photo_id>', methods=['POST'])
def writePhoto(photo_id):
    print "Write request should go to the store directly"
    return status.HTTP_400_BAD_REQUEST

if __name__ == '__main__':
    # Create a controller instance
    app.run()
