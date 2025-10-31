# src/ — Módulo Biométrico Integrado (MBI)

Firmware modular para a **ESP32-2432S028R** (ESP32 + ILI9341 320×240 + touch resistivo XPT2046 + microSD), com:

* UI em **LVGL 8.3.11** (gerada no SquareLine)
* Relógio **DS3231** (RTClib)
* Registro de dados em **CSV** no microSD (biblioteca `SD` do core ESP32)

---

## Sobre o CSV

### Layout de Pastas

* O CSV será salvo em `MBI/CSV/<ANO>/MBI_<YYYYMMDD>.csv`;
* As colunas serão: `DataHora,UserID,SessionID,Sensor,Valor,Unidade,DeviceID,FwVersion,TZ,Seq`;
* Cada dia terá seu próprio arquivo CSV;
* Cada arquivo terá **limite de tamanho de 50 MB**;
* Acontecerá um **flush** a cada **2 segundos** no buffer (no MVP atual o módulo chama `flush()` após cada `append`; a política de 2 s pode ser aplicada ao integrar com o loop/task).

### Formato das Colunas

* **DataHora:** `YYYY-MM-DD HH:MM:SS`
* **UserID:** texto digitado e sanitizado
* **SessionID:** `YYYYMMDD-XX`
* **Sensor:** `TEMP | BPM | SpO2 | GSR | ECG_BPM`
* **Valor:** número com ponto decimal
* **Unidade:** `C | bpm | % | kOhm | mV | ...`
* **DeviceID:** ex.: `MBI-ESP32-01`
* **FwVersion:** ex.: `v0.3.0`
* **TZ:** `-03:00` (timezone)
* **Seq:** contador por arquivo

### Manifest.csv

* Em `MBI.manifest.csv`
* Campos: `Arquivo,Data,Inicio,Termino,Linhas,DeviceID,FwVersion,Hash`

### Robustez

* Sem SD, a ação **“Novo Registro”** fica desabilitada e mostra aviso (UI).
* Falha de escrita pausa a gravação, mantém dados no buffer e tenta reabrir o arquivo.
* Desconexão abrupta pode causar perda das últimas linhas que não foram `flush()`.

---

## Sobre o MicroSD

* Formato **FAT32**
* **CS = 5** (chip select do SD)

---

## Sumário

* Arquitetura e Fluxo
* Estrutura de Pastas (src/)
* Dependências
* Pinagem e Interfaces
* Ordem de Inicialização
* Módulos

  * LVGL_Display
  * RTC_Module
  * SD_Module
* Exemplo Rápido
* Teste de Gravação (N linhas)
* Dicas de Build
* Soluções de Problemas
* Convenções Internas

---

## Arquitetura e Fluxo

1. Inicializa **Display + Touch + LVGL**.
2. Carrega **UI** (arquivos gerados pelo SquareLine).
3. Inicia **RTC (DS3231)**; se sem hora, usa timestamp de compilação.
4. Garante **estrutura no SD** e abre/cria o CSV do dia.
5. **Registro**: sensores escrevem linhas CSV com timestamp e metadados.

---

## Estrutura de Pastas (src/)

```
src/
├─ LVGL_Display.h
├─ LVGL_Display.cpp
├─ RTC_Module.h
├─ RTC_Module.cpp
├─ SD_Module.h
├─ SD_Module.cpp
└─ UI/                # arquivos gerados pelo SquareLine
   ├─ ui.h
   ├─ ui_helpers.h
   └─ ui_events.h
```

Observação: não edite arquivos em `src/UI/`. Conecte callbacks no seu código fora dessa pasta.

---

## Dependências

* **Core ESP32 (Arduino):** recomendado `2.0.14`
* **LVGL:** `8.3.11`
* **TFT_eSPI** (configurada para **ILI9341**)
* **TFT_Touch** (controlador **XPT2046**)
* **RTClib** (DS3231)
* **SD** (biblioteca da core ESP32)

---

## Pinagem e Interfaces

* **SPI** (display, touch, SD): fixo no módulo; **CS do SD = GPIO 5**.
* **Touch (TFT_Touch):** exemplo comum — `DOUT=39`, `DIN=32`, `DCS=33`, `DCLK=25`.
* **I²C** (RTC + sensores futuros): recomendação padrão **SDA=21, SCL=22**.
* Observação: **IO35 é apenas entrada (ADC)**.

---

## Ordem de Inicialização

1. `lvgl_display_init()` — display, LVGL e touch
2. `ui_init()` — telas geradas pelo SquareLine
3. `rtc_init()` — DS3231
4. `CSV_begin()` — SD e estrutura de diretórios
5. Registro: `openDailyIfNeeded()` → `CSV_appendRow(...)`

