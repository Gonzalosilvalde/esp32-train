#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include <math.h>


#define UART_NUM UART_NUM_0
#define BUF_SIZE (1024)

#define RESTA_ASM(dest, op1, op2) \
    __asm__ __volatile__( \
        "sub %0, %1, %2\n" \
        : "=r"(dest) \
        : "r"(op1), "r"(op2))

struct Vector
{
    int x;
    int y;
};

int cuadrado(int numero)
{
    int resultado;
    __asm__ __volatile__(
        "mull %0, %1, %2\n" // %0 = %1 + %2 (resultado = v.x + v.y)
        : "=r"(resultado)   // Output: resultado es un registro de salida (%0)
        : "r"(numero), "r"(numero));
    return resultado;
}

float raizCuadrada(float numero)
{
    // Caso especial para el número 0
    if (numero == 0) return 0;

    float x = numero;
    float y = 1;  // Empezamos con una estimación inicial de y = 1
    int contador = 0;
    
    // Usamos una tolerancia para determinar cuándo parar el bucle
    float tolerancia = 0.00001;  // precisión deseada

    while (fabs(y * y - x) > tolerancia)
    {
        y = (y + x / y) / 2;
        contador++;

        // Evitamos un bucle infinito si no converge
        if (contador > 1000) {
            
            break;
        }
    }
    
    return y;
}




float distanciaEuclidiana(struct Vector v1, struct Vector v2)
{
    int resta1;
    int resta2;
    int resultado;

    RESTA_ASM(resta1, v1.x, v2.x);
    RESTA_ASM(resta2, v1.y, v2.y);
    resta1 = cuadrado(resta1);
    resta2 = cuadrado(resta2);
    return raizCuadrada(resta1 + resta2);
}

#define RESTA_ASM(dest, op1, op2) \
    __asm__ __volatile__( \
        "sub %0, %1, %2\n" \
        : "=r"(dest) \
        : "r"(op1), "r"(op2))

int distanciaManhattan(struct Vector v1, struct Vector v2)
{
    int resta1;
    int resta2;

    RESTA_ASM(resta1, v1.x, v2.x);
    RESTA_ASM(resta2, v1.y, v2.y);

    return abs(resta1) + abs(resta2);
}

int distanciaChebyshev(struct Vector v1, struct Vector v2)
{
    int resta1;
    int resta2;

    RESTA_ASM(resta1, v1.x, v2.x);
    RESTA_ASM(resta2, v1.y, v2.y);

    if (resta1 > resta2)
    {
        return resta1;
    }
    else
    {
        return resta2;
    }
}


void app_main()
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

    uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
    int index = 0;
    int count = 0; // Variable para contar las entradas recibidas
    struct Vector v1;
    struct Vector v2;

    while (1)
    {
        // Leer datos del UART
        int len = uart_read_bytes(UART_NUM, data + index, 1, portMAX_DELAY);

        if (len > 0)
        {
            // Verificar si se ha recibido un carácter de nueva línea (Enter)
            if (data[index] == '\n' || data[index] == '\r')
            {
                data[index] = '\0'; // Terminar la cadena con NULL byte
                // Extraer los valores enteros de data y asignarlos a v.x y v.y
                int int1, int2;
                sscanf((const char *)data, "(%d,%d)", &int1, &int2);

                if (count == 0)
                {
                    v1.x = int1;
                    v1.y = int2;
                    printf("\r\nPrimer número recibido: (%d,%d)\r\n", v1.x, v1.y);
                    count++;
                }
                else if (count == 1)
                {
                    v2.x = int1;
                    v2.y = int2;
                    printf("Segundo número recibido: (%d,%d)\r\n", v2.x, v2.y);

                    // Calcular la distancia euclidiana entre los dos vectores
                    printf("Distancia euclidiana: %f\r\n", distanciaEuclidiana(v1, v2));
                    // Calcular la distancia Manhattan entre los dos vectores
                    printf("Distancia Manhattan: %d\r\n", distanciaManhattan(v1, v2));
                    // Calcular la distancia Chebyshev entre los dos vectores
                    printf("Distancia Chebyshev: %d\r\n", distanciaChebyshev(v1, v2));
                    

                    count = 0; // Reiniciar contador para esperar el próximo par de números
                }

                // Reiniciar el índice para recibir el próximo número
                index = 0;
            }
            else
            {
                // Escribir el carácter de vuelta al UART para que se refleje en el terminal
                uart_write_bytes(UART_NUM, (const char *)(data + index), 1);
                index++;
                if (index >= BUF_SIZE)
                {
                    // Manejar un buffer lleno sin recibir Enter
                    index = 0;
                }
            }
        }
    }

    free(data);
}
