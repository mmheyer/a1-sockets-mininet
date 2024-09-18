#!/usr/bin/env python
"""
Measurement topology for EECS489, Winter 2024, Assignment 1
"""

from mininet.cli import CLI
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.topo import Topo
from mininet.log import setLogLevel
from time import sleep

class AssignmentNetworks(Topo):
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
        
def measure_latency_and_throughput(net):
    # Define link host pairs for each link (L1 - L5)
    links = {
        'L1': ('h1', 'h3'),   # Link L1 between s1 and s2
        'L2': ('h3', 'h4'),   # Link L2 between s2 and s3
        'L3': ('h3', 'h7'),   # Link L3 between s2 and s4
        'L4': ('h4', 'h8'),   # Link L4 between s3 and s5
        'L5': ('h8', 'h6')    # Link L5 between s5 and s6
    }

    for link, hosts in links.items():
        h_src, h_dst = net.get(hosts[0]), net.get(hosts[1])

        # Measure latency using ping (20 packets)
        print(f"Measuring latency on {link}...")
        h_src.cmd(f'ping -c 20 {h_dst.IP()} > latency_{link}.txt')

        # Measure throughput using iperfer
        print(f"Measuring throughput on {link}...")
        
        # Start iPerfer server on destination host
        h_dst.cmd(f'./iperfer -s -p 5001 > throughput_server.txt 2>&1 &')
        #h_dst.cmd(f'./iperfer -s -p 5001 &')
        sleep(1)  # Wait for the server to start

        # print("Successfully started server!")
        # h_src.cmd(f'ping -c 5 {h_dst.IP()} > latency.txt')
        # print(f'h_dst.IP()={h_dst.IP()}')
        # print(f'link={link}')

        # Start iperfer client on source host for 20 seconds
        h_src.cmd(f'stdbuf -oL ./iperfer -c -h {h_dst.IP()} -p 5001 -t 20 > throughput_{link}.txt 2>&1')

        # Stop the iperfer server on destination host
        h_dst.cmd('kill %iperf')
        sleep(1)  # Short wait between tests

def path_latency_and_throughput(net):
    h1 = net.get('h1')
    h10 = net.get('h10')

    # Perform latency test (ping)
    print("Running ping test between h1 and h10...")
    h1.cmd('ping -c 20 10.0.0.10 > latency_Q2.txt 2>&1')

    # Start the iPerfer server on h10
    print("Starting iperfer server on h10...")
    h10.cmd('stdbuf -oL ./iperfer -s -p 5001 > /dev/null 2>&1 &')

    # Give the server time to start
    sleep(1)

    # Run the iperfer client on h1 and save the throughput results
    print("Running iperfer client on h1...")
    h1.cmd('stdbuf -oL ./iperfer -c -h 10.0.0.10 -p 5001 -t 20 > throughput_Q2.txt 2>&1')

def two_pair_multiplexing(net):
    # Get the hosts
    h1 = net.get('h1')
    h2 = net.get('h2')
    h6 = net.get('h6')
    h9 = net.get('h9')

    # Start the servers on h6 and h9 in the background
    h6.cmd('stdbuf -oL ./iperfer -s -p 5001 > throughput_h6.txt 2>&1 &')
    h9.cmd('stdbuf -oL ./iperfer -s -p 5001 > throughput_h9.txt 2>&1 &')

    # Run latency tests (ping) between h1 -> h6 and h2 -> h9
    h1.cmd('ping -c 20 10.0.0.6 > latency_h1_h6.txt &')
    h2.cmd('ping -c 20 10.0.0.9 > latency_h2_h9.txt &')

    # Run throughput tests (iPerfer) for both pairs simultaneously
    h1.cmd('stdbuf -oL ./iperfer -c -h 10.0.0.6 -p 5001 -t 20 > throughput_h1_h6.txt 2>&1 &')
    h2.cmd('stdbuf -oL ./iperfer -c -h 10.0.0.9 -p 5001 -t 20 > throughput_h2_h9.txt 2>&1 &')

    # Wait for the tests to complete
    sleep(25)

def three_pair_multiplexing(net):
    # Get the hosts
    h1 = net.get('h1')
    h2 = net.get('h2')
    h5 = net.get('h5')
    h6 = net.get('h6')
    h9 = net.get('h9')
    h10 = net.get('h10')


    # Start the servers on h6, h9, and h10 in the background
    h6.cmd('stdbuf -oL ./iperfer -s -p 5001 > throughput_h6.txt 2>&1 &')
    h9.cmd('stdbuf -oL ./iperfer -s -p 5001 > throughput_h9.txt 2>&1 &')
    h10.cmd('stdbuf -oL ./iperfer -s -p 5001 > throughput_h10.txt 2>&1 &')

    # Run latency tests (ping) between h1 -> h6, h2 -> h9, and h5 -> h10
    h1.cmd('ping -c 20 10.0.0.6 > latency_h1_h6.txt &')
    h2.cmd('ping -c 20 10.0.0.9 > latency_h2_h9.txt &')
    h5.cmd('ping -c 20 10.0.0.10 > latency_h5_h10.txt &')

    # Run throughput tests (iPerfer) for both pairs simultaneously
    h1.cmd('stdbuf -oL ./iperfer -c -h 10.0.0.6 -p 5001 -t 20 > throughput_h1_h6.txt 2>&1 &')
    h2.cmd('stdbuf -oL ./iperfer -c -h 10.0.0.9 -p 5001 -t 20 > throughput_h2_h9.txt 2>&1 &')
    h5.cmd('stdbuf -oL ./iperfer -c -h 10.0.0.10 -p 5001 -t 20 > throughput_h5_h10.txt 2>&1 &')

    # Wait for the tests to complete
    sleep(25)

def effects_of_latency(net):
    # Get the hosts
    h1 = net.get('h1')
    h3 = net.get('h3')
    h10 = net.get('h10')
    h8 = net.get('h8')

    # Start the servers on h10 and h8 in the background
    h10.cmd('stdbuf -oL ./iperfer -s -p 5001 > throughput_h10.txt 2>&1 &')
    h8.cmd('stdbuf -oL ./iperfer -s -p 5001 > throughput_h8.txt 2>&1 &')

    # Run latency tests (ping) between h1 -> h10 and h3 -> h8
    h1.cmd('ping -c 20 10.0.0.10 > latency_h1-h10.txt &')
    h3.cmd('ping -c 20 10.0.0.8 > latency_h3-h8.txt &')

    # Run throughput tests (iPerfer) for both pairs simultaneously
    h1.cmd('stdbuf -oL ./iperfer -c -h 10.0.0.10 -p 5001 -t 20 > throughput_h1-h10.txt 2>&1 &')
    h3.cmd('stdbuf -oL ./iperfer -c -h 10.0.0.8 -p 5001 -t 20 > throughput_h3-h8.txt 2>&1 &')

if __name__ == '__main__':
    setLogLevel('info')

    # Create the network
    topo = AssignmentNetworks()
    net = Mininet(topo=topo, link=TCLink, autoSetMacs=True, autoStaticArp=True)

    # Start the network
    net.start()

    # Q1: Link Latency and Throughput
    # measure_latency_and_throughput(net)

    # Q2: Path Latency and Throughput
    # path_latency_and_throughput(net)

    # Q3: Effects of Multiplexing
    # two_pair_multiplexing(net)
    # three_pair_multiplexing(net)

    # Q4: Effects of Latency
    effects_of_latency(net)

    # Drop to CLI for manual testing if needed
    CLI(net)

    # Stop the network after measurements
    net.stop()
