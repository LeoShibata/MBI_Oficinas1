
# src — Biblioteca do Módulo Biométrico Integrado (MBI)

Este diretório contém **somente** os módulos de firmware e headers destinados à placa **ESP32-2432S028R** (ESP32 + ILI9341 320×240 + touch XPT2046 + microSD).  
Abrange: **LVGL** (display/touch), **RTC (DS3231)** e **CSV em microSD**.  
Não contém nem documenta arquivos de aplicação fora de `src/`.

---

## Estrutura de diretórios

```

src/
├─ LVGL_Display.h
├─ LVGL_Display.cpp
├─ RTC_Module.h
├─ RTC_Module.cpp
├─ SD_Module.h
├─ SD_Module.cpp
└─ UI/                 # gerado pelo SquareLine (não editar)
├─ ui.h
├─ ui_helpers.h
└─ ui_events.h

````

- **Editar/estender:** `LVGL_Display.*`, `RTC_Module.*`, `SD_Module.*`
- **Não editar:** qualquer arquivo dentro de `src/UI/` (regerado pelo SquareLine)

---

## Dependências de terceiros (alvo de integração)

- **Core ESP32 (Arduino):** recomendado `2.0.14`
- **LVGL:** `8.3.11`
- **TFT_eSPI** (controlador ILI9341)
- **TFT_Touch** (controlador XPT2046)
- **RTClib** (DS3231)
- **SD** (biblioteca SD do core ESP32)

> Estas dependências são usadas **pelos módulos dentro de `src/`**; sua obtenção/instalação é responsabilidade do ambiente superior.

---

## Pinos e interfaces relevantes

- **SPI** compartilhado (display, touch, SD): fiação fixa da 2432S028R  
- **CS (chip-select) do microSD:** **GPIO 5**
- **I²C** recomendado para RTC/sensores: **SDA = GPIO 21**, **SCL = GPIO 22**
- **Observação:** `GPIO 35` é **entrada** (ADC somente input)

---

## Ordem sugerida de chamadas (nível de biblioteca)

1. `lvgl_display_init()`  
2. `ui_init()`  *(fornecido em `src/UI/`)*  
3. `rtc_init()`  
4. `CSV_begin()`  
5. `openDailyIfNeeded()`  
6. `CSV_appendRow(...)` conforme aquisição de sensores  
7. `CSV_close()` ao finalizar sessão ou desligamento controlado

> A integração e o agendamento dessas chamadas cabem à aplicação que consome `src/`.

---

## Módulos e APIs

### LVGL_Display
**Arquivos:** `LVGL_Display.h/.cpp`  
**Responsabilidade:** inicialização do display ILI9341 (via TFT_eSPI), integração do touch XPT2046 (TFT_Touch), registro de *flush* e *input* com a LVGL.

**API pública:**
```cpp
void lvgl_display_init();
````

---

### RTC_Module

**Arquivos:** `RTC_Module.h/.cpp`
**Responsabilidade:** encapsular o DS3231 (RTClib) e fornecer leitura de data/hora.

**API pública:**

```cpp
void     rtc_init();
DateTime rtc_getTime();
```

**Comportamento:**

* Se o RTC estiver sem hora, o módulo ajusta para a data/hora de compilação como *fallback*.
* Em caso de ausência física do RTC, `rtc_getTime()` não é confiável; ver política de *fallback* do CSV abaixo.

---

### SD_Module

**Arquivos:** `SD_Module.h/.cpp`
**Responsabilidade:** estrutura de diretórios no microSD e gravação de linhas CSV com metadados padronizados.

**API pública:**

```cpp
void  CSV_begin();
bool  openDailyIfNeeded();
bool  CSV_appendRow(const char* userId,
                    const char* sessionId,
                    const char* sensor,
                    double      valor,
                    const char* unidade);
void  CSV_close();
```

**Regras internas (resumo):**

* Garante existência dos diretórios-base.
* Abre/cria o arquivo do **dia corrente** (ou `UNSET` caso RTC inválido).
* Escreve **cabeçalho** se o arquivo estiver vazio.
* Ao detectar arquivo antigo **sem cabeçalho**, renomeia para `*_nohdr.csv` e cria um novo arquivo com cabeçalho.

