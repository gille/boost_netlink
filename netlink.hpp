#ifndef NETLINK_HPP
#define NETLINK_HPP
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

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

#endif
