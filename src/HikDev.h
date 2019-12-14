#ifndef HIK_DEV_H_
#define HIK_DEV_H_

#include "HCNetSDK.h"
#include "global.h"
#include <iostream>
#include <thread>
#include <unistd.h>
#include <time.h>

#define LOGIN_DEV_MAX_NUM 2048
#define ALARM_MOTION_DETECT_MAX 32

typedef struct
{
    NET_DVR_ALARMER struAlarmDevInfo;
    BYTE byRes[7];
    BYTE byChannel;
    time_t AlarmTime;
} ALARM_MOTION_DETECT, *LPALARM_MOTION_DETECT;

class HikDevHandl
{
public:
    HikDevHandl();
    ~HikDevHandl();

    std::string sDevIp; /*device ip addr for login*/
    WORD wDevPort;      /*device port for login*/
    std::string sDevUserName; /*device user name for login*/
    std::string sDevPassword; /*device password for login*/
    LONG lUserId;

private:
    /*user id returned by device after login*/
};

class HikDevOperate
{
public:
    HikDevOperate();
    ~HikDevOperate();

    /*Initial SDK Api*/
    static BOOL InitialSDK();
    static BOOL DeinitialSDK();

    static void SetPtrLogger(std::shared_ptr<spdlog::logger> logger);

    /*Login Device Api*/
    static LONG HikDevLogin(HikDevHandl DevHdl);

    /*Get Device Configuration Api*/
    static BOOL HikDevGetDevCfg(LONG lUserId);
    static BOOL HikDevGetNetCfg(LONG lUserId);

    /*Alarm Api*/
    static BOOL HikDevStartAlarm(LONG lUserId);
    static BOOL HikDevStopAlarm();

    /*Alarm Device Api*/
    static uint32_t HikDevGetAlarmNum(void);
    static void HikDevSetAlarmIdxOffset(uint32_t offset);
    static void HikDevGetAlarmDateTime(uint32_t uiAlarmIdx, uint32_t &uiYear, uint32_t &uiMonth, uint32_t &uiDay,
                                       uint32_t &uiHour, uint32_t &uiMinute, uint32_t &uiSecond);
    static void HikDevGetAlarmIp(uint32_t uiAlarmIdx, std::string &strAlarmIp);
    static void HikDevGetAlarmDevName(uint32_t uiAlarmIdx, std::string &strAlarmDevName);

    /*Get Alarm Information Varialble*/
    static LONG lAlarmHandle; /*hanlde to start or stop alarm*/
private:
    static void HikDevLoginCallback(LONG lUserID, DWORD dwResult, LPNET_DVR_DEVICEINFO_V30 lpDeviceInfo, void *pUser);
    static void HikDevAlarmCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void *pUser);

    /*Motion Detect Api*/
    static BOOL HikDevMotionDetectCfg(LONG lUserId);

    /*output info api*/
    static void HikDevOutputAlarmDevInfo(NET_DVR_ALARMER struAlarmDev, std::string &OutStr);
    static void HikDevOutputAlarmInfo(uint32_t uiAlarmIdx);

    /*translate api*/
    static void HikDevMacHexToStr(BYTE *bMacHex, char *sMacStr, DWORD dwMacStrLen);

    /*Motion Detect Varialble*/
    static ALARM_MOTION_DETECT struAlarmMotionDetect[ALARM_MOTION_DETECT_MAX];
    static DWORD dwAlarmMotionDetectWr;
    static DWORD dwAlarmMotionDetectRd;

    static std::shared_ptr<spdlog::logger> ptrLogger;
};

#endif
