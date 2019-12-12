#include "HikDev.h"
#include <string.h>
#include <stdio.h>

LONG HikDevOperate::lAlarmHandle;

ALARM_MOTION_DETECT HikDevOperate::struAlarmMotionDetect[ALARM_MOTION_DETECT_MAX];
DWORD HikDevOperate::dwAlarmMotionDetectWr;
DWORD HikDevOperate::dwAlarmMotionDetectRd;

std::shared_ptr<spdlog::logger> HikDevOperate::ptrLogger;

HikDevHandl::HikDevHandl()
{

}

HikDevHandl::~HikDevHandl()
{

}

HikDevOperate::HikDevOperate()
{
    memset((void *)&struAlarmMotionDetect[0], 0, sizeof(struAlarmMotionDetect));
    dwAlarmMotionDetectWr = 0;
    dwAlarmMotionDetectRd = 0;
}

HikDevOperate::~HikDevOperate()
{
}

BOOL HikDevOperate::InitialSDK()
{
    if (NET_DVR_Init() == false)
    {
        ptrLogger->error("initial sdk fail, error code = {0:d}", NET_DVR_GetLastError());
        ptrLogger->flush();
        return false;
    }

    NET_DVR_LOCAL_SDK_PATH struSdkPath;
    strcpy(struSdkPath.sPath, "/usr/local/mdalarm/lib");

    NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_SDK_PATH, &struSdkPath);

    return true;
}

BOOL HikDevOperate::DeinitialSDK()
{
    if (NET_DVR_Cleanup() == false)
    {
        ptrLogger->error("Deinitial sdk fail, error code = {0:d}", NET_DVR_GetLastError());
        ptrLogger->flush();
        return false;
    }

    return true;
}

void HikDevOperate::SetPtrLogger(std::shared_ptr<spdlog::logger> logger)
{
    ptrLogger = logger;
}

void HikDevOperate::HikDevLoginCallback(LONG lUserID, DWORD dwResult, LPNET_DVR_DEVICEINFO_V30 lpDeviceInfo, void *pUser)
{

}

void HikDevOperate::HikDevRealPlayCallback(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void *pUser)
{
    std::cout << "handle = " << lRealHandle << " data type = " << dwDataType << " bufsize = " << dwBufSize << std::endl;
}

