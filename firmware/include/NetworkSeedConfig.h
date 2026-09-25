#pragma once

/**
 * Optional compile-time seeds. Prefer NetworkSeedSecrets.generated.h written by
 * Build-And-Upload*.ps1 (gitignored). Fallbacks keep empty defaults for clean builds.
 */
#if __has_include("NetworkSeedSecrets.generated.h")
#include "NetworkSeedSecrets.generated.h"
#endif

#ifndef WIFI_SSID_SEED
#define WIFI_SSID_SEED ""
#endif

#ifndef WIFI_PASS_SEED
#define WIFI_PASS_SEED ""
#endif

#ifndef MICROROS_AGENT_IP_SEED
#define MICROROS_AGENT_IP_SEED ""
#endif

#ifndef MICROROS_AGENT_PORT_SEED
#define MICROROS_AGENT_PORT_SEED 8888
#endif
