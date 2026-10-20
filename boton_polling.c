#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

// Compilar: gcc boton_polling.c -o boton_polling -lgpiod

#define CHIP_PATH "/dev/gpiochip4"
#define BOTON_PIN 25

int main() {
    unsigned int offset = BOTON_PIN;
    struct gpiod_chip *chip;
    struct gpiod_line_settings *settings;
    struct gpiod_line_config *line_cfg;
    struct gpiod_request_config *req_cfg;
    struct gpiod_line_request *request;

    chip = gpiod_chip_open(CHIP_PATH);
    if (!chip) return 1;

    // Configurar como entrada
    settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);

    line_cfg = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);

    req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "boton_v2");

    request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    if (!request) {
        perror("Error solicitando linea");
        return 1;
    }

    printf("Iniciando Polling en GPIO %d (v2). Presiona CTRL+C para salir.\n", BOTON_PIN);

    int estado_anterior = GPIOD_LINE_VALUE_INACTIVE;
    while (1) {
        int estado_actual = gpiod_line_request_get_value(request, offset);
        
        if (estado_actual == GPIOD_LINE_VALUE_ACTIVE && estado_anterior == GPIOD_LINE_VALUE_INACTIVE) {
            printf("¡Botón presionado!\n");
        }
        
        estado_anterior = estado_actual;
        usleep(10000); 
    }

    // Limpieza (no alcanzable normalmente por el while infinito, pero buena práctica)
    gpiod_line_request_release(request);
    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip);
    
    return 0;
}
