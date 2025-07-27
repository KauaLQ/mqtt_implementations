#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/watchdog.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"
#include "lwip/dns.h"
#include "pico/time.h"

// Wi-Fi
#define WIFI_SSID "CLEUDO"
#define WIFI_PASSWORD "91898487"

// MQTT
#define MQTT_BROKER "broker.hivemq.com"
#define MQTT_BROKER_PORT 1883
#define MQTT_TOPIC "implementation/bh1750"

// BH1750 (I2C)
#define I2C_PORT_BH1750 i2c1
#define SDA_PIN_BH1750 2
#define SCL_PIN_BH1750 3
#define BH1750_ADDR 0x23

// LED
#define LED_PIN 13

// Estados globais
static mqtt_client_t *mqtt_client;
static ip_addr_t broker_ip;
static bool mqtt_connected = false;

// Funções MQTT
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
    printf("[MQTT] Mensagem recebida no tópico: %s (tamanho: %u)\n", topic, tot_len);
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    printf("[MQTT] Conteúdo: %.*s\n", len, data);

    if (strstr(data, "on") != NULL) {
        gpio_put(LED_PIN, 1);
        printf("[LED] Ligado\n");
    } else if (strstr(data, "off") != NULL) {
        gpio_put(LED_PIN, 0);
        printf("[LED] Desligado\n");
    } else {
        printf("[LED] Comando desconhecido\n");
    }
}

static void mqtt_connection_callback(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        printf("[MQTT] Conectado ao broker!\n");
        mqtt_connected = true;

        mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, NULL);

        err_t err = mqtt_subscribe(client, MQTT_TOPIC, 0, NULL, NULL);
        if (err == ERR_OK) {
            printf("[MQTT] Inscrito no tópico '%s'\n", MQTT_TOPIC);
        } else {
            printf("[MQTT] Erro ao se inscrever: %d\n", err);
        }
    } else {
        printf("[MQTT] Falha na conexão MQTT. Código: %d\n", status);
        mqtt_connected = false;
    }
}

static void dns_check_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    if (ipaddr != NULL) {
        broker_ip = *ipaddr;
        printf("[DNS] Resolvido: %s -> %s\n", name, ipaddr_ntoa(ipaddr));

        struct mqtt_connect_client_info_t ci = {
            .client_id = "pico-client",
            .keep_alive = 60,
        };

        printf("[MQTT] Conectando ao broker...\n");
        mqtt_client_connect(mqtt_client, &broker_ip, MQTT_BROKER_PORT, mqtt_connection_callback, NULL, &ci);
    } else {
        printf("[DNS] Falha ao resolver DNS para %s\n", name);
    }
}

void publish_data(float data) {
    if (!mqtt_connected) {
        printf("[MQTT] Não conectado, não publicando\n");
        return;
    }

    char message[64];
    sprintf(message, "%.2f", data);
    printf("[MQTT] Publicando: tópico='%s', mensagem='%s'\n", MQTT_TOPIC, message);

    err_t err = mqtt_publish(mqtt_client, MQTT_TOPIC, message, strlen(message), 0, 0, NULL, NULL);
    if (err != ERR_OK) {
        printf("[MQTT] Erro ao publicar: %d\n", err);
    }
}

// BH1750
void bh1750_init() {
    uint8_t power_on_cmd = 0x01;
    uint8_t cont_high_res_mode = 0x10;

    i2c_write_blocking(I2C_PORT_BH1750, BH1750_ADDR, &power_on_cmd, 1, false);
    sleep_ms(10);
    i2c_write_blocking(I2C_PORT_BH1750, BH1750_ADDR, &cont_high_res_mode, 1, false);
}

float bh1750_read_lux() {
    uint8_t buffer[2];
    int ret = i2c_read_blocking(I2C_PORT_BH1750, BH1750_ADDR, buffer, 2, false);
    if (ret != 2) return -1.0;
    uint16_t raw = (buffer[0] << 8) | buffer[1];
    return raw / 1.2;
}

int main() {
    stdio_init_all();
    sleep_ms(3000);
    printf("\n=== Iniciando MQTT Sensor BH1750 ===\n");

    watchdog_enable(10000, 1); // Watchdog: 10s

    // Inicializa Wi-Fi
    if (cyw43_arch_init()) {
        printf("Erro na inicialização do Wi-Fi\n");
        return -1;
    }
    cyw43_arch_enable_sta_mode();

    printf("[Wi-Fi] Conectando...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("[Wi-Fi] Falha na conexão Wi-Fi\n");
        return -1;
    } else {
        printf("[Wi-Fi] Conectado com sucesso!\n");
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    }

    // I2C do BH1750
    i2c_init(I2C_PORT_BH1750, 100 * 1000);
    gpio_set_function(SDA_PIN_BH1750, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN_BH1750, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN_BH1750);
    gpio_pull_up(SCL_PIN_BH1750);
    bh1750_init();

    // LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // MQTT
    mqtt_client = mqtt_client_new();
    err_t err = dns_gethostbyname(MQTT_BROKER, &broker_ip, dns_check_callback, NULL);
    if (err == ERR_OK) {
        dns_check_callback(MQTT_BROKER, &broker_ip, NULL);
    } else if (err == ERR_INPROGRESS) {
        printf("[DNS] Resolvendo...\n");
    } else {
        printf("[DNS] Erro ao resolver: %d\n", err);
        return -1;
    }

    // Temporizador para reconexão MQTT
    absolute_time_t last_mqtt_attempt = get_absolute_time();
    const int MQTT_RECONNECT_INTERVAL_MS = 5000;

    while (true) {
        cyw43_arch_poll();
        watchdog_update();

        // Reconectar Wi-Fi
        if (!cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP) {
            printf("[Wi-Fi] Desconectado. Tentando reconectar...\n");
            cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000);
        }

        // Reconectar MQTT se necessário
        if (!mqtt_connected &&
            cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP &&
            absolute_time_diff_us(last_mqtt_attempt, get_absolute_time()) > MQTT_RECONNECT_INTERVAL_MS * 1000) {

            printf("[MQTT] Tentando reconectar...\n");
            err_t err = dns_gethostbyname(MQTT_BROKER, &broker_ip, dns_check_callback, NULL);
            if (err == ERR_OK) {
                dns_check_callback(MQTT_BROKER, &broker_ip, NULL);
            }
            last_mqtt_attempt = get_absolute_time();
        }

        // Leitura e publicação
        float lux = bh1750_read_lux();
        publish_data(lux);

        // Delay dividido para manter rede ativa
        for (int i = 0; i < 100; i++) {
            cyw43_arch_poll();
            watchdog_update();
            sleep_ms(10);
        }
    }

    cyw43_arch_deinit();
    return 0;
}