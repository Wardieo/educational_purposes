# server.py
from flask import Flask, request, jsonify
import base64

app = Flask(__name__)

@app.route('/js/lib/srv/api.php')
def api():
    info = request.args.get('info', '')
    ip = request.args.get('ip', '')
    # You can decode and log info/ip if you want
    # info_decoded = base64.b64decode(info)
    # ip_decoded = base64.b64decode(ip)
    # Return fake credentials for testing
    return jsonify({
        "MRU_ID": "testid123",
        "MRU_UDP": "testuser",
        "MRU_PDP": "testpass"
    })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000)