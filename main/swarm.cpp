#include "swarm.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_now.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstdint>
#include <cstring>
#include <cstdio> 

//could've used constexpr-variable that can be complied at runtime 
#define MAX_PEERS 8
static const char *TAG = "AEGIS SWARM";
static std::uint32_t my_id = 0; 
static std::uint32_t seqno = 0; 

//calling the function so it compiles 
static void on_recv(const esp_now_recv_info_t* info, const uint8_t* data, int len);



//Creating unique identifier using MAC address
static inline uint32_t id_from_mac(const uint8_t m[6]){
//Shifiting 3 bytes out of the six to create a unqiue address
    return (m[3]<<16 ) | (m[4] << 8 ) | m[5];
}


// Parses a hexadecimal MAC address from a string.
static bool parse_mac(const char*s , uint8_t out[6]){
    return sscanf(s,"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",&out[0],&out[1],&out[2],&out[3],&out[4],&out[5])==6;
}

//creating a table to add peers
struct Peer {
    bool used = false;
    uint32_t id = 0;
    uint8_t mac[6]{};
};

static Peer peers[MAX_PEERS];  


static const uint8_t* mac_for_id(uint32_t id) {
    for (int i = 0; i < MAX_PEERS; i++)
        if (peers[i].used && peers[i].id==id) return peers[i].mac;
    return nullptr;
}

// wifi setup no IP/TCP
static void wifi_sta(int channel){
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();

    esp_wifi_init(&wifi_config);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_start();
    ESP_ERROR_CHECK(esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE)); // then set channel
}

// ------message packet--------
enum msg_type_t : uint8_t {
    MSG_TELEM = 1,
    MSG_ALERT = 2
};

// acts like a mini-netweeok protocol header + payload
// first 5 -> "header"
//union -> "payload" chosen based on msg_type_t
struct __attribute__((packed)) packet {
    msg_type_t type; // determine which union u is active 
    uint8_t  ttl; // limit the packets life span
    uint32_t src_id; //where the source node come from 
    uint32_t dst_id; //destination for nodes unique identifier
    uint32_t seq; // sequence number- increments every time a packet is sent  
    
    // creating two structures one for telemetry data and alert data
    union {
        struct { uint8_t pir; float range_cm; } telem;
        struct { uint8_t reason; float range_cm; } alert;
    } u;
};


uint32_t swarm_add_peer(const char* mac_str) {
    uint8_t mac[6];//holds the binary form of the MAC after parsing 

    if(!parse_mac(mac_str, mac)){
        ESP_LOGE(TAG, "bad: MAC %s", mac_str);
        return 0;
    }

    //creating a ESP-NOW peer struct (like a config)
    esp_now_peer_info_t p = {}; 
        std::memcpy(p.peer_addr, mac, 6); //copying the MAC address
        p.ifidx = WIFI_IF_STA; 
        p.channel = 0;
        p.encrypt = false;
    
    // registers peer w/ ESP-NOW
    esp_err_t e = esp_now_add_peer(&p);//adds peer to the wifi driver interal table 

    if (e!=ESP_OK && e!=ESP_ERR_ESPNOW_EXIST) {  
        ESP_LOGE(TAG, "esp_now_add_peer; %s", esp_err_to_name(e));
    }

    uint32_t id = id_from_mac(mac);

    //already present, update the MAC and return 
    for(int i = 0; i < MAX_PEERS; i++) {
        if(peers[i].used && peers[i].id == id){
            std::memcpy(peers[i].mac, mac, 6);
            ESP_LOGI(TAG, "peer updated id=%06lx mac=%s", (unsigned long)id, mac_str);
            return id;
        }
    }

    //find a free slot in the table to add peer
    for(int i = 0; i< MAX_PEERS; i++){
        if(!peers[i].used){
            peers[i].used = true;
            peers[i].id = id;

            std::memcpy(peers[i].mac, mac, 6);
            ESP_LOGI(TAG, "peer added id=%06lx mac=%s", (unsigned long)id, mac_str);
            return id;               
        }
    }
    //table full
    ESP_LOGW(TAG, "peer table full (MAX_PEERS=%d), ignoring %s", MAX_PEERS, mac_str);
    return 0;
}


