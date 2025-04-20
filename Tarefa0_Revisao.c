#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "pio_matrix.pio.h"

#define BUTTON_A 5
#define BUTTON_B 6
#define JOYSTICK_X 26
#define JOYSTICK_Y 27
#define JOYSTICK_B 22

#define RED_LED 13
#define BUZZER 10

#define I2C_PORT i2c1
#define I2C_SDA 14  
#define I2C_SCL 15 
#define address 0x3C 

#define UART_ID uart0
#define BAUD_RATE 115200

#define DEBOUNCE_TIME_MS 500 
#define MATRIX 7
#define MAX_CHAR 15
#define NUM_PIXELS 25

PIO pio = pio0;
uint sm;
ssd1306_t ssd;
uint32_t current_time;
uint32_t last_debounce_time = 0;
uint32_t led_value;
uint index_frames = 0;
uint slice_num;
float temp_c = 0.0;
float rain_level = 0.0; 
bool storm_danger = false;
uint8_t display_x; 
uint8_t display_y;

const uint32_t pixels_order[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24};

const int frames[3][NUM_PIXELS] = {
    {
        0, 1, 1, 1, 0,
        0, 1, 0, 0, 0,
        0, 1, 0, 0, 0,
        0, 1, 0, 0, 0,
        0, 1, 1, 1, 0,
    }, //Calor
    {
        1, 0, 0, 0, 1,
        1, 1, 0, 0, 1,
        1, 0, 1, 0, 1,
        1, 0, 0, 1, 1,
        1, 0, 0, 0, 1,
    },//Normal
    {
        0, 1, 1, 1, 0,
        0, 1, 0, 0, 0,
        0, 1, 1, 1, 0,
        0, 1, 0, 0, 0,
        0, 1, 0, 0, 0,
    }//Frio
};

/**
 * @brief Inicializa o ADC e os pinos do EIXO X e Y do Joystick
 */
void init_adc()
{
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);
}

/**
 * @brief Inicializa o botão A
 * 
 * @param gpio Pino a ser configurado
 */
void init_button(uint gpio)
{
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
}

/**
 * @brief Inicializa o I2C e o Display 
 */
