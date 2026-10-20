#include "stdio.h"
#include "unistd.h"
#include "fcntl.h"
#include "sys/ioctl.h"
#include "linux/i2c-dev.h"
#include "linux/spi/spidev.h"
#include "gpiod.h" // Libreria v2

#define I2C_ADDR 0x76
#define LED_PIN 17

int main(void) {
    // ==========================================
    // 1. CONFIGURACIÓN GPIO (libgpiod v2)
    // ==========================================
    // En las versiones nuevas del SO para Pi 4, los pines estándar están en gpiochip4
    struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip4"); 
    if (!chip) { perror("Fallo al abrir gpiochip4"); return 1; }

    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);

    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    unsigned int offset = LED_PIN;
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);

    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "sensor_combinado");

    struct gpiod_line_request *led_req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    if (!led_req) { perror("Fallo al solicitar el pin LED"); return 1; }

    // ==========================================
    // 2. CONFIGURACIÓN I2C (BME280)
    // ==========================================
    int fd_i2c = open("/dev/i2c-1", O_RDWR);
    if (fd_i2c < 0 || ioctl(fd_i2c, I2C_SLAVE, I2C_ADDR) < 0) {
        perror("Error I2C"); return 1;
    }
    
    unsigned char config[2] = {0xF4, 0x27};
    write(fd_i2c, config, 2);

    // ==========================================
    // 3. CONFIGURACIÓN SPI (MCP3008)
    // ==========================================
    int fd_spi = open("/dev/spidev0.0", O_RDWR);
    unsigned char mode = SPI_MODE_0, bits = 8;
    unsigned int speed = 1000000;
    ioctl(fd_spi, SPI_IOC_WR_MODE, &mode);
    ioctl(fd_spi, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(fd_spi, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    // ==========================================
    // 4. BUCLE PRINCIPAL (Cada 2 segundos)
    // ==========================================
    printf("Iniciando lectura combinada...\n");
    while (1) {
        // --- LEER POTENCIÓMETRO (SPI) ---
        unsigned char tx[3] = {1, (8 + 0) << 4, 0};
        unsigned char rx[3] = {0};
        struct spi_ioc_transfer tr = { .tx_buf = (unsigned long)tx, .rx_buf = (unsigned long)rx, .len = 3, .speed_hz = speed, .bits_per_word = bits };
        ioctl(fd_spi, SPI_IOC_MESSAGE(1), &tr);
        
        int valor_pot = ((rx[1] & 3) << 8) + rx[2];
        float umbral_temp = (valor_pot * 50.0f) / 1023.0f;

        // --- LEER TEMPERATURA (I2C) ---
        unsigned char reg_temp = 0xFA; 
        write(fd_i2c, &reg_temp, 1);
        unsigned char datos_temp[3];
        read(fd_i2c, datos_temp, 3);
        
        int raw_temp = (datos_temp[0] << 12) | (datos_temp[1] << 4) | (datos_temp[2] >> 4);
        float temp_actual = raw_temp / 22000.0f; // Ajuste calibrado para tu sensor
        // --- COMPARACIÓN Y CONTROL DE LED ---
        printf("Temp Ambiental: %.2f C | Umbral Potenciómetro: %.2f C\n", temp_actual, umbral_temp);
        
        if (temp_actual > umbral_temp) {
            gpiod_line_request_set_value(led_req, LED_PIN, GPIOD_LINE_VALUE_ACTIVE);
            printf(" -> ESTADO: LED ENCENDIDO (Temperatura alta)\n");
        } else {
            gpiod_line_request_set_value(led_req, LED_PIN, GPIOD_LINE_VALUE_INACTIVE);
            printf(" -> ESTADO: LED APAGADO\n");
        }

        usleep(2000000); 
    }

    close(fd_i2c);
    close(fd_spi);
    return 0;
}