void HikDevOperate::HikDevAlarmCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void *pUser)
{
    switch(lCommand)
    {
    case COMM_ALARM_V30:
    {
        LPNET_DVR_ALARMINFO_V30 lpAlarmInfo = (LPNET_DVR_ALARMINFO_V30)pAlarmInfo;
        switch (lpAlarmInfo->dwAlarmType)
        {
        case 3:
            for(DWORD i = 0;i < MAX_CHANNUM_V30;i ++)
            {
                if(lpAlarmInfo->byChannel[i])
                {
                    struAlarmMotionDetect[dwAlarmMotionDetectWr].struAlarmDevInfo = *pAlarmer;
                    struAlarmMotionDetect[dwAlarmMotionDetectWr].byChannel = i + 1;
                    time(&struAlarmMotionDetect[dwAlarmMotionDetectWr].AlarmTime);
                    if(++dwAlarmMotionDetectWr >= ALARM_MOTION_DETECT_MAX)
                    {
                        dwAlarmMotionDetectWr = 0;
                    }
                    break;
                }
            }
            break;
        default:
            break;
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

LONG HikDevOperate::HikDevLogin(HikDevHandl DevHdl)
{
    NET_DVR_USER_LOGIN_INFO struDevLoginInfo;
    NET_DVR_DEVICEINFO_V40 struDevInfo;
    LONG UserId;

    memset((void *)&struDevLoginInfo, 0, sizeof(struDevLoginInfo));

    strcpy(struDevLoginInfo.sDeviceAddress, DevHdl.sDevIp.c_str());
    struDevLoginInfo.wPort = DevHdl.wDevPort;

    strcpy(struDevLoginInfo.sUserName, DevHdl.sDevName.c_str());
    strcpy(struDevLoginInfo.sPassword, DevHdl.sDevPassword.c_str());

    UserId = NET_DVR_Login_V40(&struDevLoginInfo, &struDevInfo);
    if (UserId < 0)
    {
        ptrLogger->error("login device fail, error code = {0:d}", NET_DVR_GetLastError());
        ptrLogger->flush();
        return -1;
    }

    return UserId;
}

BOOL HikDevOperate::HikDevGetDevCfg(LONG lUserId)
{
    NET_DVR_DEVICECFG_V40 DevCfg;
    DWORD BytesReturned;

    if(!NET_DVR_GetDVRConfig(lUserId, NET_DVR_GET_DEVICECFG_V40, 0, &DevCfg, sizeof(DevCfg), &BytesReturned))
    {
        ptrLogger->error("get device confiuration fail, error code = {0:d}", NET_DVR_GetLastError());
        ptrLogger->flush();
        return false;
    }

    std::cout << "structrue size : " << DevCfg.dwSize << std::endl;

    std::cout << "device name : " << DevCfg.sDVRName << std::endl;
    std::cout << "device id : " << DevCfg.dwDVRID << std::endl;
    std::cout << "dvr type : " << (DWORD)DevCfg.byDVRType << std::endl;
    std::cout << "device type : " << DevCfg.wDevType << std::endl;
    std::cout << "device type name : " << DevCfg.byDevTypeName << std::endl;

    std::cout << "record cycle or not : " << (!DevCfg.dwRecycleRecord?"false":"true") << std::endl;
    std::cout << "device serial number : " << DevCfg.sSerialNumber << std::endl;

    DWORD HighVer = ((DevCfg.dwSoftwareVersion & 0xffff0000) >> 16);
    DWORD LowVer = (DevCfg.dwSoftwareVersion & 0x0000ffff);
    std::cout << "device software version : " << HighVer << "." << LowVer << std::endl;

    DWORD year = (DevCfg.dwSoftwareBuildDate >> 16);
    DWORD month = ((DevCfg.dwSoftwareBuildDate & 0x0000ff00) >> 8);
    DWORD day = (DevCfg.dwSoftwareBuildDate & 0x000000ff);
    std::cout << "device software build date : " << year << "." << month << "." << day << std::endl;

    HighVer = (DevCfg.dwDSPSoftwareVersion >> 16);
    LowVer = (DevCfg.dwDSPSoftwareVersion & 0x0000ffff);
    std::cout << "dsp software version : " << HighVer << "." << LowVer << std::endl;

    year = (DevCfg.dwDSPSoftwareBuildDate >> 16);
    month = ((DevCfg.dwDSPSoftwareBuildDate & 0x0000ff00) >> 8);
    day = (DevCfg.dwDSPSoftwareBuildDate & 0x000000ff);
    std::cout << "dsp software build date : " << year << "." << month << "." << day << std::endl;

    HighVer = (DevCfg.dwPanelVersion >> 16);
    LowVer = (DevCfg.dwPanelVersion & 0x0000ffff);
    std::cout << "panel version : " << HighVer << "." << LowVer << std::endl;

    HighVer = (DevCfg.dwHardwareVersion >> 16);
    LowVer = (DevCfg.dwHardwareVersion & 0x0000ffff);
    std::cout << "hardware version : " << HighVer << "." << LowVer << std::endl;

    std::cout << "alarm in port number : " << (DWORD)DevCfg.byAlarmInPortNum << std::endl;
    std::cout << "alarm out port number : " << (DWORD)DevCfg.byAlarmOutPortNum << std::endl;
    std::cout << "analog alarm inpot number : " << (DWORD)DevCfg.byAnalogAlarmInPortNum << std::endl;
    std::cout << "analog start alarm in port number : " << (DWORD)DevCfg.byStartAlarmInNo << std::endl;
    std::cout << "analog start alarm out port number : " << (DWORD)DevCfg.byStartAlarmOutNo << std::endl;
    std::cout << "IP start alarm in port number : " << (DWORD)DevCfg.byStartIPAlarmInNo << std::endl;
    std::cout << "IP start alarm out port number : " << (DWORD)DevCfg.byStartIPAlarmOutNo << std::endl;

    std::cout << "rs232 number : " << (DWORD)DevCfg.byRS232Num << std::endl;
    std::cout << "rs485 number : " << (DWORD)DevCfg.byRS485Num << std::endl;
    std::cout << "net port number : " << (DWORD)DevCfg.byNetworkPortNum << std::endl;
    std::cout << "disk ctrl number : " << (DWORD)DevCfg.byDiskCtrlNum << std::endl;
    std::cout << "disk number : " << (DWORD)DevCfg.byDiskNum << std::endl;
    std::cout << "channel number : " << (DWORD)DevCfg.byChanNum << std::endl;
    std::cout << "start channel number : " << (DWORD)DevCfg.byStartChan << std::endl;
    std::cout << "decord chaanel number : " << (DWORD)DevCfg.byDecordChans << std::endl;
    std::cout << "VGA port number : " << (DWORD)DevCfg.byVGANum << std::endl;
    std::cout << "usb port number : " << (DWORD)DevCfg.byUSBNum << std::endl;
    std::cout << "aux out port number : " << (DWORD)DevCfg.byAuxoutNum << std::endl;
    std::cout << "audio port number : " << (DWORD)DevCfg.byAudioNum << std::endl;

    DWORD IpChannelNum = (((DWORD)DevCfg.byHighIPChanNum << 8) | DevCfg.byIPChanNum);
    std::cout << "ip channel number : " << IpChannelNum << std::endl;

    std::cout << "zero channel number : " << (DWORD)DevCfg.byZeroChanNum << std::endl;

    DWORD IntelligentSearch = (DevCfg.bySupport & 1);
    DWORD Backup = (DevCfg.bySupport & 2);
    DWORD CompressAbilityGet = (DevCfg.bySupport & 4);
    DWORD DoubleNetCard = (DevCfg.bySupport & 8);
    DWORD RemoteSADP = (DevCfg.bySupport & 0x10);
    DWORD RaidCardFunc = (DevCfg.bySupport & 0x20);
    DWORD IPSAN_Search = (DevCfg.bySupport & 0x40);
    DWORD RTP_Over_RTSP = (DevCfg.bySupport & 0x80);
    DWORD SNMP_V30 = (DevCfg.bySupport1 & 1);
    DWORD Playback_Download = (DevCfg.bySupport1 & 2);
    DWORD ExpOSD = (DevCfg.bySupport2 & 1);
    std::cout << "Support Intelligent Search : " << (!IntelligentSearch?"no":"yes") << std::endl;
    std::cout << "Support Backup : " << (!Backup?"no":"yes") << std::endl;
    std::cout << "Support Compress Ability Get : " << (!CompressAbilityGet?"no":"yes") << std::endl;
    std::cout << "Support Double NetCard : " << (!DoubleNetCard?"no":"yes") << std::endl;
    std::cout << "Support Remote SADP : " << (!RemoteSADP?"no":"yes") << std::endl;
    std::cout << "Support RaidCard Func : " << (!RaidCardFunc?"no":"yes") << std::endl;
    std::cout << "Support IPSAN Search : " << (!IPSAN_Search?"no":"yes") << std::endl;
    std::cout << "Support RTP Over RTSP : " << (!RTP_Over_RTSP?"no":"yes") << std::endl;
    std::cout << "Support SNMP V30 : " << (!SNMP_V30?"no":"yes") << std::endl;
    std::cout << "Support Playback Download : " << (!Playback_Download?"no":"yes") << std::endl;
    std::cout << "Support Expand OSD : " << (!ExpOSD?"no":"yes") << std::endl;

    std::cout << "esata useage : " << (!DevCfg.byEsataUseage?"backup":"record") << std::endl;
    std::cout << "ipc plug : " << (!DevCfg.byIPCPlug?"close":"open") << std::endl;
    std::cout << "storage mode : " << (DWORD)DevCfg.byStorageMode << std::endl;
    std::cout << "remote power on : " << (!DevCfg.byEnableRemotePowerOn?"disable":"enable") << std::endl;

    return true;
}

BOOL HikDevOperate::HikDevGetNetCfg(LONG lUserId)
{
    NET_DVR_NETCFG_V50 struNetCfg;
    DWORD BytesReturned;

    if(!NET_DVR_GetDVRConfig(lUserId, NET_DVR_GET_NETCFG_V50, 0, &struNetCfg, sizeof(struNetCfg), &BytesReturned))
    {
        ptrLogger->error("get net configuration fail, error code = {0:d}", NET_DVR_GetLastError());
        ptrLogger->flush();
        return false;
    }

    std::cout << "structure size : " << struNetCfg.dwSize << std::endl;

    for(DWORD i = 0;i < MAX_ETHERNET;i ++)
    {
        std::cout << "ethernet " << i << " cfg : {" << std::endl;
        std::cout << "ipv4 : " << struNetCfg.struEtherNet[i].struDVRIP.sIpV4 << std::endl;
        std::cout << "ipv6 : " << struNetCfg.struEtherNet[i].struDVRIP.byIPv6 << std::endl;
        std::cout << "ipv4 mask : " << struNetCfg.struEtherNet[i].struDVRIPMask.sIpV4 << std::endl;
        std::cout << "ipv6 mask : " << struNetCfg.struEtherNet[i].struDVRIPMask.byIPv6 << std::endl;
        std::cout << "net interface : ";
        switch(struNetCfg.struEtherNet[i].dwNetInterface)
        {
        case 1:
            std::cout << "10MBase-T" << std::endl;
            break;
        case 2:
            std::cout << "10MBase-T Full Duplex" << std::endl;
            break;
        case 3:
            std::cout << "100MBase-TX" << std::endl;
            break;
        case 4:
            std::cout << "100M Full Duplex" << std::endl;
            break;
        case 5:
            std::cout << "10M/100M/1000M Self-adaption" << std::endl;
            break;
        case 6:
            std::cout << "1000M Full Duplex" << std::endl;
            break;
        default:
            std::cout << "Invalid" << std::endl;
            break;
        }
        std::cout << "dvr port : " << struNetCfg.struEtherNet[i].wDVRPort << std::endl;
        std::cout << "MTU : " << struNetCfg.struEtherNet[i].wMTU << std::endl;
        std::cout << "mac addr : ";
        for(DWORD j = 0;j < MACADDR_LEN;j ++)
        {
            printf("%02x", struNetCfg.struEtherNet[i].byMACAddr[j]);
            if(j < MACADDR_LEN - 1)
            {
                printf("-");
            }
        }
        printf("\n");

        WORD EthernetPortNo = (WORD)struNetCfg.struEtherNet[i].byEthernetPortNo;
        std::cout << "ethernet port No. : ";
        if(!EthernetPortNo)
        {
            std::cout << "invalid" << std::endl;
        }
        else
        {
            std::cout << EthernetPortNo - 1 << std::endl;
        }

        std::cout << "}" << std::endl;
    }

    std::cout << "alarm host ip addr v4 : " << struNetCfg.struAlarmHostIpAddr.sIpV4 << std::endl;
    std::cout << "alarm host ip addr v6 : " << struNetCfg.struAlarmHostIpAddr.byIPv6 << std::endl;
    std::cout << "alarm host ip port : " << struNetCfg.wAlarmHostIpPort << std::endl;
    std::cout << "alarm host2 ip addr v4 : " << struNetCfg.struAlarmHost2IpAddr.sIpV4 << std::endl;
    std::cout << "alarm host2 ip addr v6 : " << struNetCfg.struAlarmHost2IpAddr.byIPv6 << std::endl;
    std::cout << "alarm host2 ip port : " << struNetCfg.wAlarmHost2IpPort << std::endl;

    std::cout << "use dhcp : ";
    switch(struNetCfg.byUseDhcp){
    case 0:
        std::cout << "disable" << std::endl;
        break;
    case 1:
        std::cout << "enable" << std::endl;
        break;
    default:
        std::cout << "invalide" << std::endl;
        break;
    }

    std::cout << "ipv6 mode : ";
    switch(struNetCfg.byIPv6Mode){
    case 0:
        std::cout << "router advertise" << std::endl;
        break;
    case 1:
        std::cout << "manual setting" << std::endl;
        break;
    case 2:
        std::cout << "using dhcp" << std::endl;
        break;
    default:
        std::cout << "invalid" << std::endl;
        break;
    }

    std::cout << "dns server1 ip addr v4 : " << struNetCfg.struDnsServer1IpAddr.sIpV4 << std::endl;
    std::cout << "dns server1 ip addr v6 : " << struNetCfg.struDnsServer1IpAddr.byIPv6 << std::endl;
    std::cout << "dns server2 ip addr v4 : " << struNetCfg.struDnsServer2IpAddr.sIpV4 << std::endl;
    std::cout << "dns server2 ip addr v6 : " << struNetCfg.struDnsServer2IpAddr.byIPv6 << std::endl;
    std::cout << "dns mode : " << (!struNetCfg.byEnableDNS?"automatic getting":"manual setting") << std::endl;
    std::cout << "ip resolver : " << struNetCfg.byIpResolver << std::endl;
    std::cout << "ip resolver port : " << struNetCfg.wIpResolverPort << std::endl;
    std::cout << "http port : " << struNetCfg.wHttpPortNo << std::endl;
    std::cout << "multicast ipv4 : " << struNetCfg.struMulticastIpAddr.sIpV4 << std::endl;
    std::cout << "multicast ipv6 : " << struNetCfg.struMulticastIpAddr.byIPv6 << std::endl;
    std::cout << "gateway ipv4 : " << struNetCfg.struGatewayIpAddr.sIpV4 << std::endl;
    std::cout << "gateway ipv6 : " << struNetCfg.struGatewayIpAddr.byIPv6 << std::endl;

    std::cout << "PPPoE : " << (!struNetCfg.struPPPoE.dwPPPOE?"disable":"enable") << std::endl;
    std::cout << "PPPoE user name : " << struNetCfg.struPPPoE.sPPPoEUser << std::endl;
    std::cout << "PPPoE pass word : " << struNetCfg.struPPPoE.sPPPoEPassword << std::endl;
    std::cout << "PPPoE ipv4 : " << struNetCfg.struPPPoE.struPPPoEIP.sIpV4 << std::endl;
    std::cout << "PPPoE ipv6 : " << struNetCfg.struPPPoE.struPPPoEIP.byIPv6 << std::endl;

    std::cout << "private multicast discovery : ";
    switch (struNetCfg.byEnablePrivateMulticastDiscovery) {
    case 0:
        std::cout << "default" << std::endl;
        break;
    case 1:
        std::cout << "disable" << std::endl;
        break;
    case 2:
        std::cout << "enable" << std::endl;
        break;
    default:
        break;
    }

    std::cout << "onvif multicast discovery : ";
    switch (struNetCfg.byEnableOnvifMulticastDiscovery) {
    case 0:
        std::cout << "default" << std::endl;
        break;
    case 1:
        std::cout << "disable" << std::endl;
        break;
    case 2:
        std::cout << "enable" << std::endl;
        break;
    default:
        break;
    }


}

BOOL HikDevOperate::HikDevRealPlayStart(LONG lUserId)
{
    NET_DVR_PREVIEWINFO PrevInfo;

    memset((void *)&PrevInfo, 0, sizeof(PrevInfo));
    PrevInfo.lChannel = 1;

    if(NET_DVR_RealPlay_V40(lUserId, &PrevInfo, HikDevRealPlayCallback, nullptr) < 0)
    {
        ptrLogger->error("real play fail, error code = {0:d}", NET_DVR_GetLastError());
        ptrLogger->flush();
        return false;
    }

    return true;
}

BOOL HikDevOperate::HikDevMotionDetectCfg(LONG lUserId)
{
    NET_DVR_PICCFG_V30 struPicCfg;
    DWORD dwRetSize;

    if(!NET_DVR_GetDVRConfig(lUserId, NET_DVR_GET_PICCFG_V30, 1, (void *)&struPicCfg, sizeof(struPicCfg), &dwRetSize))
    {
        ptrLogger->error("get picture configuration, error code = {0:d}", NET_DVR_GetLastError());
        ptrLogger->flush();
        return false;
    }

#define MOTION_DETECT_PARAM_BUF_SIZE    64

    char MotionDetectParamBuf[MOTION_DETECT_PARAM_BUF_SIZE];
    std::string strParamOut;

    snprintf(MotionDetectParamBuf, MOTION_DETECT_PARAM_BUF_SIZE, "%s\n", "motion detect parameter : {");
    strParamOut.append(MotionDetectParamBuf, strlen(MotionDetectParamBuf));

    snprintf(MotionDetectParamBuf, MOTION_DETECT_PARAM_BUF_SIZE, " %s%d\n", "sensitive : ", (WORD)struPicCfg.struMotion.byMotionSensitive);
    strParamOut.append(MotionDetectParamBuf, strlen(MotionDetectParamBuf));

    snprintf(MotionDetectParamBuf, MOTION_DETECT_PARAM_BUF_SIZE, " %s%s\n", "motion detect handle : ",
             (!struPicCfg.struMotion.byEnableHandleMotion?"disable":"enable"));
    strParamOut.append(MotionDetectParamBuf, strlen(MotionDetectParamBuf));

    snprintf(MotionDetectParamBuf, MOTION_DETECT_PARAM_BUF_SIZE, " %s%s\n", "motion detect high displa : ",
             (!struPicCfg.struMotion.byEnableDisplay?"disable":"enable"));
    strParamOut.append(MotionDetectParamBuf, strlen(MotionDetectParamBuf));

    if (!struPicCfg.struMotion.struMotionHandleType.dwHandleType)
    {
        snprintf(MotionDetectParamBuf, MOTION_DETECT_PARAM_BUF_SIZE, " %s\n", "do nothing");
        strParamOut.append(MotionDetectParamBuf, strlen(MotionDetectParamBuf));
    }
    if (struPicCfg.struMotion.struMotionHandleType.dwHandleType & 0x01)
    {
        snprintf(MotionDetectParamBuf, MOTION_DETECT_PARAM_BUF_SIZE, " %s\n", "monitor alarm");
        strParamOut.append(MotionDetectParamBuf, strlen(MotionDetectParamBuf));
    }
    if (struPicCfg.struMotion.struMotionHandleType.dwHandleType & 0x02)
    {
        snprintf(MotionDetectParamBuf, MOTION_DETECT_PARAM_BUF_SIZE, " %s\n", "voice alarm");
        strParamOut.append(MotionDetectParamBuf, strlen(MotionDetectParamBuf));
    }
    if (struPicCfg.struMotion.struMotionHandleType.dwHandleType & 0x04)
    {
        snprintf(MotionDetectParamBuf, MOTION_DETECT_PARAM_BUF_SIZE, " %s\n", "upload to center");
        strParamOut.append(MotionDetectParamBuf, strlen(MotionDetectParamBuf));
    }
    if (struPicCfg.struMotion.struMotionHandleType.dwHandleType & 0x08)
    {
        snprintf(MotionDetectParamBuf, MOTION_DETECT_PARAM_BUF_SIZE, " %s\n", "trigger alarm output");
        strParamOut.append(MotionDetectParamBuf, strlen(MotionDetectParamBuf));
    }
    if (struPicCfg.struMotion.struMotionHandleType.dwHandleType & 0x10)
    {
        snprintf(MotionDetectParamBuf, MOTION_DETECT_PARAM_BUF_SIZE, " %s\n", "capture and email");
        strParamOut.append(MotionDetectParamBuf, strlen(MotionDetectParamBuf));
    }
    if (struPicCfg.struMotion.struMotionHandleType.dwHandleType & 0x20)
    {
        snprintf(MotionDetectParamBuf, MOTION_DETECT_PARAM_BUF_SIZE, " %s\n", "wireless, voice, light alarm together");
        strParamOut.append(MotionDetectParamBuf, strlen(MotionDetectParamBuf));
    }
    if (struPicCfg.struMotion.struMotionHandleType.dwHandleType & 0x40)
    {
        snprintf(MotionDetectParamBuf, MOTION_DETECT_PARAM_BUF_SIZE, " %s\n", "electronic map");
        strParamOut.append(MotionDetectParamBuf, strlen(MotionDetectParamBuf));
    }
    if (struPicCfg.struMotion.struMotionHandleType.dwHandleType & 0x200)
    {
        snprintf(MotionDetectParamBuf, MOTION_DETECT_PARAM_BUF_SIZE, " %s\n", "capture and ftp");
        strParamOut.append(MotionDetectParamBuf, strlen(MotionDetectParamBuf));
    }
    if (struPicCfg.struMotion.struMotionHandleType.dwHandleType & 0x1000)
    {
        snprintf(MotionDetectParamBuf, MOTION_DETECT_PARAM_BUF_SIZE, " %s\n", "capture and upload to clound");
        strParamOut.append(MotionDetectParamBuf, strlen(MotionDetectParamBuf));
    }

    snprintf(MotionDetectParamBuf, MOTION_DETECT_PARAM_BUF_SIZE, " %s\n", "channel for triggering alarm : { ");
    strParamOut.append(MotionDetectParamBuf, strlen(MotionDetectParamBuf));

    for(DWORD i = 0;i < MAX_ALARMOUT_V30;i ++)
    {
        if(!struPicCfg.struMotion.struMotionHandleType.byRelAlarmOut[i]) continue;

        snprintf(MotionDetectParamBuf, MOTION_DETECT_PARAM_BUF_SIZE, "  %d%c\n", i, ' ');
        strParamOut.append(MotionDetectParamBuf, strlen(MotionDetectParamBuf));
    }

    snprintf(MotionDetectParamBuf, MOTION_DETECT_PARAM_BUF_SIZE, "%s", " }\n}\n");
    strParamOut.append(MotionDetectParamBuf, strlen(MotionDetectParamBuf));

    // ptrLogger->info("\n{}", strParamOut.c_str());
    // std::cout << strParamOut << std::endl;

    BOOL IsNeedToSetCfg = false;

    if(!struPicCfg.struMotion.byEnableHandleMotion)
    {
        struPicCfg.struMotion.byEnableHandleMotion = 1;
        struPicCfg.struMotion.byMotionSensitive = 4;

        IsNeedToSetCfg = true;
    }

    if(IsNeedToSetCfg == true)
    {
        if(!NET_DVR_SetDVRConfig(lUserId, NET_DVR_SET_PICCFG_V30, 1, (void *)&struPicCfg, sizeof(struPicCfg)))
        {
            ptrLogger->error("set picture configuration, error code = {0:d}", NET_DVR_GetLastError());
            ptrLogger->flush();
            return false;
        }
    }

    return true;
}

BOOL HikDevOperate::HikDevStartAlarm(LONG lUserId)
{
    if(HikDevMotionDetectCfg(lUserId) == false)
    {
        return false;
    }

    if(!NET_DVR_SetDVRMessageCallBack_V30(HikDevAlarmCallback, nullptr))
    {
        return false;
    }

    NET_DVR_SETUPALARM_PARAM AlarmParam;
    memset((void *)&AlarmParam, 0, sizeof(AlarmParam));
    AlarmParam.dwSize = sizeof(AlarmParam);

    lAlarmHandle = NET_DVR_SetupAlarmChan_V41(lUserId, &AlarmParam);
    if(lAlarmHandle < 0)
    {
        ptrLogger->error("start alarme configuration, error code = {0:d}", NET_DVR_GetLastError());
        ptrLogger->flush();
        return false;
    }

    return true;
}

BOOL HikDevOperate::HikDevStopAlarm()
{
    if(!NET_DVR_CloseAlarmChan_V30(lAlarmHandle))
    {
        ptrLogger->error("stop alarme configuration, error code = {0:d}", NET_DVR_GetLastError());
        ptrLogger->flush();
        return false;
    }

    return true;
}

void HikDevOperate::HikDevOutputAlarmDevInfo(NET_DVR_ALARMER struAlarmDev, std::string &strOut)
{  
#define DEV_INFO_BUF_MAX_SIZE    64

    char DevInfoBuf[DEV_INFO_BUF_MAX_SIZE];

    snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, " %s\n", "alarm device information : {");
    strOut.append(DevInfoBuf, strlen(DevInfoBuf));

    if(struAlarmDev.byUserIDValid)
    {
        snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, "  %s%d\n", "user id : ", struAlarmDev.lUserID);
    }
    else
    {
        snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, "  %s\n", "user id : invalid");
    }
    strOut.append(DevInfoBuf, strlen(DevInfoBuf));

    if(struAlarmDev.bySerialValid)
    {
        snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, "  %s%s\n", "serial number : ", struAlarmDev.sSerialNumber);
    }
    else
    {
        snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, "  %s\n", "serial number : invalid");
    }
    strOut.append(DevInfoBuf, strlen(DevInfoBuf));

    if(struAlarmDev.byVersionValid)
    {
        snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, "  %s%d\n", "device version : ", struAlarmDev.dwDeviceVersion);
    }
    else
    {
        snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, "  %s\n", "device version : invalid");
    }
    strOut.append(DevInfoBuf, strlen(DevInfoBuf));

    if(struAlarmDev.byDeviceNameValid)
    {
        snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, "  %s%s\n", "device name : ", struAlarmDev.sDeviceName);
    }
    else
    {
        snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, "  %s\n", "device name : invalid");
    }
    strOut.append(DevInfoBuf, strlen(DevInfoBuf));

    if(struAlarmDev.byMacAddrValid)
    {
        char sMacStr[MACADDR_LEN * 2 + 1];

        HikDevMacHexToStr(struAlarmDev.byMacAddr, sMacStr, MACADDR_LEN * 2 + 1);

        snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, "  %s%s\n", "mac addr : ", sMacStr);
    }
    else
    {
        snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, "  %s\n", "mac addr : invalid");
    }
    strOut.append(DevInfoBuf, strlen(DevInfoBuf));

    if(struAlarmDev.byLinkPortValid)
    {
        snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, "  %s%u\n", "link port : ", struAlarmDev.wLinkPort);
    }
    else
    {
        snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, "  %s\n", "link port : invalid");
    }
    strOut.append(DevInfoBuf, strlen(DevInfoBuf));

    if(struAlarmDev.byDeviceIPValid)
    {
        snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, "  %s%s\n", "device ip : ", struAlarmDev.sDeviceIP);
    }
    else
    {
        snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, "  %s\n", "device ip : invalid");
    }
    strOut.append(DevInfoBuf, strlen(DevInfoBuf));

    if(struAlarmDev.bySocketIPValid)
    {
        snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, "  %s%s\n", "socket ip : ", struAlarmDev.sSocketIP);
    }
    else
    {
        snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, "  %s\n", "socket ip : invalid");
    }
    strOut.append(DevInfoBuf, strlen(DevInfoBuf));

    snprintf(DevInfoBuf, DEV_INFO_BUF_MAX_SIZE, " %s\n", "}");
    strOut.append(DevInfoBuf, strlen(DevInfoBuf));
}