---

## CSV — Especificação funcional

### Layout de pastas

* Arquivos por dia em: `MBI/CSV/<ANO>/MBI_<YYYYMMDD>.csv`
* Se a data **não** puder ser obtida (RTC inválido): `MBI/CSV/UNSET/MBI_UNSET.csv`

### Colunas e formatos

1. **DataHora** — `YYYY-MM-DD HH:MM:SS`
2. **UserID** — string sanitizada (sem vírgula/quebra de linha)
3. **SessionID** — `YYYYMMDD-XX`
4. **Sensor** — `TEMP | BPM | SpO2 | GSR | ECG_BPM`
5. **Valor** — número decimal (`.`)
6. **Unidade** — `C | bpm | % | kOhm | mV | ...`
7. **DeviceID** — ex.: `MBI-ESP32-01`
8. **FwVersion** — ex.: `v0.3.0`
9. **TZ** — ex.: `-03:00`
10. **Seq** — contador incremental por arquivo

### Limites e robustez

* **Tamanho por arquivo:** 50 MB (sem rotação automática por ora)
* **Flush:** MVP efetua `flush()` a cada `append` (simples e seguro).
* **Falhas de escrita:** pausa tentativa, mantém dados em memória até reabertura.
* **Sem SD:** sinalizar para a camada de UI desabilitar ações de gravação.

### Manifest (opcional)

* Arquivo: `MBI.manifest.csv`
* Campos: `Arquivo,Data,Inicio,Termino,Linhas,DeviceID,FwVersion,Hash`
* Pode ser preenchido quando o arquivo diário for fechado pela aplicação.

---

## Convenções internas de implementação

* **Estado estático** encapsulado em **namespace anônimo** (escopo do `.cpp`).
* **Constantes** no padrão `kCamelCase` (ex.: `kHeader`, `kFwVersion`).
* **Buffers fixos** e `snprintf()` no caminho crítico de I/O para evitar alocação dinâmica.
* **Serial logs** claros para diagnóstico (erros de `begin/open/append/flush`).
* **Sem dependência cíclica** entre módulos; `SD_Module` consome `RTC_Module` apenas para data/hora.

---

## Diagnóstico (mensagens esperadas)

* `[SD] OK! CS=5` — cartão inicializado
* `[SD] mkdir ...` / `ERRO: mkdir ...` — criação de diretórios
* `[SD] opened: /MBI/CSV/YYYY/MBI_YYYYMMDD.csv` — arquivo ativo do dia
* `[SD] header written` — cabeçalho gerado
* `[SD] append ok` / `append FAIL` — resultado da escrita da linha

> A aplicação superior pode consumir estas mensagens para telemetria/UX.

---

## Compatibilidade e versões alvo

* **ESP32 core:** 2.0.14
* **LVGL:** 8.3.11
* **Armazenamento:** microSD em **FAT32**
* **CS do SD:** GPIO **5**

---

## Coisas para fazer

**Curto prazo:**

* Sanitização consistente de `UserID`/`SessionID` (remover vírgulas, CR/LF, limitar comprimento).
* Opção de **flush temporizado** (ex.: a cada 2 s) em vez de `flush()` por `append`.
* Sinalização para UI: desabilitar ações quando `sd_ok == false` e exibir toasts de erro de gravação.

**Médio prazo:**

* Integração de sensores: **MAX30102** (BPM/SpO₂), **MLX90614** (TEMP), **GSR** (kΩ), **ECG_BPM** (mV + derivação de BPM).
* Geração de `SessionID` na entrada do modo de registro (`YYYYMMDD-##`).
* Se houver múltiplas tarefas de aquisição: proteger SD com **mutex** e centralizar I/O numa **task** de logger.

**Depois (se necessário):**

* Rotação por tamanho (50 MB) com sufixos incrementais e atualização de `manifest`.
* Versão de cabeçalho no CSV + rotina de migração.
* Card-detect (se disponível no hardware) para *hotplug* mais amigável.
* `manifest` com `Hash` (CRC32 ou SHA-1) ao fechar arquivo diário.

---

