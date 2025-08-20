#include "socket/server.hpp"

/**
 * Sim, eu odeio C++, mas ao mesmo tempo, eu amo as features, principalmente strings.
 */
int main()
{
	Socket::InitServer("127.0.0.1", 8080);

	return 0;
}