void HikDevOperate::HikDevOutputAlarmInfo(uint32_t uiAlarmIdx)
{
    struct tm AlarmDateTime;

    localtime_r(&struAlarmMotionDetect[uiAlarmIdx].AlarmTime, &AlarmDateTime);

#define ALARM_INFO_BUF_MAX_SIZE    64

    char AlarmInfoBuf[ALARM_INFO_BUF_MAX_SIZE];
    std::string strAlarmInfo;

    snprintf(AlarmInfoBuf, ALARM_INFO_BUF_MAX_SIZE, "%s\n", "motion detect alarm : {");
    strAlarmInfo.append(AlarmInfoBuf, strlen(AlarmInfoBuf));

    HikDevOutputAlarmDevInfo(struAlarmMotionDetect[uiAlarmIdx].struAlarmDevInfo, strAlarmInfo);

    snprintf(AlarmInfoBuf, ALARM_INFO_BUF_MAX_SIZE, " %s%u\n", "channel No. : ", (DWORD)struAlarmMotionDetect[uiAlarmIdx].byChannel);
    strAlarmInfo.append(AlarmInfoBuf, strlen(AlarmInfoBuf));

    snprintf(AlarmInfoBuf, ALARM_INFO_BUF_MAX_SIZE, " %s%d-%d-%d %d:%d:%d\n", "alarm date and time : ",
             AlarmDateTime.tm_year + 1900, AlarmDateTime.tm_mon + 1, AlarmDateTime.tm_mday,
             AlarmDateTime.tm_hour, AlarmDateTime.tm_min, AlarmDateTime.tm_sec);
    strAlarmInfo.append(AlarmInfoBuf, strlen(AlarmInfoBuf));

    snprintf(AlarmInfoBuf, ALARM_INFO_BUF_MAX_SIZE, "%s\n", "}");
    strAlarmInfo.append(AlarmInfoBuf, strlen(AlarmInfoBuf));

    ptrLogger->info("\n{}", strAlarmInfo.c_str());
    ptrLogger->flush();

    std::cout << strAlarmInfo << std::endl;
}

