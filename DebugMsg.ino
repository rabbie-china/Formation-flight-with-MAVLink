#include <WiFi.h>
#include <WiFiUdp.h>

// IP Address details
IPAddress server_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress debug_ip(192, 168, 1, 88);  //接收调试信息的单片机
#define localPort     5060

WiFiUDP udp;

// 启用 1 字节对齐
#pragma pack(1)
typedef struct {
    uint8_t type;  // 类型标识，1 表示 MyPacket 类型数据，其他值表示 MyLine 类型数据
                   // 2 表示编队消息,申请编队,发送编队编号
                   // 3 调试信息
                   // 4 遥控器通道信息
    // 联合体用于存储不同类型的数据
    union {
        // MyPacket 类型数据
        struct {
            uint64_t time_usec;  /*< [us] Timestamp (UNIX Epoch time or time since system boot). The receiving end can infer timestamp format (since 1.1.1970 or since system boot) by checking for the magnitude the number. */
            uint8_t fix_type;
            int32_t lat;         /*< [degE7] Latitude, expressed */
            int32_t lon;         /*< [degE7] Longitude, expressed */
            int32_t alt;         /*< [mm] Altitude (MSL). Note that virtually all GPS modules provide both WGS84 and MSL. */
            uint8_t byTeamType;  // 队形
            uint16_t uTeamDist;  // 编队距离
            int uTeamHigh;       // 编队高度
            int16_t heading;     // 飞行器飞行方向
            uint16_t velspeed;   // 飞行器地面速度
            uint8_t byMode;      // 长机飞控状态 0:上锁 1:解锁
        } my_packet;
        // MyLine 类型数据
        struct {
            uint8_t order;        // 飞机编号
            uint8_t mac[6];       // 编队飞机的网卡地址
            char chNikeName[20];  // 飞机操作者的小名
        } my_line;
        struct {
            char chDebugMsg[33];  // 调试信息
        } my_msg;
        struct {
            uint16_t chan1_raw;       // 2 字节
            uint16_t chan2_raw;       // 2 字节
            uint16_t chan3_raw;       // 2 字节
            uint16_t chan4_raw;       // 2 字节
            uint16_t chan5_raw;       // 2 字节
            uint16_t chan6_raw;       // 2 字节
            uint16_t chan7_raw;       // 2 字节
            uint16_t chan8_raw;       // 2 字节
            uint16_t chan9_raw;       // 2 字节
            uint16_t chan10_raw;      // 2 字节
            uint16_t chan11_raw;      // 2 字节
            uint16_t chan12_raw;      // 2 字节
            uint16_t chan13_raw;      // 2 字节
            uint16_t chan14_raw;      // 2 字节
            uint16_t chan15_raw;      // 2 字节
            uint16_t chan16_raw;      // 2 字节
        } my_rc_channels;
    } data;
    uint8_t checksum;             // 校验和字段
} UnifiedPacket;
// 恢复默认对齐
#pragma pack()

const char *ssid = "TeamD13B";
const char *password = "123456789";
UnifiedPacket unifiedPacket;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // 用于调试的串口
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);

  // 设置静态 IP
  if (!WiFi.config(debug_ip, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\n我的地址: ");
  Serial.println(WiFi.localIP());

  // 启动UDP监听
  udp.begin(localPort);

  // 创建接收任务
  xTaskCreate(udpReceiveTask, "UDP_Receiver", 4096, NULL, 1, NULL);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
}

// UDP接收任务
void udpReceiveTask(void *pvParameters)
{
  while (1)
  {
    int packetSize = udp.parsePacket();  // 获取数据包大小
    if (packetSize >= sizeof(UnifiedPacket)) {  // 只有当数据包完整时才读取
      int len = udp.read((uint8_t *)&unifiedPacket, sizeof(UnifiedPacket));
      if (len > 0){
        OnFlashLight(1);
        if(unifiedPacket.checksum==calculate_checksum(&unifiedPacket)){
          if(unifiedPacket.type==3){
            //收到僚机的编队序号请求
            Serial.println(unifiedPacket.data.my_msg.chDebugMsg);
          }
        }
        else{
          Serial.println("收到错误的数据包......");
        }
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
void OnFlashLight(int nStatus) {
  switch (nStatus) {
    case 1:
      digitalWrite(12, HIGH);
      vTaskDelay(4 / portTICK_PERIOD_MS);
      digitalWrite(12, LOW);
      break;
    case 0:
      digitalWrite(13, HIGH);
      vTaskDelay(4 / portTICK_PERIOD_MS);
      digitalWrite(13, LOW);
      break;
  }
}
// 计算校验和的函数
uint8_t calculate_checksum(const UnifiedPacket *packet) {
  uint8_t sum = 0;
  sum += packet->type;
  const uint8_t *ptr;
  size_t size;

  ptr = (const uint8_t *)&(packet->data.my_packet);
  size = sizeof(packet->data.my_packet);

  for (size_t i = 0; i < size; i++) {
    sum += ptr[i];
  }
  return sum;
}