void init_i2c_display()
{
    //Configuração do I2C
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    //Configuração do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, address, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display
    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

/**
 * @brief Utiliza PWM para iniciar a saída do buzzer
 */
void start_buzzer(){
    gpio_set_function(BUZZER, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(BUZZER);
    pwm_set_clkdiv(slice_num, 62.5f);
    pwm_set_wrap(slice_num, 1000);
    pwm_set_gpio_level(BUZZER, 500);
    pwm_set_enabled(slice_num, true);
}

/**
 * @brief Desliga o buzzer desabilitando o PWM
 */
void stop_buzzer(){
    pwm_set_enabled(slice_num, false);
    gpio_put(BUZZER, false);
}

/**
 * @brief Envia a previsão do tempo para o monitor serial via UART
 * 
 * @param rain Previsão de percentagem de chuva
 * @param temp Previsão de temperatura
 * @param storm Indicador de possibilidade ou não de tempestade
 */
void weather_analysis_uart(float rain, float temp, bool storm){
    printf("Chuva: %.2f%%\n", rain);
    printf("Temp: %.2fºC\n", temp);
    
    if (storm){
        printf("Alerta de Possível Tempestade!\n");
    }
    if (rain > 0.0) {
        printf("Pancadas de chuva isoladas.\n");
    } else {
        printf("Dia ensolarado!\n");
    }

    printf("\n\n");
}

/**
 * @brief Função acionada pelo callback quando há acionamento do Botão A
 * 
 * Ao ser acionada, realiza a previsão do tempo do dia seguinte. Para deixar a previsão randomizada, 
 * é feita a utilização da função rand()
 * Quando a previsão é calculada, a função que mostra os dados no monitor serial é chamada.
 */
static void gpio_irq_handler(uint gpio, uint32_t events)
{
    current_time = to_ms_since_boot(get_absolute_time());

    if ((current_time - last_debounce_time) > DEBOUNCE_TIME_MS)
    {
        float new_temp = temp_c + ((rand() % 500) / 100.0) - 2.5; //Varia entre -2.5 a 2.5 graus
        float new_rain_level = rain_level;
        bool storm = false;

        if (rain_level > 50.0)
        {
            new_rain_level += ((rand() % 20) - 10 ); // Varia em mais ou menos 10%
            if (new_temp > 20.0) storm = true;
        }else if (rain_level > 0.0){
            new_rain_level += ((rand() % 30) - 15); // Varia em mais ou menos 15%
        }else {
            new_rain_level = (rand() % 30);  
        }

        if (new_rain_level < 0.0) new_rain_level = 0.0;
        if (new_rain_level > 100.0) new_rain_level = 100.0;

        printf("Previsão Para amanhã:\n");
        weather_analysis_uart(new_rain_level, new_temp, storm);
    }
}

/**
 * @brief Verifica se há previsão de tempestade
 */
void verify_storm_danger()
{
    if (rain_level >= 70.0 && temp_c >= 25.0) storm_danger = true;
    else storm_danger = 0;
}

/**
 * @brief Calcula o valor da temperatura atual e percentual de chuva
 * 
 * @param x Valor lido pelo eixo X do Joystick
 * @param y Valor lido pelo eixo Y do Joystick
 */
void verify_weather(uint32_t x, uint32_t y)
{
    /**
     * Mapeia os valores para o sensor de temperatura, variando entre o minimo (10 graus) e Máximo (40 graus)
     */
    if (x >= 2048) {
        temp_c = 25.0 + ((x - 2048) * (15.0 / 2047)); //Mapeia os valores até 40 graus
    }else {
        temp_c = 25.0 - ((2048 - x) * (15.0 / 2048)); //Mapeia os valores até 10 graus
    }

    if (y > 2200) {
        rain_level = ((y - 2200) * (100.0 / (4095 - 2200))); // Mapeia valores quando há chuva forte
    }else if (y < 1895){
        rain_level = ((1895 - y) * (50.0 / 1895)); // Mapeia valores quando há chuva fraca
    }

    verify_storm_danger();
}

uint32_t matrix_rgb(double r, double g, double b)
{
    unsigned char R = (unsigned char)(r * 255);
    unsigned char G = (unsigned char)(g * 255);
    unsigned char B = (unsigned char)(b * 255);

    return (G << 24) | (R << 16) | (B << 8);
} 

/**
 * @brief Desenha na matriz de LEDS
 * 
 * @param R valor de intensidade da cor Vermelha
 * @param G valor de intensidade da cor Verde
 * @param B valor de intensidade da cor Azul
 */
void draw_on_matrix(double R, double G, double B)
{
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (frames[index_frames][pixels_order[i]] == 1.0) {
            led_value = matrix_rgb(R, G, B); 
        } else {
            led_value = matrix_rgb(0, 0, 0); 
        }
        pio_sm_put_blocking(pio, sm, led_value);
    }
}

/**
 * @brief Apaga todos os leds da Matriz de Leds 
 */
void restart_matrix()
{
    for (int i = 0; i < NUM_PIXELS; i++){
        led_value = matrix_rgb(0.0, 0.0, 0.0);
        pio_sm_put_blocking(pio, sm, led_value);
    }
}

/**
 * @brief Aciona/desliga as saídas (LED Vermelho e Buzzer) que indicam previsão de tempestade 
 */
void storm_alert(bool value){
    gpio_put(RED_LED, value);
    (value) ? start_buzzer() : stop_buzzer();
}

/**
 * @brief Define qual ícone deve ser desenhado na matriz de LEDs conforme previsão
 * Também é responsável por verificar se os indicadores de tempestade devem ser acionados ou desligados
 */
void weather_analysis_display(){
    
    if (temp_c > 25.0) {
        index_frames = 0;
        draw_on_matrix(1.0, 0.0, 0.0);
    }else if (temp_c < 20.0) {
        index_frames = 2;
        draw_on_matrix(0.0, 0.0, 1.0);
    }else {
        index_frames = 1;
        draw_on_matrix(0.0, 1.0, 0.0);
    }

    storm_danger ? storm_alert(true) : storm_alert(false); 
} 

/**
 * @brief Atualiza o quadrado no display, conforme a posição do joystick
 */
void update_position_on_display(uint32_t x, uint32_t y)
{
    static int last_x = 128 / 2;
    static int last_y = 64 / 2;

    display_x = x * (128-8) / 4095; //Calcula o novo valor da posição x
    display_y = y * (64-8) / 4095; //Calcula o novo valor da posição y

    ssd1306_fill(&ssd, false); //Limpa a tela anterior
    ssd1306_rect(&ssd, 3, 3, 122, 58, true, false); //Desenha a borda 
    ssd1306_draw_quad(&ssd, display_x, display_y); //Desenha o quadrado na posição atual
    ssd1306_send_data(&ssd); //Envia os dados para o display

    last_x = display_x;
    last_y = display_y;
}

int main()
{
    stdio_init_all();
    init_adc();
    init_button(BUTTON_A);
    init_button(BUTTON_B);
    init_i2c_display();
    uart_init(UART_ID, BAUD_RATE);

    /** Inicializa o LED vermelho */
    gpio_init(RED_LED);
    gpio_set_dir(RED_LED, GPIO_OUT);

    /**
     * Configuração do PIO para utilizar a matriz de LEDS
     */ 
    uint offset = pio_add_program(pio, &pio_matrix_program);
    sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, MATRIX);

    restart_matrix();

    /**
     * Funções de Callback para tratamento de acionamento dos botões A e B
     */
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    uint32_t adc_x_value = 0;
    uint32_t adc_y_value = 0;


    while (true) {
       /**
        * Faz a leitura dos sensores a cada 1 segundo para que os cálculos sejam efetuados
        *  */    
       adc_select_input(0);
       adc_x_value = adc_read();

       adc_select_input(1);
       adc_y_value = adc_read();
       
       /** Normaliza os valores lidos pelos sensores(joystick) para realizar a previsão */
       verify_weather(adc_x_value, adc_y_value);
       /** Atualiza o ícone na matriz de LEDS */
       weather_analysis_display();
       /** Atualiza a previsão no monitor serial */
       weather_analysis_uart(rain_level, temp_c, storm_danger);
       
       /** Atualiza a posição do quadrado no display */
       update_position_on_display(adc_y_value, adc_x_value);

       sleep_ms(1000);
    }
}