void HikDevOperate::HikDevMacHexToStr(BYTE *bMacHex, char *sMacStr, DWORD dwMacStrLen)
{
    DWORD offset = 0;

    for(DWORD i = 0;i < MACADDR_LEN;i ++)
    {
        if(i <  MACADDR_LEN - 1)
        {
            int retLen = snprintf(sMacStr + offset, dwMacStrLen - offset, "%02x-", bMacHex[i]);
            offset += retLen;
        }
        else
        {
            snprintf(sMacStr + offset, dwMacStrLen - offset, "%02x", bMacHex[i]);
        }
    }
}

uint32_t HikDevOperate::HikDevGetAlarmNum(void)
{
    if(dwAlarmMotionDetectWr >= dwAlarmMotionDetectRd)
    {
        return (dwAlarmMotionDetectWr - dwAlarmMotionDetectRd);
    }
    else
    {
        return (ALARM_MOTION_DETECT_MAX - dwAlarmMotionDetectRd + dwAlarmMotionDetectWr);
    }
}

void HikDevOperate::HikDevSetAlarmIdxOffset(uint32_t offset)
{
    dwAlarmMotionDetectRd += offset;
    if(dwAlarmMotionDetectRd >= ALARM_MOTION_DETECT_MAX)
    {
        dwAlarmMotionDetectRd -= ALARM_MOTION_DETECT_MAX;
    }
}