void swarm_init(int wifi_channel){
    wifi_sta(wifi_channel);
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_recv));

    uint8_t mac[6]; esp_wifi_get_mac(WIFI_IF_STA, mac);
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, mac));  // real MAC now
    my_id = id_from_mac(mac);

    ESP_LOGI(TAG,"up id=%06lx ch=%d MAC=%02x:%02x:%02x:%02x:%02x:%02x", (unsigned long)my_id, wifi_channel, mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}


static void on_recv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
    const uint8_t* mac = info->src_addr;// sender's mac 

    //1. Validation 
    if (len !=sizeof(packet)) return; //checks if the size of the recieved data matches the expected size of a packet struct
    
    //2. Deserialization 
    struct packet p = {}; // new empty packt obeject is created on the stack
    std::memcpy(&p, data, sizeof(p));// copies raw incoming data into the packet p structure

    //3. Filtering 
    if(p.dst_id != my_id) return; //if the packet is not addressed to the local decive we ignore the message
    //Processing pt.1 need static cast to convert the enum value to its underlying integer type for the switch cases 
        switch (static_cast<std::uint8_t>(p.type)) {
            case static_cast<std::uint8_t>(msg_type_t::MSG_ALERT):
                ESP_LOGI(TAG, "ALERT for me from %06lx reason=%u dist=%.1f", static_cast<unsigned long>(p.src_id), p.u.alert.reason, p.u.alert.range_cm);
                break;
    //4. Processing pt.2
            case static_cast<std::uint8_t>(msg_type_t::MSG_TELEM):
                ESP_LOGD(TAG, "TELEM for me from %06lx pir=%u d=%.1f", static_cast<unsigned long>(p.src_id), p.u.telem.pir, p.u.telem.range_cm);
                break;
                default:break;
    }
}

static void send_pkt_to(uint32_t dst_id, const packet& pkt){
    const uint8_t* mac = mac_for_id(dst_id);
    if (!mac) { ESP_LOGW(TAG,"no MAC for dst_id=%06lx", (unsigned long)dst_id); return; }
    esp_err_t e = esp_now_send(mac, (const uint8_t*)&pkt, sizeof(pkt));
    if (e!=ESP_OK) ESP_LOGE(TAG,"send: %s", esp_err_to_name(e));
}

void swarm_publish_telem_to(uint32_t dst_id, uint8_t pir, float range_cm){
    packet t{}; 
        t.type=MSG_TELEM; 
        t.ttl=0; t.src_id=my_id; 
        t.dst_id=dst_id; 
        t.seq = seqno++;
        t.u.telem.pir = pir; 
        t.u.telem.range_cm = range_cm;
    send_pkt_to(dst_id, t);
}

void swarm_publish_alert_to(uint32_t dst_id, uint8_t reason, float range_cm){
    packet a{}; 
        a.type=MSG_ALERT;
        a.ttl=0; 
        a.src_id=my_id; 
        a.dst_id=dst_id; 
        a.seq = seqno++;
        a.u.alert.reason=reason; 
        a.u.alert.range_cm=range_cm;
    send_pkt_to(dst_id, a);
}

void swarm_publish_telem_multicast(const uint32_t* ids, int n, uint8_t pir, float range_cm){
    for (int i=0;i<n;i++) swarm_publish_telem_to(ids[i], pir, range_cm);
}

void swarm_publish_alert_multicast(const uint32_t* ids, int n, uint8_t reason, float range_cm){
    for (int i=0;i<n;i++) swarm_publish_alert_to(ids[i], reason, range_cm);
}
