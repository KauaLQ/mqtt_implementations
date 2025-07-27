<img width=100% src="https://capsule-render.vercel.app/api?type=waving&color=983AD6&height=120&section=header"/>

# 🌐 MQTT Sensor + Controle via BitDogLab

Este projeto foi desenvolvido como parte das atividades da **Residência Tecnológica em Software Embarcado – EMBARCATECH**, utilizando a placa **BitDogLab** e o protocolo **MQTT** para criar um sistema de **monitoramento de luminosidade** e **controle remoto de LED** via nuvem.

## Descrição do Projeto

O projeto consiste em dois principais objetivos integrados:

1. **Monitoramento com Sensor BH1750:**
   A placa BitDogLab lê continuamente os dados de um sensor de luminosidade BH1750 via I²C e envia essas informações, a cada segundo, para um broker MQTT acessado pela internet.

2. **Controle Remoto de LED via MQTT:**
   O sistema também escuta comandos recebidos do broker MQTT (como `"on"` ou `"off"`) e os interpreta para ligar ou desligar um LED conectado à placa.

Além disso, como desafio proposto, foi utilizada uma **infraestrutura de nuvem** através do servidor **MQTTX (broker.hivemq.com)** para gerenciar as mensagens MQTT.

## Funcionalidades

* 📤 **Publicação periódica** dos dados do sensor BH1750 via MQTT.
* 📥 **Recepção e interpretação de comandos** MQTT para controle de um LED.
* 🔄 **Reconexão automática** com a rede e broker, em caso de falha.
* ⏱️ **Watchdog Timer ativado** para garantir resiliência contra travamentos e quedas de conexão.

## Tecnologias e Ferramentas

* **Placa:** BitDogLab (RP2040)
* **Firmware:** C Bare Metal (com Pico SDK)
* **Sensor:** BH1750 (I²C)
* **Protocolo de rede:** MQTT (via lwIP)
* **Broker MQTT:** MQTTX / HiveMQ (`broker.hivemq.com`)
* **Wi-Fi:** CYW43 Driver (embutido na BitDogLab)
* **Watchdog:** Hardware watchdog de 10 segundos
* **IDE recomendada:** VS Code com SDK do RP2040 configurado

## Como Funciona

### 1. Conexão Wi-Fi

A placa se conecta à rede Wi-Fi usando as credenciais definidas no código:

```c
#define WIFI_SSID "CLEUDO"
#define WIFI_PASSWORD "91898487"
```

### 2. Conexão MQTT

Usando DNS, o domínio `broker.hivemq.com` é resolvido e a conexão MQTT é estabelecida automaticamente:

```c
dns_gethostbyname(MQTT_BROKER, &broker_ip, dns_check_callback, NULL);
```

### 3. Publicação dos Dados

O sensor BH1750 é lido e os dados em lux são enviados via MQTT:

```c
float lux = bh1750_read_lux();
publish_data(lux);
```

### 4. Recepção de Comandos

Comandos como `"on"` e `"off"` são recebidos e processados para controlar o LED:

```c
if (strstr(data, "on") != NULL) {
    gpio_put(LED_PIN, 1);
}
```

### 5. Watchdog para Resiliência

Se o sistema travar, o watchdog reinicia automaticamente a placa após 10 segundos:

```c
watchdog_enable(10000, 1);
```

## Lições Aprendidas

Durante o desenvolvimento, foram enfrentadas falhas de conexão Wi-Fi e perda de comunicação com o broker. A solução encontrada foi implementar:

* Reconexão automática com o Wi-Fi e MQTT
* Uso de watchdog para reiniciar o sistema em caso de falha total

Essa experiência reforçou a importância da **resiliência em sistemas embarcados IoT**

## Autor

**Kauã Lima de Queiroz**
Residência Tecnológica em Software Embarcado | [EMBARCATECH](https://embarcatech.softex.br/)

**Você pode saber mais sobre mim em:** &nbsp;&nbsp;[![LinkedIn](https://img.shields.io/badge/LinkedIn-0077B5?style=for-the-badge&logo=linkedin&logoColor=white)](https://www.linkedin.com/in/kaualimaq/)

**Ou me contatar através do:** &nbsp;&nbsp;[![Gmail](https://img.shields.io/badge/Gmail-333333?style=for-the-badge&logo=gmail&logoColor=red)](mailto:limakaua610@gmail.com)
