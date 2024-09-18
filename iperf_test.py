#!/usr/bin/env python

from mininet.cli import CLI
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.topo import Topo
from mininet.log import setLogLevel
import time
import re

class TestTopology(Topo):
    def __init__(self, **opts):
        Topo.__init__(self, **opts)

        # Hosts
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        h3 = self.addHost('h3')
        h4 = self.addHost('h4')
        h5 = self.addHost('h5')
        h6 = self.addHost('h6')
        h7 = self.addHost('h7')
        h8 = self.addHost('h8')
        h9 = self.addHost('h9')
        h10 = self.addHost('h10')

        # Switches
        s1 = self.addSwitch('s1')
        s2 = self.addSwitch('s2')
        s3 = self.addSwitch('s3')
        s4 = self.addSwitch('s4')
        s5 = self.addSwitch('s5')
        s6 = self.addSwitch('s6')

        # Links from host to switch
        self.addLink(h1, s1)
        self.addLink(h2, s1)
        self.addLink(h5, s1)
        self.addLink(h3, s2)
        self.addLink(h4, s3)
        self.addLink(h7, s4)
        self.addLink(h8, s5)
        self.addLink(h6, s6)
        self.addLink(h9, s6)
        self.addLink(h10, s6)

        # Links between switches (L1 - L5)
        self.addLink(s1, s2, bw=20, delay='40ms')  # L1
        self.addLink(s2, s3, bw=40, delay='10ms')  # L2
        self.addLink(s2, s4, bw=30, delay='30ms')  # L3
        self.addLink(s3, s5, bw=25, delay='5ms')   # L4
        self.addLink(s5, s6, bw=25, delay='5ms')   # L5

def parse_output(output, is_server):
    """ Helper function to parse bytes and rate from iperfer output """
    if is_server:
        bytes_match = re.search(r'Received: (\d+)', output)
    else:
        bytes_match = re.search(r'Sent: (\d+)', output)
    rate_match = re.search(r'Rate: ([\d.]+) Mbps', output)

    if bytes_match and rate_match:
        bytes_received = int(bytes_match.group(1))
        rate = float(rate_match.group(1))
        return bytes_received, rate
    else:
        return None, None

def run_test():
    # Set up Mininet and start network
    setLogLevel('info')
    topo = TestTopology()
    net = Mininet(topo=topo, link=TCLink, autoSetMacs=True, autoStaticArp=True)
    net.start()

    h1 = net.get('h1')
    h10 = net.get('h10')

    print("Starting iperfer server on h10...")
    h10.cmd('iperf -s > /tmp/iperf_server.txt &')  # Start iperf server in background
    h10.cmd('./iperfer -s -p 5002 > /tmp/iperfer_server.txt &')  # Start iperfer server in background

    time.sleep(2)  # Give the servers time to start

    print("Running iperfer client from h1 to h10...")
    h1.cmd('./iperfer -c -h 10.0.0.10 -p 5002 -t 20 > /tmp/iperfer_client.txt')
    print("Running iPerf client from h1 to h10...")
    h1.cmd('iperf -c 10.0.0.10 -t 20 > /tmp/iperf_client.txt')

    time.sleep(2)  # Allow time for clients to finish

    # Retrieve server and client output
    iperfer_client_output = h1.cmd('cat /tmp/iperfer_client.txt')
    iperfer_server_output = h10.cmd('cat /tmp/iperfer_server.txt')

    print(f'iperfer_client_output={iperfer_client_output}')
    print(f'iperfer_server_output={iperfer_server_output}')

    # Parse and compare iperfer output
    # client_bytes, client_rate = parse_output(iperfer_client_output, False)
    # server_bytes, server_rate = parse_output(iperfer_server_output, True)

    # if client_bytes and server_bytes:
    #     print(f"iperfer Client: {client_bytes} bytes, {client_rate} Mbps")
    #     print(f"iperfer Server: {server_bytes} bytes, {server_rate} Mbps")

    #     # Check if they are within the acceptable threshold (e.g., 1%)
    #     threshold = 0.01  # 1% threshold
    #     byte_diff = abs(client_bytes - server_bytes)
    #     rate_diff = abs(client_rate - server_rate)

    #     if byte_diff <= client_bytes * threshold and rate_diff <= client_rate * threshold:
    #         print("iperfer client and server results match within threshold.")
    #     else:
    #         print("Mismatch in iperfer client and server results!")

    # else:
    #     print("Error parsing iperfer output!")

    # Parse iPerf output (optional, can compare with iperfer)
    iperf_client_output = h1.cmd('cat /tmp/iperf_client.txt')
    iperf_server_output = h10.cmd('cat /tmp/iperf_server.txt')

    print("\n\nComparing iPerf output:")
    print(f"iPerf Client: {iperf_client_output}")
    print(f"iPerf Server: {iperf_server_output}")

    # Stop the network
    net.stop()

if __name__ == '__main__':
    run_test()
