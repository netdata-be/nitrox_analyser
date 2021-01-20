# Nitrox Anylyser

## BOM 

| # | Name                                            | Price  |
|---|-------------------------------------------------|--------|
| 1 | PCB custom made                                 |  9,99  |
| 1 | Arduino Nano                                    | 19,00  |
| 1 | ADS1115                                         | 14,95  |
| 1 | 1.3 inch OLED Display 128*64 pixels wit - I2C   |  8,00  |
| 1 | MT3608 DC-DC Boost converter                    |  2,00  |
| 1 | Diode 1N4007                                    |  0,10  |
| 1 | 18650 Battery holder to solder                  |  1,50  |
| 1 | 18650 Battery with protection                   |  6,50  |
| 1 | Audio Jack 3,5mm (mono) 0,6m                    |  1,50  |
| 1 | 40 Pin header row                               |  0,40  |
| 1 | [O2 oxygen Cell (R-17VAN Oxygen Sensor) or other](https://www.vandagraph.com/index.php?main_page=product_info&cPath=5_8&products_id=30) | 79,00  |
| 1 | Spelsberg Kabeldoos abox 060-62                 | 12,00  |
| 1 | On/Off button                                   |  2,50  |
| 1 | Push Button                                     |  2,50  |
| 1 | [B-50057 Flow Diverter](https://www.vandagraph.com/index.php?main_page=product_info&cPath=5_6&products_id=17?main_page=product_info&cPath=5_6&products_id=17)                         |  5,00  |
| 1 | [Quick-Ox Sampler](https://www.vandagraph.com/index.php?main_page=product_info&products_id=20)                              | 22,00  |


Total cost: 194,95

The sensor, flow divertor, and Quick-Ox are high end quality products as they are needed for an accurate measurement.


## Functions 

* Auto calibrate iteself at boot to 20.9 oxygen
* Force a calibration by long pressing the button
* When measuring 100% o2 and forcing a calibration by a long press a second calibration point is stored ( 20,9% air ADN 100% ) 
  so compensating for the not fully linear scale.
* Each press on the button gives you an other page with info
  This page is saved to eeprom so if you shutdown and restart the analyzer the last page selected will be shown again.
* Battery monitor of the 18650 battery


## Screenshots of the different screens

![Calibration_screen](/img/screen_calibration.jpg)

![Big screen](/img/screen_big.jpg)

![Error screen](/img/Screen_error.jpg)

![Max screen](/img/screen_max_mix.jpg)

![MOD screen](/img/screen_mod.jpg)

![Tech info screen](/img/screen_tech_info.jpg)

## PCB

![PCB](/img/mainboard.png)
