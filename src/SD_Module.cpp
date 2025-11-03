#include "SD_Module.h"
#include "RTC_Module.h"
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <cstring>

bool sd_ok = false;
File s_file;
bool isOpen = false; 
char current_path[64] = {0};

SPIClass spi_sd(VSPI);

namespace {
    const int sd_cs = 5;

    int  current_yyyymmdd = -1;
    int  current_suffix = 0;

    constexpr uint32_t kMaxBytes = 50u * 1024u * 1024u;

    const char* kHeader    = "DataHora,UserID,SessionID,Sensor,Valor,Unidade,DeviceID,FwVersion,TZ,Seq";
    const char* kDeviceID  = "MBI-ESP32-01";
    const char* kFwVersion = "v0.1.0";
    const char* kTzStr     = "-03:00";

    uint32_t kSeq = 0;
}

static bool rtc_is_valid(const DateTime& t) {
    return (t.year()  >= 2020 && t.year()  <= 2099 &&
            t.month() >= 1    && t.month() <= 12   &&
            t.day()   >= 1    && t.day()   <= 31   &&
            t.hour()  <  24   && t.minute()<  60   && t.second()<60);
}

static DateTime rtc_now_safe(bool* ok = nullptr) {
    DateTime t = rtc_getTime();
    bool valid = rtc_is_valid(t);
    if (ok) 
        *ok = valid;
    if (!valid) 
        return DateTime(2000, 1, 1, 0, 0, 0);
    return t;
}

void build_path_safe(char* dir_year, size_t dir_sz,
                     char* file_path, size_t path_sz,
                     int* today_out, bool* rtc_ok_out)
{
    bool ok = false;
    DateTime now = rtc_now_safe(&ok);

    if(ok) {
        if(dir_year)
            snprintf(dir_year, dir_sz, "/MBI/CSV/%04d", now.year());
        if(file_path)
            snprintf(file_path, path_sz, "/MBI/CSV/%04d/MBI_%04d%02d%02d.csv",
                     now.year(), now.year(), now.month(), now.day());
        if(today_out)
            *today_out = now.year()*10000 + now.month()*100 + now.day();
    } else {
        if(dir_year)
            snprintf(dir_year, dir_sz, "/MBI/CSV/UNSET");
        if(file_path)
            snprintf(file_path, path_sz, "/MBI/CSV/UNSET/MBI_UNSET.csv");
        if(today_out)
            *today_out = 20000101;
    }

    if (rtc_ok_out) 
        *rtc_ok_out = ok;
}

static bool check_SD() {
    Serial.println("Inicializando barramento SPI para o SD Card...");
    
    // Inicializa o barramento SPI
    spi_sd.begin(18, 19, 23);
    sd_ok = SD.begin(sd_cs, spi_sd);
    if(!sd_ok){
        Serial.println("Cartão SD não inserido ou falha na leitura.");
        return false;
    }
    Serial.println("Cartão SD lido com sucesso");
    return true;
}

void CSV_init() {
    if(!check_SD()) {
        return;
    }
  
    if(!SD.exists("/MBI")) {
        Serial.println("Criando pasta 'MBI'");
        if(!SD.mkdir("/MBI")) {
            Serial.println("ERRO: mkdir /MBI");
            return;
        }
        Serial.println("Pasta 'MBI' criada.");
    }
  
    if(!SD.exists("/MBI/CSV")) {
        Serial.println("Criando pasta 'CSV' em '/MBI/'.");
        if (!SD.mkdir("/MBI/CSV")) {
            Serial.println("ERRO: mkdir /MBI/CSV");
            return;
        }
        Serial.println("Pasta 'CSV' criada.");
    }

    char dir_year[32];
    char dummy_path[4];
    build_path_safe(dir_year, sizeof(dir_year), dummy_path, sizeof(dummy_path), nullptr, nullptr);

    if(!SD.exists(dir_year)) {
        Serial.print("Criando pasta do ano (ou UNSET): ");
        Serial.println(dir_year);
        if (!SD.mkdir(dir_year)) {
            Serial.print("ERRO: mkdir ");
            Serial.println(dir_year);
            return;
        }
        Serial.println("Pasta criada.");
    }
}

