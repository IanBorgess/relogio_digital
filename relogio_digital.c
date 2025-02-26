#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "ssd1306.h"
#include "font.h"

// Definições para o display OLED
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define OLED_ADDRESS 0x3C

// Pinos dos botões
#define BUTTON_A 5    // Botão para configurar o alarme
#define BUTTON_B 6    // Botão para alternar entre horas e minutos
#define BUTTON_SELECT 22 // Botão para entrar/sair do modo de configuração

// Pino do eixo Y do joystick (conectado ao ADC)
#define JOYSTICK_Y_PIN 26

// Pino do Led vermelho
#define LED_PIN_RED 13

// Pino do Buzzer
#define BUZZER_PIN 21

datetime_t current_time = {2025, 1, 1, 1, 12, 0, 0}; // Horário inicial (ano padrão 2025)

// Variável global para armazenar o alarme
typedef struct {
    uint8_t hour;
    uint8_t min;
    bool enabled;
} alarm_t;

alarm_t alarm = {0, 0, false}; // Alarme desativado por padrão

// Inicializa o display OLED
ssd1306_t ssd;

// Variáveis para controle do menu
volatile uint8_t selected_field = 0; // 0 = hora, 1 = minuto, 2 = dia, 3 = mês, 4 = ano
volatile bool in_config_mode = false;

// Flags para indicar que um botão foi pressionado
volatile bool button_a_pressed = false;
volatile bool button_b_pressed = false;
volatile bool button_select_pressed = false;

// Variáveis para debounce
volatile absolute_time_t last_button_a_time = 0;
volatile absolute_time_t last_button_b_time = 0;
volatile absolute_time_t last_button_select_time = 0;
const uint32_t debounce_time_ms = 200; // Tempo de debounce em milissegundos

// Variável para controlar a exibição da notificação do alarme
volatile bool show_alarm_notification = false;
volatile absolute_time_t alarm_notification_time = 0;
const uint32_t alarm_notification_duration_ms = 3000; // Duração da notificação em milissegundos

// Variável para controlar o estado do buzzer
volatile bool buzzer_active = false;
volatile bool buzzer_state = false;
volatile absolute_time_t buzzer_toggle_time = 0;
const uint32_t buzzer_toggle_interval_ms = 125; // Intervalo para 4 apitos por segundo (1000 ms / 8)

void gpio_callback(uint gpio, uint32_t events);
bool buzzer_timer_callback(struct repeating_timer *t);
void increment_field();
void decrement_field();
void toggle_field();
void pwm_setup_buzzer(uint gpio_pin, uint32_t freq_hz, uint16_t wrap_value);
void set_alarm();
void disable_alarm();
void trigger_alarm();
void update_display();
void increment_time();
void setup();

