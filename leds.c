#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

// Compilar: gcc leds.c -o leds -lgpiod

#define CHIP_PATH "/dev/gpiochip0"
#define NUM_LEDS 4

int main() {
    unsigned int offsets[NUM_LEDS] = {17, 27, 22, 23};
    struct gpiod_chip *chip;
    struct gpiod_line_settings *settings;
    struct gpiod_line_config *line_cfg;
    struct gpiod_request_config *req_cfg;
    struct gpiod_line_request *request;

    // 1. Abrir el chip (en v2 se recomienda la ruta absoluta)
    chip = gpiod_chip_open(CHIP_PATH);
    if (!chip) {
        perror("Error abriendo el gpiochip");
        return 1;
    }

    // 2. Crear las configuraciones de salida
    settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);

    // 3. Aplicar configuracion a los pines
    line_cfg = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(line_cfg, offsets, NUM_LEDS, settings);

    // 4. Configurar la solicitud
    req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "test_leds_v2");

    // 5. Solicitar las líneas
    request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    if (!request) {
        perror("Error solicitando las lineas");
        return 1;
    }

    printf("Iniciando prueba de LEDs (v2)...\n");
    for (int ciclos = 0; ciclos < 5; ciclos++) {
        for (int i = 0; i < NUM_LEDS; i++) {
            gpiod_line_request_set_value(request, offsets[i], GPIOD_LINE_VALUE_ACTIVE);
            usleep(200000); 
            gpiod_line_request_set_value(request, offsets[i], GPIOD_LINE_VALUE_INACTIVE);
        }
    }

    // 6. Liberar memoria y cerrar recursos
    gpiod_line_request_release(request);
    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip);
    
    printf("Prueba terminada.\n");
    return 0;
}
