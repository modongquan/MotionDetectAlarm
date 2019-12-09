CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
#DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

QMAKE_LFLAGS += -Wl,-rpath,../lib

SOURCES += \
    ../src/CsvParse.cpp \
    ../src/DataBase.cpp \
    ../src/HikDev.cpp \
    ../src/SerialPort.cpp \
    ../src/main.cpp

HEADERS += \
    ../src/CsvParse.h \
    ../src/DataBase.h \
    ../src/HikDev.h \
    ../src/SerialPort.h \
    ../src/csv.h

LIBS += \
    -L../lib -lmysqlclient -lhcnetsdk \
#    ../lib/libmysqlclient.so \
#    ../lib/libhcnetsdk.so \
#    ../lib/libAudioRender.so \
#    ../lib/libcrypto.so \
#    ../lib/libcrypto.so.1.0.0 \
#    ../lib/libNPQos.so \
#    ../lib/libssl.so \
#    ../lib/libHCCore.so \
#    ../lib/libhpr.so \
#    ../lib/libPlayCtrl.so \
#    ../lib/libSuperRender.so \
#    ../lib/HCNetSDKCom/libanalyzedata.so \
#    ../lib/HCNetSDKCom/libHCCoreDevCfg.so \
#    ../lib/HCNetSDKCom/libHCGeneralCfgMgr.so \
#    ../lib/HCNetSDKCom/libHCPlayBack.so \
#    ../lib/HCNetSDKCom/libHCVoiceTalk.so \
#    ../lib/HCNetSDKCom/libStreamTransClient.so \
#    ../lib/HCNetSDKCom/libHCAlarm.so \
#    ../lib/HCNetSDKCom/libHCDisplay.so \
#    ../lib/HCNetSDKCom/libHCIndustry.so \
#    ../lib/HCNetSDKCom/libHCPreview.so \
#    ../lib/HCNetSDKCom/libiconv2.so \
#    ../lib/HCNetSDKCom/libSystemTransform.so

INCLUDEPATH += \
    ../inc \
    ../inc/spdlog

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