int main() {
    stdio_init_all();

    //Função que configura os pinos, I2C, ADC e PWM para o buzzer
    setup();

    //Configuração das interrupções para os botões
    gpio_set_irq_callback(gpio_callback);
    irq_set_enabled(IO_IRQ_BANK0, true);
    

    // Configuração do timer para o buzzer
    struct repeating_timer buzzer_timer;
    add_repeating_timer_ms(100, buzzer_timer_callback, NULL, &buzzer_timer);

    while (true) {
        // Verifica se o botão SELECT foi pressionado para entrar/sair do modo de configuração
        if (button_select_pressed) {
            button_select_pressed = false;
            in_config_mode = !in_config_mode;
        }

        // Modo de configuração
        if (in_config_mode) {
            // Verifica se o botão A foi pressionado para configurar o alarme
            if (button_a_pressed) {
                button_a_pressed = false;
                set_alarm();
            }

            // Verifica se o botão B foi pressionado para alternar entre os campos
            if (button_b_pressed) {
                button_b_pressed = false;
                toggle_field();
            }

            // Leitura do eixo Y do joystick para ajustar o campo selecionado
            uint16_t joystick_y_value = adc_read();
            if (joystick_y_value < 1000) { // Joystick para a esquerda
                decrement_field();
                sleep_ms(200); // Debounce
            } else if (joystick_y_value > 3000) { // Joystick para a direita
                increment_field();
                sleep_ms(200); // Debounce
            }
        } else {
            // Fora do modo de configuração, verifica se o botão A foi pressionado para desativar o alarme
            if (button_a_pressed) {
                button_a_pressed = false;
                disable_alarm();
            }
        }

        // Atualiza o display
        update_display();

         // Incrementa o horário em 1 segundo (se não estiver no modo de configuração)
         if (!in_config_mode) {
            increment_time();
            sleep_ms(1000); // Aguarda 1 segundo
        }
        
    }
}
void setup(){
    adc_init();
    adc_gpio_init(JOYSTICK_Y_PIN); // Configura o pino do eixo Y como entrada analógica
    adc_select_input(0); // Seleciona o canal ADC 0 (GPIO 26)

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_set_irq_enabled(BUTTON_A, GPIO_IRQ_EDGE_FALL, true);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);

    gpio_init(BUTTON_SELECT);
    gpio_set_dir(BUTTON_SELECT, GPIO_IN);
    gpio_pull_up(BUTTON_SELECT);
    gpio_set_irq_enabled(BUTTON_SELECT, GPIO_IRQ_EDGE_FALL, true);

    // Inicializa o LED vermelho
    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);

    // Inicializa o Buzzer
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    // Inicializa o I2C para o display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display OLED
    ssd1306_init(&ssd, 128, 64, false, OLED_ADDRESS, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

// Função de callback para interrupções dos botões
void gpio_callback(uint gpio, uint32_t events) {
    absolute_time_t current_time = get_absolute_time();

    if (gpio == BUTTON_A) {
        if (absolute_time_diff_us(last_button_a_time, current_time) > debounce_time_ms * 1000) {
            button_a_pressed = true;
            last_button_a_time = current_time;
        }
    } else if (gpio == BUTTON_B) {
        if (absolute_time_diff_us(last_button_b_time, current_time) > debounce_time_ms * 1000) {
            button_b_pressed = true;
            last_button_b_time = current_time;
        }
    } else if (gpio == BUTTON_SELECT) {
        if (absolute_time_diff_us(last_button_select_time, current_time) > debounce_time_ms * 1000) {
            button_select_pressed = true;
            last_button_select_time = current_time;
        }
    }
}

// Função para incrementar o valor do campo selecionado
void increment_field() {
    switch (selected_field) {
        case 0: // Hora
            current_time.hour = (current_time.hour + 1) % 24;
            break;
        case 1: // Minuto
            current_time.min = (current_time.min + 1) % 60;
            break;
        case 2: // Dia
            current_time.day = (current_time.day % 31) + 1;
            break;
        case 3: // Mês
            current_time.month = (current_time.month % 12) + 1;
            break;
        case 4: // Ano
            current_time.year++;
            break;
    }
}

// Função para decrementar o valor do campo selecionado
void decrement_field() {
    switch (selected_field) {
        case 0: // Hora
            current_time.hour = (current_time.hour == 0) ? 23 : current_time.hour - 1;
            break;
        case 1: // Minuto
            current_time.min = (current_time.min == 0) ? 59 : current_time.min - 1;
            break;
        case 2: // Dia
            current_time.day = (current_time.day == 1) ? 31 : current_time.day - 1;
            break;
        case 3: // Mês
            current_time.month = (current_time.month == 1) ? 12 : current_time.month - 1;
            break;
        case 4: // Ano
            current_time.year = (current_time.year == 0) ? 0 : current_time.year - 1;
            break;
    }
}

// Função para alternar entre horas, minutos, dia, mês e ano
void toggle_field() {
    selected_field = (selected_field + 1) % 5; // Alterna entre 0 (hora), 1 (minuto), 2 (dia), 3 (mês), 4 (ano)
}

// Função para configurar o PWM no buzzer
void pwm_setup_buzzer(uint gpio_pin, uint32_t freq_hz, uint16_t wrap_value) {
    gpio_set_function(gpio_pin, GPIO_FUNC_PWM); // Configura o pino como PWM
    uint slice_num = pwm_gpio_to_slice_num(gpio_pin); // Obtém o slice PWM

    // Calcula o divisor de clock para alcançar a frequência desejada
    float clk_div = (float)clock_get_hz(clk_sys) / (freq_hz * wrap_value);

    // Configura o PWM
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clk_div);       // Define o divisor de clock
    pwm_config_set_wrap(&config, wrap_value - 1);  // Define o valor máximo do contador (wrap)
    pwm_init(slice_num, &config, true);            // Inicializa o PWM
}

// Função para configurar o alarme
void set_alarm() {
    alarm.hour = current_time.hour;
    alarm.min = current_time.min;
    alarm.enabled = true;
    show_alarm_notification = true;
    alarm_notification_time = get_absolute_time(); // Registra o tempo atual
    printf("Alarme configurado para %02d:%02d\n", alarm.hour, alarm.min);
}

