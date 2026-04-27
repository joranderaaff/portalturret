# Portal Electronics v3

## Bill of Materials

| Qty | Part | Function | Image |
| --- | --- | --- | --- |
| 1 | ADXL345 accelerometer | Motion/orientation sensing | [ADXL345.jpg](docs/ADXL345.jpg) |
| 1 | MAX98357 amplifier | I2S audio amplifier | [max98357.jpg](docs/max98357.jpg) |
| 1 | AMS1117_3V3 power supply | 3.3 V voltage regulation | - |
| 1 | Wemos S2 Mini CPU | Main controller board | [Wemos-S2-Mini.jpg](docs/Wemos-S2-Mini.jpg) |
| 1 | 4 Way IIC I2C Level Conversion Module | I2C voltage level conversion | [i2c-voltage-level-converter.jpg](docs/i2c-voltage-level-converter.jpg) |
| 1 per LED | 20 ohm SMD resistor | LED current limiting | - |
| 2 | 4.7 uF SMD capacitor | Power supply decoupling | - |

## Assembly Notes

- Bridge the `RG` pads and use one `20 ohm` resistor per gun LED
- Use one `20 ohm` SMD resistor for RC
- Fit two `4.7 uF` SMD capacitors for C33_1 C33_2

## Reference

- [PCB preview](docs/pcb.jpg)
- [Final build PCB 1](docs/pcb_1.jpg)
- [Final build PCB 2](docs/pcb_2.jpg)