void HikDevOperate::HikDevGetAlarmDateTime(uint32_t uiAlarmIdx, uint32_t &uiYear, uint32_t &uiMonth, uint32_t &uiDay,
                                   uint32_t &uiHour, uint32_t &uiMinute, uint32_t &uiSecond)
{
    uiAlarmIdx += dwAlarmMotionDetectRd;
    if(uiAlarmIdx >= ALARM_MOTION_DETECT_MAX)
    {
        uiAlarmIdx -= ALARM_MOTION_DETECT_MAX;
    }

    struct tm AlarmDateTime;

    localtime_r(&struAlarmMotionDetect[uiAlarmIdx].AlarmTime, &AlarmDateTime);

    uiYear = static_cast<uint32_t>(AlarmDateTime.tm_year) + 1900;
    uiMonth = static_cast<uint32_t>(AlarmDateTime.tm_mon) + 1;
    uiDay = static_cast<uint32_t>(AlarmDateTime.tm_mday);
    uiHour = static_cast<uint32_t>(AlarmDateTime.tm_hour);
    uiMinute = static_cast<uint32_t>(AlarmDateTime.tm_min);
    uiSecond = static_cast<uint32_t>(AlarmDateTime.tm_sec);
}

void HikDevOperate::HikDevGetAlarmIp(uint32_t uiAlarmIdx, std::string &strAlarmIp)
{
    uiAlarmIdx += dwAlarmMotionDetectRd;
    if(uiAlarmIdx >= ALARM_MOTION_DETECT_MAX)
    {
        uiAlarmIdx -= ALARM_MOTION_DETECT_MAX;
    }

    NET_DVR_ALARMER struAlarmDev;

    struAlarmDev = struAlarmMotionDetect[uiAlarmIdx].struAlarmDevInfo;

    if(struAlarmDev.byDeviceIPValid)
    {
        strAlarmIp.append(struAlarmDev.sDeviceIP);
    }
    else
    {
        strAlarmIp.clear();
    }
}

void HikDevOperate::HikDevGetAlarmDevName(uint32_t uiAlarmIdx, std::string &strAlarmDevName)
{
    uiAlarmIdx += dwAlarmMotionDetectRd;
    if(uiAlarmIdx >= ALARM_MOTION_DETECT_MAX)
    {
        uiAlarmIdx -= ALARM_MOTION_DETECT_MAX;
    }

    NET_DVR_ALARMER struAlarmDev;

    struAlarmDev = struAlarmMotionDetect[uiAlarmIdx].struAlarmDevInfo;

    if(struAlarmDev.byDeviceNameValid)
    {
        strAlarmDevName.append(struAlarmDev.sDeviceName);
    }
    else
    {
        strAlarmDevName.clear();
    }
}
