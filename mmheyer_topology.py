#!/usr/bin/env python
"""
Custom topology for EECS489, Winter 2024, Assignment 1
"""

from mininet.cli import CLI
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.topo import Topo
from mininet.log import setLogLevel

class AssignmentNetworks(Topo):
    def __init__(self, **opts):
        Topo.__init__(self, **opts)
        
        # Add hosts
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        h3 = self.addHost('h3')
        h4 = self.addHost('h4')
        h5 = self.addHost('h5')

        # Add switches
        s1 = self.addSwitch('s1')
        s2 = self.addSwitch('s2')
        s3 = self.addSwitch('s3')
        s4 = self.addSwitch('s4')
        s5 = self.addSwitch('s5')

        # Add links between hosts and switches
        self.addLink(h1, s1, bw=10)  # bw = bandwidth in Mbps
        self.addLink(h2, s2, bw=10)
        self.addLink(h3, s3, bw=10)
        self.addLink(h4, s4, bw=10)
        self.addLink(h5, s5, bw=10)

        # Add links between switches (forming a mesh-like topology)
        self.addLink(s1, s2, bw=20, delay='10ms')
        self.addLink(s1, s3, bw=20, delay='10ms')
        self.addLink(s2, s4, bw=20, delay='10ms')
        self.addLink(s3, s5, bw=20, delay='10ms')
        self.addLink(s4, s5, bw=20, delay='10ms')

if __name__ == '__main__':
    setLogLevel('info')

    # Create the network
    topo = AssignmentNetworks()
    net = Mininet(topo=topo, link=TCLink, autoSetMacs=True, autoStaticArp=True)

    # Start the network
    net.start()

    # Drop to CLI for manual testing if needed
    CLI(net)

    # Stop the network after measurements
    net.stop()
