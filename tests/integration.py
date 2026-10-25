"""Isolated PostgreSQL/core containers + local adapter + fake sensor/Telegram.
Usage: python3 tests/integration.py /absolute/path/to/iot-http-adapter
Requires Docker Desktop and image iot-core-task-test (docker build -t ... .).
No requests are sent to the real Telegram API. Containers are removed on exit.
"""
import base64
import json
import os
from pathlib import Path
import subprocess
import sys
import threading
import time
import urllib.error
import urllib.request
import urllib.parse
import uuid
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

received = []
telegram_fails = False


class External(BaseHTTPRequestHandler):
    def do_POST(self):
        body = json.loads(self.rfile.read(int(self.headers['Content-Length'])))
        path = urllib.parse.unquote(self.path)
        received.append((path, body))
        is_telegram = path == '/bot123:test/sendMessage'
        self.send_response(500 if is_telegram and telegram_fails else 200)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps({'ok': not telegram_fails} if is_telegram else {'status': 'accepted'}).encode())

    def log_message(self, *args):
        pass


def docker(*args, check=True):
    result = subprocess.run(['docker', *args], text=True, capture_output=True)
    if check and result.returncode:
        raise RuntimeError(result.stderr or result.stdout)
    return result


def request(origin, method, path, body=None, expected=200):
    data = None if body is None else json.dumps(body).encode()
    req = urllib.request.Request(origin + path, method=method, data=data,
                                 headers={'Content-Type': 'application/json'})
    try:
        with urllib.request.urlopen(req, timeout=10) as response:
            status, raw = response.status, response.read()
    except urllib.error.HTTPError as error:
        status, raw = error.code, error.read()
    assert status == expected, (method, path, expected, status, raw)
    return json.loads(raw) if raw and raw != b'pong' else None


def wait_until(predicate, seconds=15):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        try:
            if predicate():
                return
        except (OSError, AssertionError):
            pass
        time.sleep(.1)
    raise RuntimeError('Timed out waiting for test service/event')


def notifications():
    return [body for path, body in received if path == '/bot123:test/sendMessage']


