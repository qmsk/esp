#include <system_eth.h>

const char *eth_speed_str(eth_speed_t speed)
{
  switch (speed) {
    case ETH_SPEED_10M:     return "10M";
    case ETH_SPEED_100M:    return "100M";
    default:                return "?";
  }
}

const char *eth_duplex_str(eth_duplex_t duplex)
{
  switch (duplex) {
    case ETH_DUPLEX_HALF:   return "HALF";
    case ETH_DUPLEX_FULL:   return "FULL";
    default:                return "?";
  }
}
