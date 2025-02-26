# 🕒 Relógio com Alarme e Display OLED 🚨

Este projeto é um **relógio digital** com funcionalidade de **alarme**, utilizando um display OLED, botões para configuração, um joystick para ajustes e um buzzer para o alarme. Desenvolvido para a plataforma **Raspberry Pi Pico**, o código foi escrito em C utilizando o **SDK da Raspberry Pi**. O projeto é ideal para quem quer aprender sobre microcontroladores, interfaces de usuário e manipulação de periféricos como displays, botões e ADCs.

---

## 🎯 Funcionalidades

- **🕒 Relógio Digital**: Exibe a hora, minutos, segundos, dia, mês e ano no display OLED.
- **⚙️ Configuração de Horário**: Permite ajustar a hora, minutos, dia, mês e ano usando botões e um joystick.
- **⏰ Alarme**: Configura um alarme para um horário específico, com notificação visual (LED vermelho) e sonora (buzzer).
- **🔧 Modo de Configuração**: Entra em um modo de configuração onde é possível ajustar o horário e configurar o alarme.
- **🔔 Notificação de Alarme**: Quando o alarme é disparado, o LED vermelho pisca e o buzzer emite um som intermitente.
- **🎮 Controle por Joystick**: O eixo Y do joystick é usado para incrementar ou decrementar os valores no modo de configuração.

---

## 🛠️ Componentes Utilizados

- **Raspberry Pi Pico** 🍓: Microcontrolador principal.
- **Display OLED SSD1306** 🖥️: Display de 128x64 pixels para exibição do horário e menu de configuração.
- **Botões** 🔘: Três botões para interação com o usuário:
  - **Botão A**: Configura o alarme.
  - **Botão B**: Alterna entre os campos (hora, minuto, dia, mês, ano).
  - **Botão SELECT**: Entra/sai do modo de configuração.
- **Joystick** 🕹️: Utilizado para ajustar os valores dos campos no modo de configuração.
- **LED Vermelho** 🔴: Indicador visual para o alarme.
- **Buzzer** 🔊: Emite o som do alarme.

---

## 📂 Estrutura do Código

O código está organizado em funções principais que controlam diferentes aspectos do projeto:

### 1. **Inicialização**
   - Configuração dos pinos GPIO, I2C para o display OLED, ADC para o joystick e interrupções para os botões.
   - Funções:
     - `init_adc()`: Inicializa o ADC para ler o eixo Y do joystick.
     - `init_buttons()`: Configura os botões com interrupções.
     - `gpio_callback()`: Função de callback para tratar os eventos dos botões.

### 2. **Modo de Configuração**
   - Permite ao usuário ajustar o horário e configurar o alarme.
   - Funções:
     - `increment_field()`: Incrementa o valor do campo selecionado (hora, minuto, dia, mês, ano).
     - `decrement_field()`: Decrementa o valor do campo selecionado.
     - `toggle_field()`: Alterna entre os campos no modo de configuração.

### 3. **Atualização do Display**
   - Atualiza o display OLED com o horário atual e o menu de configuração.
   - Função:
     - `update_display()`: Formata e exibe a hora, data e menu de configuração no display.

### 4. **Controle do Alarme**
   - Verifica se o horário atual coincide com o horário do alarme e dispara o alarme, se necessário.
   - Funções:
     - `set_alarm()`: Configura o alarme com o horário atual.
     - `disable_alarm()`: Desativa o alarme e desliga o LED e o buzzer.
     - `trigger_alarm()`: Ativa o alarme, ligando o LED e o buzzer.

### 5. **Callback de Interrupção**
   - Trata os eventos de pressionamento dos botões com debounce.
   - Função:
     - `gpio_callback()`: Gerencia os eventos dos botões e evita bouncing.

### 6. **Timer do Buzzer**
   - Controla o som intermitente do buzzer quando o alarme está ativo.
   - Função:
     - `buzzer_timer_callback()`: Alterna o estado do buzzer para criar o som intermitente.

### 7. **Incremento do Tempo**
   - Atualiza o horário a cada segundo.
   - Função:
     - `increment_time()`: Incrementa os segundos, minutos, horas, dias, etc., e verifica se o alarme deve ser disparado.

---

## 🚀 Como Usar

1. **Compilação**: Compile o código usando o SDK da Raspberry Pi Pico.
2. **Upload**: Carregue o código no Raspberry Pi Pico.
3. **Interação**:
   - **Botão SELECT**: Entra/sai do modo de configuração.
   - **Botão A**: No modo de configuração, configura o alarme. Fora do modo de configuração, desativa o alarme.
   - **Botão B**: Alterna entre os campos (hora, minuto, dia, mês, ano) no modo de configuração.
   - **Joystick**: Ajusta o valor do campo selecionado no modo de configuração.
4. **Alarme**: Quando o horário atual coincide com o horário do alarme, o LED vermelho pisca e o buzzer emite um som intermitente. Pressione o botão A para desativar o alarme.

---

## 🔌 Diagrama de Conexões

| Componente       | Pino Raspberry Pi Pico |
|------------------|------------------------|
| Display OLED SDA | GPIO 14                |
| Display OLED SCL | GPIO 15                |
| Botão A          | GPIO 5                 |
| Botão B          | GPIO 6                 |
| Botão SELECT     | GPIO 22                |
| Joystick (Eixo Y)| GPIO 26 (ADC0)         |
| LED Vermelho     | GPIO 13                |
| Buzzer           | GPIO 21                |

---

## 👨‍💻 Autor

[Ian Borges](https://github.com/IanBorgess)  
📧 **Email**: ianborgesdev@hotmail.com
🌐 **GitHub**: [IanBorgess](https://github.com/IanBorgess)