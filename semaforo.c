#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

// Compilar: gcc semaforo.c -o semaforo -lgpiod

#define CHIP_PATH "/dev/gpiochip4"
#define COCHES_VERDE 17
#define COCHES_ROJO 27
#define PEATON_VERDE 22
#define PEATON_ROJO 23
#define BOTON 24

int main() {
    struct gpiod_chip *chip;
    struct gpiod_line_settings *settings_out, *settings_in;
    struct gpiod_line_config *line_cfg;
    struct gpiod_request_config *req_cfg;
    struct gpiod_line_request *request;
    
    unsigned int leds[4] = {COCHES_VERDE, COCHES_ROJO, PEATON_VERDE, PEATON_ROJO};
    unsigned int boton = BOTON;

    chip = gpiod_chip_open(CHIP_PATH);
    if (!chip) return 1;

    // Configurar settings de salida (LEDs)
    settings_out = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings_out, GPIOD_LINE_DIRECTION_OUTPUT);

    // Configurar settings de entrada (Botón)
    settings_in = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings_in, GPIOD_LINE_DIRECTION_INPUT);

    // Unir todo en el mismo config de lineas
    line_cfg = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(line_cfg, leds, 4, settings_out);
    gpiod_line_config_add_line_settings(line_cfg, &boton, 1, settings_in);

    req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "semaforo_v2");

    request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    if (!request) return 1;

    printf("Semáforo iniciado (v2). Coches avanzando...\n");

    while (1) {
        // Estado por defecto
        gpiod_line_request_set_value(request, COCHES_VERDE, GPIOD_LINE_VALUE_ACTIVE);
        gpiod_line_request_set_value(request, COCHES_ROJO, GPIOD_LINE_VALUE_INACTIVE);
        gpiod_line_request_set_value(request, PEATON_VERDE, GPIOD_LINE_VALUE_INACTIVE);
        gpiod_line_request_set_value(request, PEATON_ROJO, GPIOD_LINE_VALUE_ACTIVE);

        // Lectura del botón
        if (gpiod_line_request_get_value(request, BOTON) == GPIOD_LINE_VALUE_ACTIVE) {
            printf("Botón presionado. Iniciando cambio de semáforo...\n");
            
            // 1. Coches se detienen
            gpiod_line_request_set_value(request, COCHES_VERDE, GPIOD_LINE_VALUE_INACTIVE);
            gpiod_line_request_set_value(request, COCHES_ROJO, GPIOD_LINE_VALUE_ACTIVE);
            sleep(1);

            // 2. Peatones avanzan
            gpiod_line_request_set_value(request, PEATON_ROJO, GPIOD_LINE_VALUE_INACTIVE);
            gpiod_line_request_set_value(request, PEATON_VERDE, GPIOD_LINE_VALUE_ACTIVE);
            printf("Paso peatonal abierto.\n");
            sleep(5);

            // 3. Parpadeo preventivo peatones
            printf("Paso peatonal cerrando...\n");
            for (int i = 0; i < 6; i++) {
                gpiod_line_request_set_value(request, PEATON_VERDE, GPIOD_LINE_VALUE_INACTIVE);
                usleep(300000);
                gpiod_line_request_set_value(request, PEATON_VERDE, GPIOD_LINE_VALUE_ACTIVE);
                usleep(300000);
            }

            // 4. Peatones se detienen
            gpiod_line_request_set_value(request, PEATON_VERDE, GPIOD_LINE_VALUE_INACTIVE);
            gpiod_line_request_set_value(request, PEATON_ROJO, GPIOD_LINE_VALUE_ACTIVE);
            sleep(1);

            printf("Semáforo restablecido. Coches avanzando...\n");
        }
        usleep(50000);
    }

    return 0; 
}
