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
	CustomServer(uint16_t nPort) : hsc::net::server_interface<CustomMsgTypes>(nPort)
	{

	}
protected:
	virtual bool onClientConnect(std::shared_ptr<hsc::net::connection<CustomMsgTypes>> client)
	{
		return true;
	}
};

int server_main(void) {
	CustomServer server(60000);
	server.start();

	while (1)
	{
		server.update(-1);
	}
	return 0;
}