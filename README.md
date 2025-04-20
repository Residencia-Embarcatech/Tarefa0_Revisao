# Simulador de Estação Meteorológica

## Objetivo Geral

O objetivo do projeto é simular uma estação meteorológica simples, onde são feitas previsões do tempo com base em um sensor de temperatura e um sensor de chuva.

---

## Descrição Funcional

O sistema realiza a leitura dos sensores de temperatura e chuva — simulados pelos eixos **X** e **Y** do **Joystick**, respectivamente. Com base nesses valores, são calculadas previsões de clima que são exibidas:

- **Via monitor serial:** Apresenta os valores de temperatura e o nível de chuva.
- **Na matriz de LEDs:** Mostra ícones que representam a sensação térmica:
  - **C** → Sensação de Calor.
  - **F** → Sensação de Frio.
  - **N** → Sensação térmica Normal.

O usuário pode solicitar uma previsão do dia seguinte pressionando o **Botão A**. Nessa ação, os valores são randomizados usando `rand()`, e os cálculos de previsão são baseados nos valores atuais.

Em caso de **previsão de tempestade**, o sistema emite alertas:
- Aciona o **LED vermelho**.
- Emite som com o **Buzzer**.
- Os alertas permanecem ativos até que a previsão de tempestade deixe de existir.

---

## Uso dos Periféricos da BitDogLab

### Joystick
Simula dois sensores:
- **Eixo X:** Sensor de Temperatura.
- **Eixo Y:** Sensor de Chuva.

A leitura é feita através do conversor analógico-digital (**ADC**) e os valores são normalizados para simular as medições reais.

---

### Botão A
Ao ser pressionado:
- Solicita e calcula a **previsão climática do dia seguinte**.
- Exibe as informações no monitor serial.

---

### Display OLED
Exibe um quadrado que se movimenta conforme a posição atual do joystick, simulando uma interface de navegação simples.

---

### Matriz de LEDs
Responsável por exibir ícones que indicam a **sensação térmica atual**:
- Calor, Frio ou Normal.

---

### LED RGB e Buzzer
Quando há previsão de **tempestade**:
- O **LED vermelho** acende.
- O **Buzzer** emite um alerta sonoro.
- Ambos só são desligados quando não há mais previsão de tempestade.

---

### Interrupção
Utilizada para capturar o acionamento do **Botão A** usando:

```c
gpio_set_irq_enabled_with_callback()
```

## Compilação e Execução

1. **Pré-requisitos**:
   - Ter o ambiente de desenvolvimento para o Raspberry Pi Pico configurado (compilador, SDK, etc.).
   - CMake instalado.

2. **Compilação**:
   - Clone o repositório ou baixe os arquivos do projeto.
   - Navegue até a pasta do projeto e crie uma pasta de build:
     ```bash
     mkdir build
     cd build
     ```
   - Execute o CMake para configurar o projeto:
     ```bash
     cmake ..
     ```
   - Compile o projeto:
     ```bash
     make
     ```

3. **Upload para a placa**:
   - Conecte o Raspberry Pi Pico ao computador.
   - Copie o arquivo `.uf2` gerado para a placa.

## Simulação no Wokwi
Para visualizar a simulação do projeto no Wokwi:
1. Instale e configure o simulador Wokwi seguindo as instruções encontradas no link a seguir:  
   [Introdução ao Wokwi para VS Code](https://docs.wokwi.com/pt-BR/vscode/getting-started).
2. Abra o arquivo `diagram.json` no VS Code.
3. Clique em "Start Simulation".
