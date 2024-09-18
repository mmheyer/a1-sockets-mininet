#!/usr/bin/env python

from mininet.net import Mininet
from mininet.topo import Topo
from mininet.cli import CLI
from mininet.node import OVSController
from mininet.link import TCLink
import time
import subprocess

class CustomTopology(Topo):
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


def run_iperf_test(net, h_client, h_server):
    """ Runs iPerf test between two hosts """
    print("Starting iPerf server on {}".format(h_server.name))
    h_server.cmd('iperf -s &')  # Start iPerf server in the background

    print("Running iPerf client on {}".format(h_client.name))
    result = h_client.cmd('iperf -c {} -t 20'.format(h_server.IP()))  # Run iPerf client
    print("iPerf Result:\n", result)

    return result


def run_iperfer_test(net, h_client, h_server):
    """ Runs iperfer test between two hosts """
    print("Starting iperfer server on {}".format(h_server.name))
    h_server.cmd('stdbuf -oL ./iperfer -s -p 5002 > iperfer_server_output.txt 2>&1 &')  # Start iperfer server in the background

    print("Running iperfer client on {}".format(h_client.name))
    result = h_client.cmd('./iperfer -c -h {} -p 5002 -t 20'.format(h_server.IP()))  # Run iperfer client
    print("iperfer Result:\n", result)

    return result


def run():
    # Create the custom topology
    topo = CustomTopology()

    # Initialize the network with OVS controller
    net = Mininet(topo=topo, controller=OVSController, link=TCLink)

    # Start the network
    net.start()

    # Select hosts for testing
    h1 = net.get('h1')
    h6 = net.get('h6')

    # Run iPerf test between h1 and h6
    print("=== Running iPerf Test ===")
    iperf_result = run_iperf_test(net, h1, h6)

    # Run iperfer test between h1 and h6
    print("=== Running iperfer Test ===")
    iperfer_result = run_iperfer_test(net, h1, h6)

    # Save the results to a file
    with open('iperf_vs_iperfer_results.txt', 'w') as f:
        f.write("=== iPerf Test Result ===\n")
        f.write(iperf_result)
        f.write("\n=== iperfer Test Result ===\n")
        f.write(iperfer_result)

    # Stop the network
    net.stop()


if __name__ == '__main__':
    # Run the Mininet network and tests
    run()
