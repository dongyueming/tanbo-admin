from flask import Flask, request, jsonify, send_from_directory, session
from flask_cors import CORS
import json, os, hashlib, secrets
from datetime import datetime

app = Flask(__name__, static_folder=None)
app.secret_key = os.environ.get('SECRET_KEY', secrets.token_hex(32))
CORS(app, supports_credentials=True)

DATA_DIR = os.environ.get('DATA_DIR', 'data')
TYPES = ['batches', 'strands', 'sales', 'losses', 'expenses']

# 账号
ACCOUNTS = {
    '17739465409': 'dongyueming123',
    '18768335998': 'zzzmmmkkk323232'
}

def load_data(t):
    path = os.path.join(DATA_DIR, f'{t}.json')
    if not os.path.exists(path):
        return []
    with open(path, 'r', encoding='utf-8') as f:
        return json.load(f)

def save_data(t, data):
    os.makedirs(DATA_DIR, exist_ok=True)
    path = os.path.join(DATA_DIR, f'{t}.json')
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(data, f, ensure_ascii=False, indent=2)

def get_next_id(t):
    data = load_data(t)
    return max([item['id'] for item in data], default=0) + 1

def login_required(f):
    from functools import wraps
    @wraps(f)
    def decorated(*args, **kwargs):
        if 'user' not in session:
            return jsonify({'error': 'unauthorized'}), 401
        return f(*args, **kwargs)
    return decorated

@app.route('/api/health')
def health():
    return jsonify({'status': 'ok'})

@app.route('/api/login', methods=['POST'])
def login():
    body = request.get_json()
    phone = body.get('phone', '')
    password = body.get('password', '')
    if phone in ACCOUNTS and ACCOUNTS[phone] == password:
        session['user'] = phone
        return jsonify({'user': phone})
    return jsonify({'error': 'wrong phone or password'}), 401

@app.route('/api/logout', methods=['POST'])
def logout():
    session.pop('user', None)
    return jsonify({'ok': True})

@app.route('/api/me')
def me():
    if 'user' not in session:
        return jsonify({'user': None})
    return jsonify({'user': session['user']})

@app.route('/api/state')
@login_required
def state():
    result = {}
    for t in TYPES:
        result[t] = load_data(t)
    return jsonify(result)

@app.route('/api/items/<t>', methods=['GET'])
@login_required
def list_items(t):
    if t not in TYPES:
        return jsonify({'error': 'unknown type'}), 400
    return jsonify(load_data(t))

@app.route('/api/items/<t>', methods=['POST'])
@login_required
def create_item(t):
    if t not in TYPES:
        return jsonify({'error': 'unknown type'}), 400
    data = load_data(t)
    body = request.get_json()
    body['id'] = get_next_id(t)
    body['createdAt'] = datetime.now().isoformat()
    data.append(body)
    save_data(t, data)
    return jsonify(body), 201

@app.route('/api/items/<t>/<int:item_id>', methods=['PUT'])
@login_required
def update_item(t, item_id):
    if t not in TYPES:
        return jsonify({'error': 'unknown type'}), 400
    data = load_data(t)
    body = request.get_json()
    for i, item in enumerate(data):
        if item['id'] == item_id:
            body['id'] = item_id
            data[i] = body
            save_data(t, data)
            return jsonify(body)
    return jsonify({'error': 'not found'}), 404

@app.route('/api/items/<t>/<int:item_id>', methods=['DELETE'])
@login_required
def delete_item(t, item_id):
    if t not in TYPES:
        return jsonify({'error': 'unknown type'}), 400
    data = load_data(t)
    data = [item for item in data if item['id'] != item_id]
    save_data(t, data)
    return jsonify({'ok': True})

# 静态文件
DIST_DIR = 'dist'

@app.route('/')
def index():
    return send_from_directory(DIST_DIR, 'index.html')

@app.route('/<path:path>')
def static_files(path):
    if os.path.exists(os.path.join(DIST_DIR, path)):
        return send_from_directory(DIST_DIR, path)
    return send_from_directory(DIST_DIR, 'index.html')

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=int(os.environ.get('PORT', 8080)))
