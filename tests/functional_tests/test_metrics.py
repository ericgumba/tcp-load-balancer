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
    try:
        procs = []
        lb = subprocess.Popen(["../../lb"])
        time.sleep(1)
        echo1 = subprocess.Popen(["../../echo_server", "9001"])
        echo2 = subprocess.Popen(["../../echo_server", "9002"])
        echo3 = subprocess.Popen(["../../echo_server", "9003"])
        time.sleep(1)
        procs += [lb, echo1, echo2, echo3]

        yield
    finally:
        for p in procs:
            p.terminate()


def test_metrics_initial_state(basic_system):

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
    