---

## Módulos

### LVGL_Display

* **Arquivos:** `LVGL_Display.h/.cpp`
* **Responsabilidade:** inicializa TFT_eSPI, LVGL e touch; registra `my_disp_flush` e `my_touchpad_read`.
* **API:** `void lvgl_display_init();`

### RTC_Module

* **Arquivos:** `RTC_Module.h/.cpp`
* **Responsabilidade:** encapsula RTClib/DS3231.
* **APIs:**

  * `void rtc_init();`
  * `DateTime rtc_getTime();`

### SD_Module

* **Arquivos:** `SD_Module.h/.cpp`

* **Responsabilidade:** criar estrutura de pastas, abrir e escrever CSV com **fallback** quando o RTC não está válido.

* **Caminhos:**

  * RTC **válido**: `/MBI/CSV/<ANO>/MBI_<YYYYMMDD>.csv`
  * RTC **inválido/ausente**: `/MBI/CSV/UNSET/MBI_UNSET.csv`

* **APIs públicas:**

  * `void CSV_begin();`
  * `bool openDailyIfNeeded();`
  * `bool CSV_rotateIfNeeded();`  (stub; sempre `true` no MVP; rotação por 50 MB planejada)
  * `bool CSV_appendRow(const char* userId, const char* sessionId, const char* sensor, double valor, const char* unidade);`
  * `void CSV_close();`

* **Comportamentos importantes:**

  * Se o arquivo não existir ou estiver vazio → escreve **cabeçalho**.
  * Se o arquivo existir **sem cabeçalho** → renomeia para `*_nohdr.csv` e cria um novo com cabeçalho.
  * Sem RTC válido → grava em `UNSET` e usa timestamp `2000-01-01 00:00:00`.

---

## Exemplo Rápido

```cpp
#include <Wire.h>
#include "src/LVGL_Display.h"
#include "src/RTC_Module.h"
#include "src/SD_Module.h"

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  lvgl_display_init();
  ui_init();
  rtc_init();
  CSV_begin();
  openDailyIfNeeded();

  // Exemplo: registrar um valor simples
  CSV_appendRow("USER123", "20251030-00", "PING", 1.0, "-");
}

void loop() {
  // processamento normal do app
}
```

---

## Teste de Gravação (N linhas)

```cpp
#include <Wire.h>
#include "src/RTC_Module.h"
#include "src/SD_Module.h"

const int kLines = 20;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  rtc_init();
  CSV_begin();

  if (!openDailyIfNeeded()) {
    Serial.println("[TEST] ERRO: não abriu/criou o arquivo do dia.");
    return;
  }

  for (int i = 1; i <= kLines; ++i) {
    bool ok = CSV_appendRow("TESTE", "YYYYMMDD-00", "PING", (double)i, "-");
    Serial.printf("[TEST] linha %d %s\n", i, ok ? "OK" : "FALHA");
    delay(5);
  }

  CSV_close();
  Serial.println("[TEST] Concluído. Verifique o CSV no cartão.");
}

void loop() {}
```

---

## Dicas de Build

* **Board:** ESP32 Dev Module
* **Partition Scheme:** use uma opção com **APP ≥ 2 MB** (ex.: *No OTA (2MB APP/2MB SPIFFS)*) por conta do tamanho de LVGL + UI.
* **microSD:** **FAT32** (exFAT não é suportado por `SD` da core ESP32).
* **Único `setup()/loop()`:** por sketch/pasta, deve haver **um** par `setup/loop`.

---

## Soluções de Problemas

* **Sem cabeçalho no CSV:** arquivos antigos são renomeados para `*_nohdr.csv` e um novo é criado com cabeçalho.
* **RTC desconectado:** grava em `/MBI/CSV/UNSET/MBI_UNSET.csv` com timestamp de fallback.
* **Falha ao abrir/gravar:** verifique `CS=5`, formatação **FAT32**, mensagens “Cartão SD lido com sucesso”, e o caminho impresso no Serial.
* **Erros de compilação por múltiplos `.ino`:** o Arduino concatena `.ino`; mantenha **apenas um** `setup/loop` por pasta.

---

## Convenções Internas

* Estado estático encapsulado em **namespace anônimo** dentro dos `.cpp`.
* Constantes nomeadas como `kCamelCase` (ex.: `kHeader`, `kFwVersion`).
* Somente APIs **necessárias** são expostas nos headers; helpers e detalhes de arquivo ficam privados no `.cpp`.
