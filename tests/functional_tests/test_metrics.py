import subprocess
import time
import socket
from dataclasses import dataclass
import re

import pytest
@dataclass
class BackendMetric:
    host: str
    port: int
    healthy: bool
    connection_count: int

class Client:
    def __init__(self):
        self.s : socket = socket.create_connection(("localhost", 8080))
    
    def send_msg(self, s:str):
        self.s.sendall(s.encode())
    
    def recv_msg(self):
        return self.s.recv(4096).decode()

def metrics_plaintext():
    ret = None
    with socket.create_connection(("localhost", 7071), timeout=2) as s:
        s.sendall(
            b"GET /metrics HTTP/1.1\r\n"
            b"HOST: localhost\r\n"
            b"\r\n"
        )
        ret = s.recv(4096).decode()
    return ret


def get_count(text: str, search_str: str):
    for line in text.splitlines():
        if line.startswith(search_str):
            return int(line.split()[-1])
    return 0

def make_backend_metric(text: str):
    pattern = r'host="([^"]+)",port="(\d+)"\}\s+healthy=(\d+)\s+connections=(\d+)'

    match = re.search(pattern, text)

    backend_ret = None

    if match:
        host_val = match.group(1)
        port_val = int(match.group(2))
        health_stat_val = match.group(3) == '1'
        connections_val = int(match.group(4))
        backend_ret = BackendMetric(host_val,port_val,health_stat_val,connections_val)
    return backend_ret

def parse_backends(text: str):
    ret = []

    for line in text.splitlines():
        if line.startswith("backend"):
            backend = make_backend_metric(line)
            if backend is None:
                continue
            ret.append(backend)

    return ret

def parse_metrics_plaintext(metrics_text: str):
    ret = {}
    ret['active_connections'] = get_count(metrics_text, "active_connections")
    ret['registered_backends'] = get_count(metrics_text, "registered_backends")
    ret['backends'] = parse_backends(metrics_text)
    return ret

def get_metrics():
    metrics_text = metrics_plaintext() 
    ret = parse_metrics_plaintext(metrics_text)

    return ret

@pytest.fixture
def basic_system():
    procs = {}
    def start(num_backends=3): 
        procs['lb'] = subprocess.Popen(["../../lb"])
        time.sleep(1)
        for i in range(num_backends):
            procs[f"echo{i+1}"] = subprocess.Popen(["../../echo_server", f"{9001 + i}"])
            time.sleep(1)
        return procs

    yield start
    for p in procs.values():
        if p.poll() is None:
            p.terminate()
            p.wait(timeout=2)

def test_balanced_connections(basic_system):
    procs = basic_system(2)
    clients = []
    for _ in range(4):
        clients.append(Client())
    
    metrics = get_metrics()

    assert metrics['active_connections'] == 4
    assert metrics['registered_backends'] == 2

    host = '127.0.0.1'

    expected = [
        BackendMetric(host=host, port=9001, healthy=True, connection_count=2),
        BackendMetric(host=host, port=9002, healthy=True, connection_count=2),
    ]
    assert metrics['backends'] == expected

def test_teardown_backend(basic_system):
    procs = basic_system(1)
    clients = []
    for _ in range(4):
        clients.append(Client())
    
    metrics = get_metrics()

    assert metrics['active_connections'] == 4
    assert metrics['registered_backends'] == 1

    host = '127.0.0.1'

    expected = [
        BackendMetric(host=host, port=9001, healthy=True, connection_count=4),
    ]
    assert metrics['backends'] == expected

    procs['echo1'].terminate()
    time.sleep(1)

    metrics = get_metrics()
    assert metrics['active_connections'] == 0
    assert metrics['registered_backends'] == 1
    expected = [
        BackendMetric(host=host, port=9001, healthy=False, connection_count=0),
    ]
    assert metrics['backends'] == expected




def test_metrics_initial_state(basic_system): 
    procs = basic_system()
    metrics = get_metrics()
    print(metrics)
    assert metrics['active_connections'] == 0
    assert metrics['registered_backends'] == 3

    host = '127.0.0.1'
    expected = [
        BackendMetric(host=host, port=9001, healthy=True, connection_count=0),
        BackendMetric(host=host, port=9002, healthy=True, connection_count=0),
        BackendMetric(host=host, port=9003, healthy=True, connection_count=0),
    ]

    assert metrics['backends'] == expected

def test_backend_comes_back_up(basic_system):
    procs = basic_system()

    procs['echo1'].terminate()
    time.sleep(2)
    procs["echo1"] = subprocess.Popen(["../../echo_server", "9001"])
    time.sleep(2)

    metrics = get_metrics()
    host = '127.0.0.1'
    expected = [
        BackendMetric(host=host, port=9001, healthy=True, connection_count=0),
        BackendMetric(host=host, port=9002, healthy=True, connection_count=0),
        BackendMetric(host=host, port=9003, healthy=True, connection_count=0),
    ]

    assert metrics['backends'] == expected
def test_backend_goes_down(basic_system):
    procs = basic_system()

    procs['echo1'].terminate()
    time.sleep(2)

    metrics = get_metrics()
    host = '127.0.0.1'
    expected = [
        BackendMetric(host=host, port=9001, healthy=False, connection_count=0),
        BackendMetric(host=host, port=9002, healthy=True, connection_count=0),
        BackendMetric(host=host, port=9003, healthy=True, connection_count=0),
    ]

    assert metrics['backends'] == expected
