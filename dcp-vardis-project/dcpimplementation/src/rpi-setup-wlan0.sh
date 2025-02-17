#!/usr/bin/bash

if (($#ARGV+1) ne 1)
{
    print "One parameter expected: last byte of 192.168.144.x IP address.\nExiting.\n";
    exit;
}


sudo killall wpa_supplicant
sudo ip link set wlan0 down
sleep 1
sudo iwconfig wlan0 mode ad-hoc channel 1 essid "DroneAdHoc"
sleep 1
sudo ip link set wlan0 up
sleep 1
sudo ip addr add 192.168.144.$1/24 dev wlan0
sleep 1
ip link
