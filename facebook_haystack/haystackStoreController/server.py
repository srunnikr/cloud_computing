from flask import Flask, render_template
from haystackCacheController import haystackCacheController
import json

app = Flask(__name__)

controller = haystackCacheController()

@app.route('/photo/<photo_id>', methods=['GET'])
def getPhoto(photo_id):
    print "request to get photo with id: ", photo_id
    data = {}
    data["name"] = "Sreejith"
    queryResult = controller.queryMemcache(photo_id)
    print "Trying to get from cache, returned: ", queryResult
    return json.dumps(data)

@app.route('/photo/<photo_id>', methods=['POST'])
def writePhoto(photo_id):
    print "Request to write photo with id: ", photo_id
    photo = request.files['file']
    print "Saving file locally"
    photo.save(secure_filename(f.filename))
    print "Successfully saved uploaded photo locally"

if __name__ == '__main__':
    # Create a controller instance
    app.run()
