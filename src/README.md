
# src — Biblioteca do Módulo Biométrico Integrado (MBI)

Este diretório contém **somente** os módulos e headers de firmware para a **ESP32-2432S028R** (ESP32 + ILI9341 320×240 + touch XPT2046 + microSD).  
Inclui: **LVGL** (display/touch), **RTC (DS3231)** e **CSV em microSD**.  
**Não** documenta a aplicação de alto nível; apenas a API e o comportamento dos módulos dentro de `src/`.

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

```

- **Editar/estender:** `LVGL_Display.*`, `RTC_Module.*`, `SD_Module.*`  
- **Não editar:** qualquer arquivo dentro de `src/UI/` (regerado pelo SquareLine)

---

## Dependências (bibliotecas externas)

- **Core ESP32 (Arduino):** recomendado `2.0.14`  
- **LVGL:** `8.3.11`  
- **TFT_eSPI** (controlador ILI9341)  
- **TFT_Touch** (controlador XPT2046)  
- **RTClib** (DS3231)  
- **SD** (biblioteca SD do core ESP32)

> A instalação/gestão dessas libs é responsabilidade do ambiente superior. Os módulos em `src/` apenas as consomem.

---

## Pinos e interfaces relevantes

- **SPI** compartilhado (display, touch, SD): fiação fixa da 2432S028R.  
- **CS (chip-select) do microSD:** **GPIO 5**.  
- **I²C** recomendado para RTC/sensores: **SDA = GPIO 21**, **SCL = GPIO 22**.  
- **Observação:** `GPIO 35` é **apenas entrada** (ADC).

---

## LVGL_Display — implementação atual (LVGL_Display.cpp)

**Resumo do que o módulo faz:**
- Inicializa o TFT (TFT_eSPI), define rotação **1** (landscape) e limpa a tela.  
- Inicializa o touch (TFT_Touch) **com calibração fixa**: `touch.setCal(526, 3443, 750, 3377, screenWidth, screenHeight, 1)`  
- Inicializa a LVGL, cria **um único draw buffer** estático e registra:
  - `my_disp_flush(...)` → envia a área renderizada para o ILI9341 via `tft.pushColors(...)`;
  - `my_touchpad_read(...)` → lê o XPT2046 e alimenta o input pointer da LVGL.
- Registra `disp_drv` (display) e `indev_drv` (touch) na LVGL.

**Pinos do touch usados na implementação:**
```

DOUT = 39 (T_DO)
DIN  = 32 (T_DIN)
DCS  = 33 (T_CS)
DCLK = 25 (T_CLK)

````

**Buffers e tamanhos:**
- `screenWidth = 320`, `screenHeight = 240` (definidos no header).  
- `static lv_color_t buf[screenWidth * screenHeight / 4];` cria um buffer de ~19.200 pixels (¼ da tela).  
- Na inicialização do draw buffer, **o terceiro parâmetro** (segundo buffer) é **nulo** (single buffer).  
  - Boas-práticas: o parâmetro `size_in_px_cnt` deve corresponder ao tamanho real do buffer (em **pixels**). Em geral, usar `sizeof(buf) / sizeof(buf[0])`.

**Notas úteis:**
- Para C++, prefira `nullptr` em vez de `NULL` ao indicar ausência de segundo buffer.  
- Se houver “*flicker*” ou subutilização do buffer, ajuste o `size_in_px_cnt` para refletir o tamanho exato do `buf`.  
- A calibração do touch é específica do módulo/lote; re-calibre se necessário e atualize os parâmetros no código.

**API pública deste módulo:**
```cpp
void lvgl_display_init();
````

---

## RTC_Module — API e comportamento

**API pública:**

```cpp
void     rtc_init();
DateTime rtc_getTime();
```

**Comportamentos relevantes (utilizados pelo SD_Module):**

* Validação de data/hora por faixa simples (anos 2020–2099, campos válidos).
* *Fallback* seguro: quando o RTC não é válido/ausente, usa-se `2000-01-01 00:00:00` para registro.

---

## SD_Module — API, caminhos e comportamento

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

// Mantido como stub (sem rotação automática atualmente)
bool  CSV_rotateIfNeeded(); // sempre retorna true
```

**Comportamentos implementados:**

* `CSV_begin()`

  * Inicializa o SD (CS=5), garante `/MBI`, `/MBI/CSV` e a pasta do **ano** atual;
  * Sem RTC válido → usa pasta `/MBI/CSV/UNSET`.
* `openDailyIfNeeded()`

  * Constrói o caminho do arquivo do dia (`/MBI/CSV/<ANO>/MBI_<YYYYMMDD>.csv` ou `/MBI/CSV/UNSET/MBI_UNSET.csv`);
  * Abre em `FILE_APPEND`; se o arquivo estiver **vazio**, grava o **cabeçalho**;
  * Mantém estado interno: arquivo aberto, data corrente e caminho atual.
* `CSV_appendRow(...)`

  * Monta a linha `DataHora,UserID,SessionID,Sensor,Valor,Unidade,DeviceID,FwVersion,TZ,Seq`;
  * Usa *fallback* de timestamp (`2000-01-01 00:00:00`) se RTC inválido;
  * Concatena valores padrão `"-"` para parâmetros nulos;
  * `flush()` imediato após cada linha (MVP simples e seguro).
