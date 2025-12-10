#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>

// Obtém a especificação do dispositivo para os sensores de "Lead Off" positivo (LO+) e negativo (LO-)
static const struct gpio_dt_spec lo_pos = GPIO_DT_SPEC_GET(DT_NODELABEL(lo_p), gpios);
static const struct gpio_dt_spec lo_neg = GPIO_DT_SPEC_GET(DT_NODELABEL(lo_n), gpios);

// Obtém o ponteiro para a estrutura do dispositivo ADC
#define ADC_NODE DT_NODELABEL(ads1115_dev)
static const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);

// Define a fila de mensagens para comunicação entre threads (máximo 20 itens, alinhamento de 4 bytes)
K_MSGQ_DEFINE(adc_data_q, sizeof(int16_t), 20, 4);

// Define o semáforo para sincronização entre a thread de processamento e a de interface
K_SEM_DEFINE(sem_novo_batimento, 0, 1);

// Define o Mutex
K_MUTEX_DEFINE(bpm_mutex);

// Variável global para armazenar o valor de batimentos por minuto (BPM)
volatile int bpm_global = 0;

// Esta thread é responsável por ler os dados do sensor (ADC) e colocá-los na fila
void thread_aquisicao(void) {
    if (!device_is_ready(adc_dev)) return;

    // Configura os pinos dos sensores de "Lead Off" (LO+ e LO-) como entrada digital
    gpio_pin_configure_dt(&lo_pos, GPIO_INPUT);
    gpio_pin_configure_dt(&lo_neg, GPIO_INPUT);

    // Estrutura para configurar o canal do ADC.
    struct adc_channel_cfg channel_cfg = {
        .gain = ADC_GAIN_1,
        .reference = ADC_REF_INTERNAL,
        .acquisition_time = ADC_ACQ_TIME_DEFAULT,
        .channel_id = 0 // Pino A0 do ADS1115
    };
    // Aplica a configuração de canal ao dispositivo ADC
    adc_channel_setup(adc_dev, &channel_cfg);

    // Buffer para armazenar a leitura do ADC
    int16_t buffer;
    // Estrutura que define a sequência de leitura do ADC.
    struct adc_sequence sequence = {
        .channels = BIT(0),
        .buffer = &buffer,
        .buffer_size = sizeof(buffer),
        .resolution = 15 // ADS1115 usa 15 bits (apenas valores positivos)
    };

    while (1) {
        if (gpio_pin_get_dt(&lo_pos) || gpio_pin_get_dt(&lo_neg)) {
            int16_t erro = 0;
            // Coloca o valor de erro na fila
            k_msgq_put(&adc_data_q, &erro, K_NO_WAIT);
            k_msleep(100); continue;
        }

        // Lê o valor do ADC
        if (adc_read(adc_dev, &sequence) == 0) {
            // Envia o dado bruto para a fila de processamento
            k_msgq_put(&adc_data_q, &buffer, K_NO_WAIT);
        }

        k_msleep(5);
    }
}

// Esta thread pega os dados da fila, processa-os para encontrar picos e calcular o BPM
void thread_processamento(void) {
    int16_t leitura; // Variável para armazenar o dado lido da fila
    int threshold = 28000; // Limiar para detecção de pico (onda R do ECG)
    int64_t last_time = 0; // Armazena o tempo (em ms) do último pico detectado

    while (1) {
        // Bloqueia a thread até um novo dado chegar na fila
        k_msgq_get(&adc_data_q, &leitura, K_FOREVER);

        if (leitura == 0) continue;

        // Lógica de detecção de pico
        if (leitura > threshold) {
            // Obtém o tempo atual do sistema em milissegundos
            int64_t now = k_uptime_get();

            // Se este não for o primeiro pico detectado
            if (last_time != 0) {
                // Calcula o intervalo de tempo (delta) entre o pico atual e o anterior
                int64_t delta = now - last_time;
                // Filtro para ignorar ruídos ou picos muito próximos
                if (delta > 400) {

                    // Calcula o novo BPM
                    int novo_bpm = 60000 / delta;

                    // Bloqueia o mutex
                    k_mutex_lock(&bpm_mutex, K_FOREVER);
                    // Atualiza a variável global com o novo valor de BPM
                    bpm_global = novo_bpm;
                    // Libera o mutex
                    k_mutex_unlock(&bpm_mutex);

                    // Sinaliza para a thread de interface que há um novo dado de BPM para mostrar
                    k_sem_give(&sem_novo_batimento);

                    // Atualiza o tempo do último pico para o tempo do pico atual
                    last_time = now;
                }
            } else {
                // Se for o primeiro pico, apenas armazena o tempo
                last_time = now;
            }
            // Ignora os próximos 200ms para não contar o mesmo batimento várias vezes
            k_msleep(200);
        }
    }
}

// Esta thread é responsável por mostrar os resultados ao usuário
void thread_interface(void) {
    int bpm_local; // Variável local para armazenar o BPM a ser exibido

    printk("Sistema Iniciado. Aguardando batimentos...\n");

    while (1) {
        // Ela só continuará a execução quando k_sem_give(&sem_novo_batimento) for chamado
        k_sem_take(&sem_novo_batimento, K_FOREVER);

        // Bloqueia o mutex
        k_mutex_lock(&bpm_mutex, K_FOREVER);
        // Copia o valor global para uma variável local
        bpm_local = bpm_global;
        // Libera o mutex
        k_mutex_unlock(&bpm_mutex);

        // Imprime o resultado no console
        printk("BATIMENTO DETECTADO! Frequencia: %d BPM\n", bpm_local);
    }
}

// Cria e inicia as threads do sistema com suas respectivas stack sizes e prioridades
K_THREAD_DEFINE(t_aq, 1024, thread_aquisicao, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(t_pr, 1024, thread_processamento, NULL, NULL, NULL, 6, 0, 0);
K_THREAD_DEFINE(t_in, 1024, thread_interface, NULL, NULL, NULL, 7, 0, 0);
