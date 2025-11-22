#include "SD_Module.h"
#include "RTC_Module.h"
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <cstring>

// Estado interno do módulo (static → escopo do TU, evita colisões)
static bool sd_ok = false;
static File s_file;
static bool isOpen = false;
static char current_path[64] = {0};
static SPIClass spi_sd(VSPI);

namespace {
    // Pinos/constantes do SD
    const int sd_cs = 5;

    // Estado do arquivo do dia (YYYYMMDD)
    int current_yyyymmdd = -1;

    // Cabeçalho e metadados fixos
    const char* kHeader    = "DataHora,UserID,SessionID,Sensor,Valor,Unidade,DeviceID,FwVersion,TZ,Seq";
    const char* kDeviceID  = "MBI-ESP32-01";
    const char* kFwVersion = "v0.1.0";
    const char* kTzStr     = "-03:00";

    // Sequência monotônica
    uint32_t kSeq = 0;
}

// ------------------ Helpers de tempo ------------------
static bool rtc_is_valid(const DateTime& t) {
    // por quê: evita escrever datas inválidas
    return (t.year()  >= 2020 && t.year()  <= 2099 &&
            t.month() >= 1    && t.month() <= 12   &&
            t.day()   >= 1    && t.day()   <= 31   &&
            t.hour()  <  24   && t.minute()<  60   && t.second()<60);
}

static DateTime rtc_now_safe(bool* ok = nullptr) {
    DateTime t = rtc_getTime();
    bool valid = rtc_is_valid(t);
    if (ok) *ok = valid;
    if (!valid) return DateTime(2000, 1, 1, 0, 0, 0);
    return t;
}

// ------------------ Helpers de caminho ------------------
static void build_path_safe(char* dir_year, size_t dir_sz,
                            char* file_path, size_t path_sz,
                            int* today_out, bool* rtc_ok_out)
{
    bool ok = false;
    DateTime now = rtc_now_safe(&ok);

    if (ok) {
        if (dir_year)
            snprintf(dir_year, dir_sz, "/MBI/CSV/%04d", now.year());
        if (file_path)
            snprintf(file_path, path_sz, "/MBI/CSV/%04d/MBI_%04d%02d%02d.csv",
                     now.year(), now.year(), now.month(), now.day());
        if (today_out)
            *today_out = now.year()*10000 + now.month()*100 + now.day();
    } else {
        if (dir_year)
            snprintf(dir_year, dir_sz, "/MBI/CSV/UNSET");
        if (file_path)
            snprintf(file_path, path_sz, "/MBI/CSV/UNSET/MBI_UNSET.csv");
        if (today_out)
            *today_out = 20000101;
    }
    if (rtc_ok_out) *rtc_ok_out = ok;
}

// ------------------ SD / Arquivos ------------------
static bool check_SD() {
    if (sd_ok) return true;                  // evita reinit/logs desnecessários

    Serial.println("Inicializando barramento SPI para o SD Card...");
    spi_sd.begin(18, 19, 23);                // VSPI: SCK=18, MISO=19, MOSI=23
    sd_ok = SD.begin(sd_cs, spi_sd);
    if (!sd_ok) {
        Serial.println("Cartão SD não inserido ou falha na leitura.");
        return false;
    }
    Serial.println("Cartão SD lido com sucesso");
    return true;
}

void CSV_init() {
    if (!check_SD()) return;

    if (!SD.exists("/MBI")) {
        Serial.println("Criando pasta 'MBI'");
        if (!SD.mkdir("/MBI")) {
            Serial.println("ERRO: mkdir /MBI");
            return;
        }
    }
    if (!SD.exists("/MBI/CSV")) {
        Serial.println("Criando pasta 'CSV' em '/MBI/'.");
        if (!SD.mkdir("/MBI/CSV")) {
            Serial.println("ERRO: mkdir /MBI/CSV");
            return;
        }
    }

    char dir_year[32];
    // simples: só precisamos do diretório do ano/UNSET aqui
    build_path_safe(dir_year, sizeof(dir_year), /*file_path*/nullptr, /*path_sz*/0, nullptr, nullptr);

    if (!SD.exists(dir_year)) {
        Serial.print("Criando pasta do ano (ou UNSET): ");
        Serial.println(dir_year);
        if (!SD.mkdir(dir_year)) {
            Serial.print("ERRO: mkdir ");
            Serial.println(dir_year);
            return;
        }
    }
}

static bool openDailyIfNeeded() {
    if (!sd_ok && !check_SD()) return false;

    int today = -1;
    char dir_year[32];
    char path[64];
    bool rtc_ok = false;
    build_path_safe(dir_year, sizeof(dir_year), path, sizeof(path), &today, &rtc_ok);

    // Se já está aberto e é o mesmo dia, não faz nada.
    if (isOpen && current_yyyymmdd == today) return true;

    // Troca de dia ou ainda não aberto → fecha e prepara.
    if (isOpen) {
        s_file.flush();
        s_file.close();
        isOpen = false;
        current_path[0] = '\0';
    }
    current_yyyymmdd = -1;

    CSV_init(); // garante diretórios antes de abrir

    File f = SD.open(path, FILE_APPEND);
    if (!f) {
        Serial.print("[SD] ERRO: não foi possível abrir "); Serial.println(path);
        return false;
    }
    const bool need_header = (f.size() == 0);

    s_file = f;             // cópia por valor (ok no ESP32)
    isOpen = true;
    current_yyyymmdd = today;

    // Atualiza caminho atual (nome real pode diferir do "path" no SD)
    const char* nm = s_file.name();
    if (nm && *nm) {
        strncpy(current_path, nm, sizeof(current_path));
    } else {
        strncpy(current_path, path, sizeof(current_path));
    }
    current_path[sizeof(current_path)-1] = '\0';

    if (need_header) {
        s_file.println(kHeader);
        s_file.flush();
    }
    return true;
}

bool CSV_appendRow(const char* userId,
                   const char* sessionId,
                   const char* sensor,
                   double valor,
                   const char* unidade)
{
    if (!openDailyIfNeeded()) return false;

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
    s_file.flush(); // prioriza segurança sobre throughput
    return ok;
}

void CSV_close() {
    if (isOpen) {
        s_file.flush();
        s_file.close();
        isOpen = false;
    }
    current_yyyymmdd = -1;
    current_path[0] = '\0';
}

const char* CSV_get_current_path(void) {
    if (isOpen && sd_ok && current_path[0] != '\0') {
        return current_path;
    }
    return NULL;
}