void disable_alarm() {
    alarm.enabled = false;
    gpio_put(LED_PIN_RED, 0); // Desliga o LED vermelho

    // Desativa o PWM no buzzer
    pwm_set_gpio_level(BUZZER_PIN, 0); // Desliga o som
    buzzer_active = false;

    printf("Alarme desativado\n");
}

void trigger_alarm() {

    printf("ALARME! Horário atual: %02d:%02d\n", current_time.hour, current_time.min);
    gpio_put(LED_PIN_RED, 1); // Liga o LED vermelho

    // Configura o PWM para o buzzer
    const uint32_t BUZZER_FREQ = 2000; // Frequência do som do alarme (2 kHz)
    const uint16_t BUZZER_WRAP_VALUE = 4095; // Resolução de 12 bits
    pwm_setup_buzzer(BUZZER_PIN, BUZZER_FREQ, BUZZER_WRAP_VALUE);

    // Define o nível de saída do PWM para gerar o som
    pwm_set_gpio_level(BUZZER_PIN, BUZZER_WRAP_VALUE / 2); // Duty cycle de 50%
    buzzer_active = true;
}

// Função para atualizar o display com o horário atual e o menu de configuração
void update_display() {
    char time_str[12];
    char date_str[12];
    char menu_str[20];
    char alarm_str[20];

    // Formata a hora no formato HH:MM:SS
    sprintf(time_str, "%02d:%02d:%02d", current_time.hour, current_time.min, current_time.sec);

    // Formata a data no formato DD/MM/YYYY
    sprintf(date_str, "%02d/%02d/%04d", current_time.day, current_time.month, current_time.year);

    // Limpa o display
    ssd1306_fill(&ssd, false);

    // Exibe a hora e a data
    ssd1306_draw_string(&ssd, time_str, 20, 10);
    ssd1306_draw_string(&ssd, date_str, 15, 30);

    // Exibe o menu de configuração
    if (in_config_mode) {
        const char *fields[] = {"Hora", "Minuto", "Dia", "Mes", "Ano"};
        sprintf(menu_str, "Ajustar: %s", fields[selected_field]);
        ssd1306_draw_string(&ssd, menu_str, 10, 50);
    }

    // Exibe a notificação do alarme, se necessário
    if (show_alarm_notification) {
        sprintf(alarm_str, "Alarme: %02d:%02d", alarm.hour, alarm.min);
        ssd1306_draw_string(&ssd, alarm_str, 10, 50);

        // Verifica se a notificação deve ser desativada
        if (absolute_time_diff_us(alarm_notification_time, get_absolute_time()) > alarm_notification_duration_ms * 1000) {
            show_alarm_notification = false;
        }
    }

    // Atualiza o display
    ssd1306_send_data(&ssd);
}

// Função para incrementar o horário em 1 segundo
void increment_time() {
    current_time.sec++; // Incrementa os segundos

    // Verifica se os segundos atingiram 60
    if (current_time.sec >= 60) {
        current_time.sec = 0; // Zera os segundos
        current_time.min++;   // Incrementa os minutos

        // Verifica se os minutos atingiram 60
        if (current_time.min >= 60) {
            current_time.min = 0; // Zera os minutos
            current_time.hour++;  // Incrementa as horas

            // Verifica se as horas atingiram 24
            if (current_time.hour >= 24) {
                current_time.hour = 0; // Zera as horas
                current_time.day++;   // Incrementa o dia

                // Aqui você pode adicionar lógica para ajustar dias/meses/anos, se necessário
            }
        }
    }

    // Verifica se o alarme deve ser tocado
    if (alarm.enabled && current_time.hour == alarm.hour && current_time.min == alarm.min && current_time.sec == 0) {
        trigger_alarm();
    }
}

// Função de callback para o timer do buzzer
bool buzzer_timer_callback(struct repeating_timer *t) {
    const uint16_t BUZZER_WRAP_VALUE = 4095; // Resolução de 12 bits
    if (buzzer_active) {
        buzzer_state = !buzzer_state; // Alterna o estado do buzzer
        pwm_set_gpio_level(BUZZER_PIN, buzzer_state ? BUZZER_WRAP_VALUE / 2 : 0); // Liga/desliga o som
    } else {
        pwm_set_gpio_level(BUZZER_PIN, 0); // Desliga o buzzer
    }
    return true; // Mantém o timer ativo
}