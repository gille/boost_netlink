//
// Copyright (c) 2012 Magnus Gille (mgille at gmail dot com)
//
//

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>


#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "netlink.hpp"

/* I couldn't include linux/if.h */

#define IFF_LOWER_UP	0x10000

enum { max_length = 1024 };



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
