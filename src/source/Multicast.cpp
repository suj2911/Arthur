//
// Created by VIKLOD on 26-02-2023.
//

#include "Multicast.hpp"

#include "DataFeed/CentralFeed.hpp"

#include <boost/asio/detail/socket_option.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/socket_base.hpp>

#include <iostream>

MulticastReceiver::MulticastReceiver(boost::asio::io_context& io_context_, MarketEventQueueT& queue_)
    : CentralFeed(queue_), _socket(io_context_) {
}

void MulticastReceiver::ReceiverFrom(const boost::system::error_code& errorCode_, size_t size_) {
    if (!errorCode_) {
        Process(size_);
        Read();
    }
}

void MulticastReceiver::BindMc(const std::string& address_, int port_, const std::string& multicast_) {
    printf("Try to join address_ %s multicast_ %s port %d\n", address_.c_str(), multicast_.c_str(), port_);

    _endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(address_), port_);

    _socket.open(_endpoint.protocol());
    std::cout << "Socket opened on port " << port_ << "\n";
    _socket.set_option(boost::asio::socket_base::reuse_address(true));
    std::cout << "Set socket to reuse address\n";
    _socket.bind(_endpoint);
    std::cout << "Socket bound to " << _endpoint.address().to_string() << ":" << port_ << "\n";

    // Join the multicast group
    boost::asio::ip::address    multicast_addr  = boost::asio::ip::make_address(multicast_);
    boost::asio::ip::address_v4 local_interface = boost::asio::ip::make_address_v4(address_);
    _socket.set_option(boost::asio::ip::multicast::join_group(multicast_addr.to_v4(), local_interface));
    std::cout << "Joined multicast group " << multicast_ << " on interface " << address_ << "\n";
}

void MulticastReceiver::Read() {
    _socket.async_receive_from(boost::asio::buffer(_buffer, 512), _senderEndpoint, [this](const boost::system::error_code& errorCode_, size_t size_) 
    {
        ReceiverFrom(errorCode_, size_);
    });
}
