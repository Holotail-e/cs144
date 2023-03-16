#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

/*
 * 我觉得有一种更节省时间的实现方法就是维护一个计时器，每次使用该 arp 条目时与计时器比对，如果已经超时即将其 erase 掉，
 * 但后来想想这样占用的空间可能大大变多，所以算了
 */

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    auto iter = _arp_table.find(next_hop_ip);
    if (iter == _arp_table.end()){
        if (_flying_arp_request.find(next_hop_ip) == _flying_arp_request.end()) {
            ARPMessage message;
            message.opcode = ARPMessage::OPCODE_REQUEST;
            message.sender_ethernet_address = _ethernet_address;
            message.sender_ip_address = _ip_address.ipv4_numeric();
            message.target_ethernet_address = {}; // 由以太网帧进行广播，因此这里随意
            message.target_ip_address = next_hop_ip;
            EthernetFrame frame;
            frame.header().src = _ethernet_address;;
            frame.header().dst = ETHERNET_BROADCAST;
            frame.header().type = EthernetHeader::TYPE_ARP;
            frame.payload() = message.serialize();
            _frames_out.push(frame);
            _flying_arp_request[next_hop_ip] = 5000;
        }
        _waiting_arp_datagram.emplace_back(next_hop, dgram);
    }
    else {
        EthernetFrame frame;
        frame.header().src = _ethernet_address;;
        frame.header().dst = iter->second.mac_address;
        frame.header().type = EthernetHeader::TYPE_IPv4;
        frame.payload() = dgram.serialize();
        _frames_out.push(frame);
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    if (frame.header().dst != _ethernet_address && frame.header().dst != ETHERNET_BROADCAST)
        return nullopt;
    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram datagram;
        if (datagram.parse(frame.payload()) != ParseResult::NoError)
            return nullopt;
        return datagram;
    }
    if (frame.header().type == EthernetHeader::TYPE_ARP) {
        ARPMessage message;
        if (message.parse(frame.payload()) != ParseResult::NoError)
            return nullopt;
        // 这里需要加这一条判断，因为我们不需要对是广播但目标地址不是本地的帧进行处理
        if ((message.opcode == ARPMessage::OPCODE_REQUEST && message.target_ip_address == _ip_address.ipv4_numeric()) ||
            (message.opcode == ARPMessage::OPCODE_REPLY && message.target_ethernet_address == _ethernet_address)){
            _flying_arp_request.erase(message.sender_ip_address);
            _arp_table[message.sender_ip_address].mac_address = message.sender_ethernet_address;
            _arp_table[message.sender_ip_address].ttl = 30000;
            for (auto iter = _waiting_arp_datagram.begin(); iter != _waiting_arp_datagram.end(); ){
                if (iter->first.ipv4_numeric() == message.sender_ip_address) {
                    send_datagram(iter->second, iter->first);
                    iter = _waiting_arp_datagram.erase(iter);
                }
                else {
                    iter++;
                }
            }
        }
        if (message.opcode == ARPMessage::OPCODE_REQUEST && message.target_ip_address == _ip_address.ipv4_numeric()) {
            ARPMessage reply;
            reply.opcode = ARPMessage::OPCODE_REPLY;
            reply.sender_ethernet_address = _ethernet_address;
            reply.sender_ip_address = _ip_address.ipv4_numeric();
            reply.target_ethernet_address = message.sender_ethernet_address;
            reply.target_ip_address = message.sender_ip_address;

            EthernetFrame frame1;
            frame1.header().type = EthernetHeader::TYPE_ARP;
            frame1.header().src = _ethernet_address;
            frame1.header().dst = message.sender_ethernet_address;
            frame1.payload() = reply.serialize();
            _frames_out.push(frame1);
        }
    }
    return nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    for (auto iter = _arp_table.begin(); iter != _arp_table.end(); ) {
        if (iter->second.ttl <= ms_since_last_tick)
            iter = _arp_table.erase(iter);
        else {
            iter->second.ttl -= ms_since_last_tick;
            iter++;
        }
    }
    for (auto iter = _flying_arp_request.begin(); iter != _flying_arp_request.end(); ) {
        // 对于已经超时的要重发，这里实验指导上没有提到
        if (iter->second <= ms_since_last_tick) {
            ARPMessage message;
            message.opcode = ARPMessage::OPCODE_REQUEST;
            message.sender_ethernet_address = _ethernet_address;
            message.sender_ip_address = _ip_address.ipv4_numeric();
            message.target_ethernet_address = {};
            message.target_ip_address = iter->first;
            EthernetFrame frame;
            frame.header().src = _ethernet_address;;
            frame.header().dst = ETHERNET_BROADCAST;
            frame.header().type = EthernetHeader::TYPE_ARP;
            frame.payload() = message.serialize();
            _frames_out.push(frame);
            iter->second = 5000;
        }
        else {
            iter->second -= ms_since_last_tick;
            iter++;
        }
    }
}
