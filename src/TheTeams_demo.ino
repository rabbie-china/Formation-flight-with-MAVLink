#include "MAVLink/all/MAVLink.h"
#include "g_head.h"

#define BAUD_RATE 115200  // 飞控 UART 波特率
#define RXPIN 5           // ESP32 UART RX 引脚（连接飞控 TX）
#define TXPIN 4           // ESP32 UART TX 引脚（连接飞控 RX）

// MAVLink 系统定义
mavlink_system_t mavlink_system = {
    1,                    // 系统 ID（地面站）
    MAV_COMP_ID_TELEMETRY_RADIO  // 组件 ID（通信模块）
};

// 存储解析后的飞控数据
struct FlightData {
  uint8_t system_status;      // 系统状态
  uint8_t base_mode;          // 基本模式
  uint32_t custom_mode;       // 自定义模式
  int32_t gps_lat;            // GPS 纬度 (1e7 度)
  int32_t gps_lon;            // GPS 经度 (1e7 度)
  int32_t gps_alt;            // GPS 高度 (毫米)
  uint8_t gps_fix_type;       // GPS 固定类型
  float roll;                 // 滚转角度 (弧度)
  float pitch;                // 俯仰角度 (弧度)
  float yaw;                  // 偏航角度 (弧度)
} flight_data;

// 初始化串口和 MAVLink
void setup() {
  Serial.begin(115200);  // 串口监视器
  Serial1.begin(BAUD_RATE, SERIAL_8N1, RXPIN, TXPIN);  // 飞控 UART
  Serial.println("开始读取飞控数据...");

  // 初始化飞控数据
  memset(&flight_data, 0, sizeof(FlightData));

  // 请求数据流
  requestDataStreams(MAV_DATA_STREAM_ALL, 5);  // 5Hz
}

// 主循环：读取和打印飞控数据
void loop() {
  sendHeartbeat();  // 发送心跳包
  readMAVLinkMessages();  // 读取飞控消息
  printFlightData();  // 打印数据
  delay(200);  // 每 200ms 更新一次（约 5Hz）
}

// 发送 MAVLink 心跳包
void sendHeartbeat() {
  mavlink_message_t msg;
  uint8_t buf[MAVLINK_MAX_PACKET_LEN];

  mavlink_msg_heartbeat_pack(
      mavlink_system.sysid,
      mavlink_system.compid,
      &msg,
      MAV_TYPE_GCS,  // 地面站
      MAV_AUTOPILOT_INVALID,
      MAV_MODE_MANUAL_ARMED,
      0,
      MAV_STATE_ACTIVE
  );

  uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
  Serial1.write(buf, len);
}

// 请求数据流
void requestDataStreams(uint8_t stream_id, uint16_t rate) {
  mavlink_message_t msg;
  uint8_t buf[MAVLINK_MAX_PACKET_LEN];

  mavlink_msg_request_data_stream_pack(
      mavlink_system.sysid,
      mavlink_system.compid,
      &msg,
      1,  // 目标系统 ID（飞控）
      0,  // 目标组件 ID
      stream_id,
      rate,
      1   // 开启数据流
  );

  uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
  Serial1.write(buf, len);
}

// 读取和解析 MAVLink 消息
void readMAVLinkMessages() {
  mavlink_message_t msg;
  mavlink_status_t status;

  while (Serial1.available()) {
    uint8_t c = Serial1.read();
    if (mavlink_parse_char(MAVLINK_COMM_0, c, &msg, &status)) {
      switch (msg.msgid) {
        case MAVLINK_MSG_ID_HEARTBEAT: {
          mavlink_heartbeat_t heartbeat;
          mavlink_msg_heartbeat_decode(&msg, &heartbeat);
          flight_data.system_status = heartbeat.system_status;
          flight_data.base_mode = heartbeat.base_mode;
          flight_data.custom_mode = heartbeat.custom_mode;
          break;
        }
        case MAVLINK_MSG_ID_GPS_RAW_INT: {
          mavlink_gps_raw_int_t gps;
          mavlink_msg_gps_raw_int_decode(&msg, &gps);
          flight_data.gps_lat = gps.lat;
          flight_data.gps_lon = gps.lon;
          flight_data.gps_alt = gps.alt;
          flight_data.gps_fix_type = gps.fix_type;
          break;
        }
        case MAVLINK_MSG_ID_ATTITUDE: {
          mavlink_attitude_t attitude;
          mavlink_msg_attitude_decode(&msg, &attitude);
          flight_data.roll = attitude.roll;
          flight_data.pitch = attitude.pitch;
          flight_data.yaw = attitude.yaw;
          break;
        }
      }
    }
  }
}

// 打印飞控数据到串口监视器
void printFlightData() {
  Serial.println("=== 飞控数据 ===");
  Serial.print("系统状态: ");
  switch (flight_data.system_status) {
    case MAV_STATE_STANDBY: Serial.println("待机"); break;
    case MAV_STATE_ACTIVE: Serial.println("活跃"); break;
    case MAV_STATE_CRITICAL: Serial.println("危急"); break;
    default: Serial.println(flight_data.system_status); break;
  }
  Serial.print("基本模式: ");
  Serial.print(flight_data.base_mode, BIN);
  Serial.print(" (");
  if (flight_data.base_mode & MAV_MODE_FLAG_SAFETY_ARMED) Serial.print("已解锁 ");
  if (flight_data.base_mode & MAV_MODE_FLAG_GUIDED_ENABLED) Serial.print("引导模式 ");
  Serial.println(")");
  Serial.print("自定义模式: ");
  Serial.println(flight_data.custom_mode);

  Serial.print("GPS 纬度: ");
  Serial.print(flight_data.gps_lat / 1e7, 7);
  Serial.println(" 度");
  Serial.print("GPS 经度: ");
  Serial.print(flight_data.gps_lon / 1e7, 7);
  Serial.println(" 度");
  Serial.print("GPS 高度: ");
  Serial.print(flight_data.gps_alt / 1000.0, 2);
  Serial.println(" 米");
  Serial.print("GPS 固定类型: ");
  switch (flight_data.gps_fix_type) {
    case 0: Serial.println("无固定"); break;
    case 1: Serial.println("无 GPS"); break;
    case 2: Serial.println("2D 固定"); break;
    case 3: Serial.println("3D 固定"); break;
    default: Serial.println(flight_data.gps_fix_type); break;
  }

  Serial.print("滚转: ");
  Serial.print(flight_data.roll * 180.0 / PI, 2);
  Serial.println(" 度");
  Serial.print("俯仰: ");
  Serial.print(flight_data.pitch * 180.0 / PI, 2);
  Serial.println(" 度");
  Serial.print("偏航: ");
  Serial.print(flight_data.yaw * 180.0 / PI, 2);
  Serial.println(" 度");
  Serial.println();
}