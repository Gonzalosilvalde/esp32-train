#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

#define UART_NUM UART_NUM_0
#define BUF_SIZE (1024)

void app_main() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    int index = 0;

    while (1) {
        // Leer datos del UART
        int len = uart_read_bytes(UART_NUM, data + index, 1, portMAX_DELAY);
        if (len > 0) {
            // Verificar si se ha recibido un carácter de nueva línea (Enter)
            if (data[index] == '\n' || data[index] == '\r') {
                data[index] = '\0';  // Terminar la cadena con NULL byte

                // Convertir los datos recibidos a un número entero
                int numero;
                sscanf((const char*) data, "%d", &numero);

                // Multiplicar por 10 en ensamblador
                int resultado;
                __asm__ __volatile__ (
                    "slli %0, %1, 3\n"   // %0 = %1 << 3 (num * 8)
                    "add %0, %0, %1\n"   // %0 = %0 + %1 (num * 8 + num = num * 9)
                    "add %0, %0, %1\n"   // %0 = %0 + %1 (num * 9 + num = num * 10)
                    : "=r" (resultado)     // Output: result es un registro de salida (%0)
                    : "r" (numero)         // Input: num es un registro de entrada (%1)
                );
                


                // Imprimir resultado por UART
                printf("\r\nEl número recibido es: %d\r\n", numero);
                printf("El resultado de multiplicar por 10 es: %d\r\n\r\n", resultado);

                // Reiniciar el índice para recibir el próximo número
                index = 0;
            } else {
                // Escribir el carácter de vuelta al UART para que se refleje en el terminal
                uart_write_bytes(UART_NUM, (const char *) (data + index), 1);
                index++;
                if (index >= BUF_SIZE) {
                    // Manejar un buffer lleno sin recibir Enter
                    index = 0;
                }
            }
        }
    }

    free(data);
}
