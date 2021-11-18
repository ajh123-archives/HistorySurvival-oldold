#include <server_main.hpp>
#include <net_server.hpp>

enum class CustomMsgTypes : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage,
};

class CustomServer : public hsc::net::server_interface<CustomMsgTypes>
{
public:
	CustomServer(uint16_t port, const char* address) : hsc::net::server_interface<CustomMsgTypes>(port, address)
	{

	}
protected:
	virtual bool onClientConnect(std::shared_ptr<hsc::net::connection<CustomMsgTypes>> client)
	{
		return true;
	}
};

int server_main(std::string bind_to) {
	CustomServer server(36676, bind_to.c_str());
	server.start();

	while (1)
	{
		server.update(-1);
	}
	return 0;
}