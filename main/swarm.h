#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void     swarm_init(int wifi_channel);

// Add a unicast peer by MAC string "6C:C8:40:8A:98:48".
// Returns node_id = last 3 MAC bytes (e.g., 0x8A9848).
uint32_t swarm_add_peer(const char* mac_str);

// Send to one specific node id
void     swarm_publish_telem_to(uint32_t dst_id, uint8_t pir, float range_cm);
void     swarm_publish_alert_to(uint32_t dst_id, uint8_t reason, float range_cm);

// Convenience multicast
void     swarm_publish_telem_multicast(const uint32_t* ids, int n, uint8_t pir, float range_cm);
void     swarm_publish_alert_multicast(const uint32_t* ids, int n, uint8_t reason, float range_cm);

// No-op placeholder (kept for future timers)
void     swarm_poll(void);

#ifdef __cplusplus
}
#endif