static int getSuffix(File file) {
    const char* name = file.name();
    if(!name || !*name) 
        return 0;

    const char* us  = strrchr(name, '_');
    const char* dot = strrchr(name, '.');
    if(!us || !dot || dot <= us + 1) 
        return 0;

    int val = 0;
    for(const char* p = us + 1; p < dot; ++p) {
        if (*p < '0' || *p > '9') 
            return 0;
        val = val * 10 + (*p - '0');
        if (val > 9999) 
            break;
    }
    return val;
}

bool openDailyIfNeeded() {
    if(!sd_ok && !check_SD()) {
        return false;
    }

    int today = -1;
    char dir_year[32];
    char path[64];
    bool rtc_ok = false;
    build_path_safe(dir_year, sizeof(dir_year), path, sizeof(path), &today, &rtc_ok);

    if (isOpen && current_yyyymmdd == today) {
        return true;
    }

    if(isOpen && current_yyyymmdd != today) {
        s_file.flush();
        s_file.close();
        isOpen = false;
        current_yyyymmdd = -1;
        current_suffix = 0;
        current_path[0] = '\0';
    }

    CSV_init();

    File f = SD.open(path, FILE_APPEND);
    if (!f) {
        Serial.print("[SD] ERRO: não foi possível abrir "); Serial.println(path);
        return false;
    }

    const bool need_header = (f.size() == 0);

    s_file = f;
    isOpen = true;
    current_yyyymmdd = today;
    current_suffix = 0; 

    strncpy(current_path, path, sizeof(current_path));
    current_path[sizeof(current_path)-1] = '\0';

    const char* nm = s_file.name();
    if (nm && *nm) {
        strncpy(current_path, nm, sizeof(current_path));
        current_path[sizeof(current_path)-1] = '\0';
    } else {
        strncpy(current_path, path, sizeof(current_path));
        current_path[sizeof(current_path)-1] = '\0';
    }

    if(need_header) {
      s_file.println(kHeader);
      s_file.flush();
    }
    
    return true;
}

bool CSV_rotateIfNeeded() {
    return true;
}

bool CSV_appendRow(const char* userId,
                   const char* sessionId,
                   const char* sensor,
                   double valor,
                   const char* unidade)
{
    if(!openDailyIfNeeded()) 
        return false;
    if(!CSV_rotateIfNeeded()) 
        return false;

    bool rtc_ok = false;
    DateTime now = rtc_now_safe(&rtc_ok);

    char dt[24];
    if (rtc_ok) {
        snprintf(dt, sizeof(dt), "%04d-%02d-%02d %02d:%02d:%02d",
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second());
    } else {
        snprintf(dt, sizeof(dt), "2000-01-01 00:00:00");
    }

    char valorStr[24];
    snprintf(valorStr, sizeof(valorStr), "%.6f", valor);

    char line[256];
    snprintf(line, sizeof(line),
            "%s,%s,%s,%s,%s,%s,%s,%s,%s,%lu",
            dt,
            (userId    ? userId    : "-"),
            (sessionId ? sessionId : "-"),
            (sensor    ? sensor    : "-"),
            valorStr,
            (unidade   ? unidade   : "-"),
            kDeviceID,
            kFwVersion,
            kTzStr,
            (unsigned long)++kSeq);

    bool ok = s_file.println(line);
    s_file.flush();
    return ok;
}

void CSV_close() {
    if (isOpen) {
        s_file.flush();
        s_file.close();
        isOpen = false;
    }
    current_yyyymmdd = -1;
    current_suffix = 0;
    current_path[0] = '\0';
}

const char* CSV_get_current_path(void) {
    if (isOpen && sd_ok && current_path[0] != '\0') {
        return current_path;
    }
    return NULL;
}