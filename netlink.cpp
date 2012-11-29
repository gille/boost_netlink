 //
// blocking_udp_echo_client.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>


#include <linux/netlink.h>
#include <linux/rtnetlink.h>

/* I couldn't include linux/if.h */

#define IFF_LOWER_UP	0x10000

enum { max_length = 1024 };


template <typename Protocol>
class nl_endpoint
{
private:
    sockaddr_nl sockaddr;
public:
    /// The protocol type associated with the endpoint.
    typedef Protocol protocol_type;
    typedef boost::asio::detail::socket_addr_type data_type;
    
    
    /// Default constructor.
    nl_endpoint()
    {
	sockaddr.nl_family = PF_NETLINK;
	sockaddr.nl_groups = 0;
	sockaddr.nl_pid = getpid();	
    }
    
    /// Construct an endpoint using the specified path name.
    nl_endpoint(int group, int pid=getpid())
    {
	sockaddr.nl_family = PF_NETLINK;
	sockaddr.nl_groups = group;
	sockaddr.nl_pid = pid;
	std::cout << "Called my constructor!!"; 
    }
    
    /// Copy constructor.
    nl_endpoint(const nl_endpoint& other)
    {
	sockaddr = other.sockaddr;
    }
    
    /// Assign from another endpoint.
    nl_endpoint& operator=(const nl_endpoint& other)
    {
	sockaddr = other.sockaddr;
	return *this;
    }
    
    /// The protocol associated with the endpoint.
    protocol_type protocol() const
    {
	return protocol_type();
    }
    
    /// Get the underlying endpoint in the native type.
    data_type* data()
    {
	return &sockaddr;
    }
    
    /// Get the underlying endpoint in the native type.
    const data_type* data() const
    {
	return (struct sockaddr*)&sockaddr;
    }
    
    /// Get the underlying size of the endpoint in the native type.
    std::size_t size() const
    {
	return sizeof(sockaddr);
    }
    
    /// Set the underlying size of the endpoint in the native type.
    void resize(std::size_t size)
    {
	/* nothing we can do here */
    }
    
    /// Get the capacity of the endpoint in the native type.
    std::size_t capacity() const
    {
	return sizeof(sockaddr);
    }
    
    /// Compare two endpoints for equality.
    friend bool operator==(const nl_endpoint<Protocol>& e1,
			   const nl_endpoint<Protocol>& e2)
    {
	return e1.sockaddr == e2.sockaddr;
    }
    
    /// Compare two endpoints for inequality.
    friend bool operator!=(const nl_endpoint<Protocol>& e1,
			   const nl_endpoint<Protocol>& e2)
    {
	return !(e1.sockaddr == e2.sockaddr);
    }
    
    /// Compare endpoints for ordering.
    friend bool operator<(const nl_endpoint<Protocol>& e1,
			  const nl_endpoint<Protocol>& e2)
    {
	return e1.sockaddr < e2.sockaddr;
    }
    
    /// Compare endpoints for ordering.
    friend bool operator>(const nl_endpoint<Protocol>& e1,
			  const nl_endpoint<Protocol>& e2)
    {
	return e2.sockaddr < e1.sockaddr;
    }
    
    /// Compare endpoints for ordering.
    friend bool operator<=(const nl_endpoint<Protocol>& e1,
			   const nl_endpoint<Protocol>& e2)
    {
	return !(e2 < e1);
    }
    
    /// Compare endpoints for ordering.
    friend bool operator>=(const nl_endpoint<Protocol>& e1,
			   const nl_endpoint<Protocol>& e2)
    {
	return !(e1 < e2);
    }
};


class nl_protocol
{
private:
    int proto; 
public:
    nl_protocol() {
	proto = 0;
    }
    nl_protocol(int proto) {
	this->proto = proto;
    }
    /// Obtain an identifier for the type of the protocol.
    int type() const
    {
	return SOCK_RAW;
    }
    
    /// Obtain an identifier for the protocol.
    int protocol() const
    {
	/* FIXME */
	return proto;
    }
    
    /// Obtain an identifier for the protocol family.
    int family() const
    {
	return PF_NETLINK;
    }
    
    typedef nl_endpoint<nl_protocol> endpoint;
    typedef boost::asio::basic_raw_socket<nl_protocol> socket;
};


void handle_netlink(struct nlmsghdr *nlm);

int main(int argc, char* argv[])
{
    try
    {
	boost::asio::io_service io_service;
	boost::asio::basic_raw_socket<nl_protocol> s(io_service ); 
	
	s.open(nl_protocol(NETLINK_ROUTE));
	s.bind(nl_endpoint<nl_protocol>(RTMGRP_LINK)); 

	char buffer[max_length];
	int bytes;

	while((bytes=s.receive(boost::asio::buffer(buffer, max_length)))) {
	    struct nlmsghdr *nlm;
	    std::cout << "got a buffer\n"; 
	    
	    for (nlm = (struct nlmsghdr *)buffer;
		 NLMSG_OK(nlm, (size_t)bytes);
		 nlm = NLMSG_NEXT(nlm, bytes))
	    {
		handle_netlink(nlm);
	    }
	}
    }
    catch (std::exception& e)
    {
	std::cerr << "Exception: " << e.what() << "\n";
    }
    
    return 0;
}


void handle_netlink(struct nlmsghdr *nlm) {
    int len;
    char ifname[IF_NAMESIZE + 1];
    ifinfomsg *ifi;
    rtattr *rta;
    
    if (nlm->nlmsg_type == RTM_NEWLINK) {
	len = nlm->nlmsg_len - sizeof(*nlm);
	if ((size_t)len < sizeof(*ifi)) {
	    errno = EBADMSG;
	    return;
	}
	ifi = (ifinfomsg*)NLMSG_DATA(nlm);
	if (ifi->ifi_flags & IFF_LOOPBACK)
	    return;
	
	rta = (rtattr *) ((char *)ifi + NLMSG_ALIGN(sizeof(*ifi)));
	len = NLMSG_PAYLOAD(nlm, sizeof(*ifi));
	*ifname = '\0';
	while (RTA_OK(rta, len)) {
	    switch (rta->rta_type) {
	    case IFLA_IFNAME:
		strncpy(ifname, (char*)RTA_DATA(rta), sizeof(ifname));
		break;
	    }
	    rta = RTA_NEXT(rta, len);	    
	}
    }
    if (nlm->nlmsg_type == RTM_NEWLINK)
	len = ifi->ifi_change == ~0U ? 1 : 0;
    
    std::cout << "Interface " << ifname << " changed status, now: ";
    if((ifi->ifi_flags&IFF_LOWER_UP)==IFF_LOWER_UP)
	std::cout << " Up" << std::endl;
    else
	std::cout << " Down" << std::endl;    
}