suffix = uuid.uuid4().hex[:10]
network, db, core = ['iot-test-' + name + '-' + suffix for name in ('net', 'db', 'core')]
server = ThreadingHTTPServer(('0.0.0.0', 0), External)
threading.Thread(target=server.serve_forever, daemon=True).start()
external_port = server.server_port
adapter_origin = 'http://127.0.0.1:8080'
process = None
try:
    # Do not accidentally test against or disrupt an already running adapter.
    import socket
    with socket.socket() as probe:
        probe.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        probe.bind(('127.0.0.1', 8080))
    docker('network', 'create', network)
    migration = Path(__file__).resolve().parents[1] / 'migrations/001_schema.sql'
    docker('run', '-d', '--name', db, '--network', network,
           '-e', 'POSTGRES_PASSWORD=test-password', '-e', 'POSTGRES_USER=iot', '-e', 'POSTGRES_DB=iot_db',
           '-v', str(migration) + ':/docker-entrypoint-initdb.d/001_schema.sql:ro', 'postgres:16-alpine')
    wait_until(lambda: docker('exec', db, 'pg_isready', '-h', '127.0.0.1', '-U', 'iot', '-d', 'iot_db', check=False).returncode == 0)
    # Apply twice: migration must be safe for an existing schema too.
    for _ in range(2):
        docker('exec', db, 'psql', '-U', 'iot', '-d', 'iot_db', '-v', 'ON_ERROR_STOP=1',
               '-f', '/docker-entrypoint-initdb.d/001_schema.sql')
    docker('run', '-d', '--name', core, '--network', network, '-p', '127.0.0.1::8000',
           '-e', f'IOT_DATABASE_URL=host={db} dbname=iot_db user=iot password=test-password',
           '-e', 'IOT_ADAPTER_URL=http://host.docker.internal:8080',
           '-e', 'IOT_TELEGRAM_BOT_TOKEN=123:test',
           '-e', f'IOT_TELEGRAM_API_URL=http://host.docker.internal:{external_port}',
           'iot-core-task-test')
    port = docker('port', core, '8000').stdout.strip().rsplit(':', 1)[1]
    core_origin = 'http://127.0.0.1:' + port
    wait_until(lambda: request(core_origin, 'GET', '/health') == {'status': 'ok'})
    for algorithm in ('base64', 'aes-256-gcm'):
        received.clear()
        env = dict(os.environ, IOT_SENSOR_ALGORITHM=algorithm,
                   IOT_AES_KEY_BASE64=base64.b64encode(bytes(32)).decode(),
                   IOT_ADAPTER_HOST='0.0.0.0', IOT_CORE_URL=core_origin,
                   IOT_SENSOR_URL=f'http://127.0.0.1:{external_port}')
        process = subprocess.Popen([str(Path(sys.argv[1]).resolve())], env=env, stdout=subprocess.DEVNULL)
        wait_until(lambda: request(adapter_origin, 'GET', '/ping') is None)
        def api(method, path, body=None, expected=200):
            return request(adapter_origin, method, '/api/v1' + path, body, expected)

        user_input = {'name': 'Owner', 'email': algorithm + '@example.test', 'telegram_chat_id': '12345'}
        user = api('POST', '/users', user_input, 201)
        uid = user['id']
        api('POST', '/users', user_input, 409)
        api('POST', '/users', {'name': 'No email'}, 400)
        api('GET', '/users/nope', expected=400)
        api('GET', '/users?limit=0', expected=400)
        assert api('GET', '/users?limit=1')['count'] == 1
        assert api('GET', f'/users/{uid}')['name'] == 'Owner'
        user_input['name'] = 'Updated Owner'
        assert api('PUT', f'/users/{uid}', user_input)['name'] == 'Updated Owner'
        device_input = {'sensor_id': algorithm + '-sensor', 'name': 'Temperature', 'user_id': uid}
        device = api('POST', '/devices', device_input, 201)
        did = device['id']
        api('POST', '/devices', device_input, 409)
        api('POST', '/devices', dict(device_input, sensor_id='orphan', user_id=999999999), 409)
        api('DELETE', f'/users/{uid}', expected=409)
        assert api('GET', '/devices?limit=1')['count'] == 1
        assert api('GET', f'/devices/{did}')['sensor_id'] == device_input['sensor_id']
        update = {'name': 'Updated Device', 'user_id': uid, 'enabled': True}
        api('PUT', f'/devices/{did}', dict(update, sensor_id='changed'), 400)
        assert api('PUT', f'/devices/{did}', update)['name'] == 'Updated Device'
        # Full core -> adapter -> sensor path; echo encrypted data to test ingest.
        command = {'command': 'sample', 'value': 23.5, 'unit': 'C', 'sensor_id': 'must-be-overridden'}
        assert api('POST', f'/devices/{did}/commands', command)['status'] == 'accepted'
        envelope = [body for path, body in received if path == '/api/v1/commands'][-1]
        assert envelope['algorithm'] == algorithm
        if algorithm == 'base64':
            assert json.loads(base64.b64decode(envelope['payload']))['sensor_id'] == device_input['sensor_id']
        stored = api('POST', '/ingest', envelope, 201)
        assert stored['sensor_id'] == device_input['sensor_id']
        assert 'telegram_chat_id' not in stored
        wait_until(lambda: len(notifications()) == 1)
        assert notifications()[0]['chat_id'] == '12345'
        assert device_input['sensor_id'] in notifications()[0]['text']
        assert '23.5' in notifications()[0]['text']
        data = api('GET', '/data?limit=1')
        assert data['count'] == 1 and data['data'][0]['value'] == 23.5
        api('GET', '/data?limit=abc', expected=400)
        api('GET', '/data?limit=1001', expected=400)
        api('POST', '/ingest', dict(envelope, algorithm='unknown'), 400)
        api('POST', '/ingest', dict(envelope, payload='!!!!'), 400)
        if algorithm == 'aes-256-gcm':
            packed = bytearray(base64.b64decode(envelope['payload']))
            packed[-1] ^= 1
            api('POST', '/ingest', dict(envelope, payload=base64.b64encode(packed).decode()), 400)
        # Telegram errors never roll back or turn a stored reading into a failure.
        telegram_fails = True
        api('POST', '/ingest', envelope, 201)
        wait_until(lambda: len(notifications()) == 2)
        telegram_fails = False
        user_input['notifications_enabled'] = False
        api('PUT', f'/users/{uid}', user_input)
        api('POST', '/ingest', envelope, 201)
        api('PUT', f'/devices/{did}', dict(update, enabled=False))
        api('POST', f'/devices/{did}/commands', command, 409)
        api('POST', '/ingest', envelope, 404)
        api('DELETE', f'/devices/{did}', expected=204)
        api('GET', f'/devices/{did}', expected=404)
        api('PUT', f'/devices/{did}', update, 404)
        api('POST', '/ingest', envelope, 404)
        assert api('GET', '/data?limit=1')['count'] == 1  # history survives deletion
        api('DELETE', f'/users/{uid}', expected=204)
        api('GET', f'/users/{uid}', expected=404)
        api('DELETE', f'/users/{uid}', expected=404)
        time.sleep(.2)
        assert len(notifications()) == 2
        process.terminate()
        process.wait(timeout=5)
        process = None
        print(algorithm + ': CRUD, commands, ingest, notifications, validation passed', flush=True)
    print('All platform integration checks passed', flush=True)
except Exception:
    logs = docker('logs', core, check=False)
    print(logs.stdout + logs.stderr, file=sys.stderr)
    print('Fake upstream requests:', received, file=sys.stderr)
    raise
finally:
    if process:
        process.terminate()
        process.wait(timeout=5)
    for name in (core, db):
        result = docker('rm', '-f', name, check=False)
    docker('network', 'rm', network, check=False)
    server.shutdown()
    server.server_close()
