#ifndef _G_HEAD
#define _G_HEAD
#include <stdint.h>
#include "MAVLink\all\MAVLink.h"
#include <WiFi.h>

// -----------------------------------------------------------
// SHOW_DEBUG_INFO  打开飞控所有的信息通过串口显示在电脑屏幕上
// DEBUG_MODE 调试模式
// BUTTON_MODE 打开按钮功能
// -----------------------------------------------------------

// #define SHOW_DEBUG_INFO
// #define DEBUG_MODE
// #define BUTTON_MODE

#define MAX_UINT64_T  0xFFFFFFFFFFFFFFFF    // uint64_t 的最大值
#define FLIGHT_NUM    9                     // 编队飞机数量,最大9架
#define localPort     5060
// 定义圆周率
#define PI 3.14159265358979323846
#define TOLERANCE 1e-6

#ifdef ARDUINO_ESP32S3_DEV
#define RXPIN 5          // GPIO 5 => RX for Serial1 or Serial1 接飞控的tx1 测试uart1和uart6能正确连接，其他端口无法连接
#define TXPIN 4          // GPIO 4 => TX for Serial1 or Serial1 接飞控的rx1
#define SWITCH1 15       // 重置开关
#define LED_PIN 1       // LED
#define LED_PIN2 2      // LED
#define DATA_PIN 10      // ws2812
#define NUM_LEDS 8       // 定义LED灯带的数量
#endif

#ifdef ARDUINO_ESP32C3_DEV
#define RXPIN 5          // GPIO 5 => RX for Serial1 or Serial1 接飞控的tx1 测试uart1和uart6能正确连接，其他端口无法连接
#define TXPIN 4          // GPIO 4 => TX for Serial1 or Serial1 接飞控的rx1
#define SWITCH1 3       // 重置开关
#define LED_PIN 12       // LED
#define LED_PIN2 13      // LED
#define DATA_PIN 8      // ws2812
#define NUM_LEDS 1       // 定义LED灯带的数量
#endif

#define MAX_TX_POWER 20
#define MAX_TEAM_DISTANT 20000      // 最大飞行编队距离，如果超过这个距离，系统拒绝编队飞行,200米
#define LOWEST_ALTITUDE  6000       // 允许编队最低高度，单位(毫米),6米
#define MAX_LOST_WAIT_TIME  12000   // 失联后最大等待时间，单位(毫秒),12秒

// 启用 1 字节对齐
#pragma pack(1)
//---------------------------------------------------------------------------
typedef struct tag_MyConfig
{
    uint8_t AP;            // 长机
    char chSsid[50];       // 编队长机使用的ssid
    char chPasswd[50];     // 编队长机使用的密码
    uint8_t byPlayerCount; // 玩家飞机数量,目前仅支持10架编队
    char MyNikeName[20];   // 玩家飞机的名称
    uint16_t uTeamDist;    // 编队距离
    int uTeamHigh;         // 编队高度
    uint8_t byTeamType;    // 编队类型
    uint8_t ControlMeByLeader;  // 允许我被长机遥控 
} MyConfig;
//---------------------------------------------------------------------------
typedef struct tag_MyStatus
{
    mavlink_gps_raw_int_t gpsData; // gps信息
    mavlink_vfr_hud_t hudData;     // 飞行姿态
    mavlink_attitude_t attitude;   // 飞机姿态
    mavlink_heartbeat_t heartbeat; // 心跳包
    mavlink_global_position_int_t pos;  //全球位置信息
    mavlink_system_time_t system_time;  //系统时间
    mavlink_sys_status_t sysStatus;
    mavlink_autopilot_version_t autopilot_version;  //飞控版本
    mavlink_rc_channels_t rc_channels;  // 遥控器通道信息
    uint8_t byPlaneOrder;          // 编队飞机序号，长机0 僚机1 僚机2
    uint8_t byWifi;                // 飞机之间的WIFI状态 0:断网 1:联网
    uint8_t mac[6];                // 网卡地址
    unsigned long lastFcTime;      // 最后收到飞控发来数据的时间 
    unsigned long lastWifiTime;    // 最后收到wifi的时间
} MyStatus;
//---------------------------------------------------------------------------
//飞机编队信息
typedef struct tag_MyLine
{
  uint8_t useage;       // 是否使用
  uint8_t mac[6];       // 编队飞机的网卡地址
  char chNikeName[20];  // 飞机操作者的小名
  unsigned long line_time;   // 最后一次收到定位消息的时间戳
}MyLine;
// 恢复默认对齐
#pragma pack()

#endif