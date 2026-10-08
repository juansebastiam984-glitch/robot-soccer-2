## Conexiones

### Alimentación

| Desde | Hacia |
|---|---|
| Batería LiPo 2S (7.4V +) | VM del TB6612 y entrada del regulador de 5V |
| Salida del regulador (5V) | VIN del ESP32 |
| VIN (5V) del ESP32 | VCC del TB6612 (lógica) |
| GND de batería, regulador, TB6612 y ESP32 | Todos unidos en un mismo punto de tierra |

### TB6612FNG ↔ ESP32 DevKit V1 (30 pines)

| Pin TB6612 | GPIO ESP32 |
|---|---|
| STBY | 13 |
| AIN1 | 27 |
| AIN2 | 26 |
| PWMA | 14 |
| BIN1 | 25 |
| BIN2 | 33 |
| PWMB | 32 |
| AO1 / AO2 | Motor izquierdo |
| BO1 / BO2 | Motor derecho |

### Recomendaciones físicas

- Capacitor electrolítico de 100 a 470µF entre VM y GND, lo más cerca posible del TB6612. Reduce las caídas de voltaje al arrancar los motores.
- Un capacitor cerámico de 100nF directo en los terminales de cada motor, para reducir el ruido eléctrico.
- Usar cable grueso para GND y VM, que llevan la corriente de los motores. Un jumper fino puede causar reinicios del ESP32 a máxima potencia.
- No pasar cables por encima de la antena del ESP32, porque reduce el alcance del Bluetooth.