* `CSV_close()`

  * `flush()` e fecha o arquivo, limpando o estado interno.

**Caminhos de arquivo/pasta:**

* RTC **válido** → `/MBI/CSV/<ANO>/MBI_<YYYYMMDD>.csv`
* RTC **inválido/ausente** → `/MBI/CSV/UNSET/MBI_UNSET.csv`

---

## CSV — especificação funcional

### Layout de pastas

* Arquivos **por dia** em: `MBI/CSV/<ANO>/MBI_<YYYYMMDD>.csv`
* Se a data **não** puder ser obtida (RTC inválido/ausente): `MBI/CSV/UNSET/MBI_UNSET.csv`

### Colunas e formatos

1. **DataHora** — `YYYY-MM-DD HH:MM:SS`
2. **UserID** — string sanitizada (sem vírgula/quebra de linha)
3. **SessionID** — `YYYYMMDD-XX`
4. **Sensor** — `TEMP | BPM | SpO2 | GSR | ECG_BPM`
5. **Valor** — número decimal (`.`)
6. **Unidade** — `C | bpm | % | kOhm | mV | ...`
7. **DeviceID** — ex.: `MBI-ESP32-01`
8. **FwVersion** — ex.: `v0.1.0`
9. **TZ** — ex.: `-03:00`
10. **Seq** — contador incremental por arquivo

### Cabeçalho (estado atual)

* O módulo **escreve o cabeçalho somente quando cria um arquivo novo** (tamanho `0`).
* **Não há verificação de cabeçalho** em arquivos existentes (não implementado por design neste estágio).

### Limites e política de flush

* **Tamanho por arquivo:** limite lógico de **50 MB** (sem rotação automática por ora).
* **Flush:** MVP faz `flush()` **a cada `append`** (simples e seguro).

---

## Ordem sugerida de uso (nível de biblioteca)

1. `lvgl_display_init()`
2. `ui_init()` *(fornecido em `src/UI/`)*
3. `rtc_init()`
4. `CSV_begin()`
5. `openDailyIfNeeded()`
6. `CSV_appendRow(...)` conforme aquisição de sensores
7. `CSV_close()` ao finalizar uma sessão/desligamento controlado

> O agendamento/ciclo dessas chamadas é definido pela aplicação que consome a biblioteca.

---

## Logs/diagnóstico esperados (Serial)

* `Cartão SD lido com sucesso` — SD inicializado (CS=5)
* `Criando pasta 'MBI'` / `ERRO: mkdir /MBI`
* `Criando pasta 'CSV' em '/MBI/'` / `ERRO: mkdir /MBI/CSV`
* `Criando pasta do ano (ou UNSET): /MBI/CSV/2025` / `ERRO: mkdir ...`
* `[SD] ERRO: não foi possível abrir /MBI/CSV/2025/MBI_YYYYMMDD.csv`

Esses logs auxiliam a camada superior a informar o usuário/registrar telemetria.

---

## Convenções internas

* **Estado estático** encapsulado em **namespace anônimo** (escopo do `.cpp`).
* **Constantes** no padrão `kCamelCase` (ex.: `kHeader`, `kFwVersion`, `kTzStr`).
* **Buffers fixos** (`snprintf`) para evitar alocações dinâmicas no caminho crítico de I/O.
* **`nullptr`** para parâmetros ponteiro opcionais; arrays (`char[ ]`) são inicializados com `{0}`.
* Em `LVGL_Display.cpp`, o objeto `TFT_eSPI tft` e as *callbacks* de flush/input são **locais ao TU**, mantendo encapsulamento do driver.

---

## Coisas para fazer

**Curto prazo:**

* **Sanitização de campos**: remover vírgulas e CR/LF de `UserID`/`SessionID`; limitar tamanho (ex.: 32 chars).
* **Sinalização para UI**: quando `sd_ok == false`, desabilitar ações de gravação e exibir aviso.
* **Ajuste do draw buffer**: alinhar `size_in_px_cnt` ao tamanho real de `buf` (`sizeof(buf)/sizeof(buf[0])`) para melhor eficiência.

**Médio prazo:**

* **Flush temporizado** (opcional): substituir `flush()` por append por uma política de 2 s baseada em timer.
* **Integração de sensores**: MAX30102 (BPM/SpO₂), MLX90614 (TEMP), GSR (kΩ), ECG_BPM (mV + BPM derivado).
* **SessionID**: geração consistente ao entrar no Modo Registro (`YYYYMMDD-##`).
* **Concorrência**: se houver tarefas paralelas (FreeRTOS), proteger o SD com **mutex** e concentrar escrita numa **task** de logger.

**Depois (se necessário):**

* **Verificação de cabeçalho** em arquivos existentes (renomear `*_nohdr.csv` quando divergente)
* **Rotação por tamanho** (50 MB) com sufixos incrementais e `manifest` opcional.
* **Card-detect** (se disponível) para *hotplug* mais amigável e feedback imediato.
* **Versão de cabeçalho** e rotina de migração entre versões.

---

## Compatibilidade e suporte

* **ESP32 core:** 2.0.14
* **LVGL:** 8.3.11
* **Armazenamento:** microSD em **FAT32**
* **CS do SD:** **GPIO 5**

```
::contentReference[oaicite:0]{index=0}
```
