  
/*****************************************************************************
 * 
 * (c) 2020 Wouter D'Haeseleer <wouter@netdata.be>
 * 
 * o2 analyser - gasblender.org
 * source code derivated from ej's o2 oled analyzer - v0.21 (http://ejlabs.net/arduino-oled-nitrox-analyzer)  
 * 
 * License  
 * -------    
 * This program is free software: you can redistribute it and/or modify    
 * it under the terms of the GNU General Public License as published by    
 * the Free Software Foundation, either version 3 of the License, or    
 * (at your option) any later version.
 *     
 *     This program is distributed in the hope that it will be useful,    
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    
 *     GNU General Public License for more details.
 *         
 *         You should have received a copy of the GNU General Public License    
 *         along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *         
 *****************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_ADS1015.h>
#include "RunningAverage.h"
#include <Arduino.h>
#include <U8x8lib.h>
#include <avr/wdt.h>

#define RA_SIZE 20
RunningAverage RA(RA_SIZE);
RunningAverage battVolt(20);

Adafruit_ADS1115 ads(0x48);

U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);


double barsize = RA_SIZE / 16.0 ;
double oxVair = 0;
double oxV1atm = 0;
double oxVmax = 0;


double result;
double oxVact = 0.0;

float multiplier;
byte previous = HIGH;
unsigned long firstTime; // how long since the button was first pressed
char battPcrt = 0;
char active = 0;
char bootcounter = 0;
char toggle = 0;
char errorState = 0;
char page = 0;
double result_max = 0;
uint8_t lijn_onder[8] = { 128,128,128,128,128,128,128,128};

uint8_t lijn_midden[8] = { 8,8,8,8,8,8,8,8};
uint8_t table_left[8] = { 0,0,0,255,8,8,8,8};
uint8_t table_right[8] = { 8,8,8,255,0,0,0,0};

uint8_t table_center[8] = { 8,8,8,255,8,8,8,8};

uint8_t lijn_boven[8] = { 1,1,1,1,1,1,1,1};
uint8_t lijn_links[8] = { 255,0,0,0,0,0,0,0};
uint8_t lijn_rechts[8] = { 0,0,0,0,0,0,0,255};

uint8_t middlebar[8] = { 60  ,60  ,60  ,60  ,60  ,60  ,0   ,0};
uint8_t firstbar[8]  = { 255 ,60  ,60  ,60  ,60  ,60  ,0  , 0};
uint8_t lastbar[8]   = { 60  ,60  ,60  ,60  ,60  ,60  ,0   ,255};

// if the voltage drops below or raises above these thresholds an error is shown
#define milivolt_low_error 6
#define milivolt_high_error 70

#define buttonPin 8
#define buzzer 9  // buzzer


int millis_held;    // How long the button was held (milliseconds)
int secs_held;      // How long the button was held (seconds)
int prev_secs_held; // How long the button was held in the previous check



float cal_mod (float percentage, float ppo2 = 1.4) {
  return 10 * ( (ppo2 / (percentage / 100)) - 1 ); 
}


void beep(int x = 1) { // make beep for x time

  for (int i = 0; i < x; i++) {
    tone(buzzer, 2800, 100);
    delay(200);
  }
  noTone(buzzer);
}


int read_sensor(int adc = 0) {
  int16_t millivolts = 0;
  double currentmv = 0;
  millivolts = ads.readADC_Differential_0_1();
  millivolts = abs(millivolts);

 
  RA.addValue(millivolts);

  currentmv = RA.getAverage();
  currentmv = abs(currentmv);
  return currentmv;

  /* debug  erics
   Serial.print("read_sensor o2: ");
   Serial.print(oxVact);
   Serial.print(" mV\n");
  */
  
  wdt_reset();
}

void setup(void) {
  
  Serial.begin(9600);
  Serial.print("Running: Setup\n");

  page   = EEPROMReadInt(1);
  oxVmax = EEPROMReadInt(2);


  // Init 
  u8x8.begin();     // Init the OLED
  ads.begin();      // ads1115 start
  RA.clear();       // Clear the Running Avarage
  battVolt.clear(); // Clear the Running Avarage


  /* power saving stuff for battery power */
  ACSR = B10000000;
  // Disable digital input buffers on all analog input pins
  DIDR0 = DIDR0 | B00111111;


  /*
    Here we can play with the gain of the ADS chip and its multiplier
    An o2 sensor hase a voltage in air between 7-13 mV
    And it's maximum is at 1bar O2 = 61.9mV
    And at the absuluut max of 3,5 Bar o2 it would be 175mV
    175mV is 0.175V
    
    With GAIN_SIXTEEN the max voltage is +/-0.256V 
    This is well above the the 0.175V
    And gives us the highest resulution
    1 bit = 0.125mV
    Examples found on the internet use GAIN_EIGHT
   */
  
  //ads.setGain(GAIN_TWO);
  //multiplier = 0.0625F;
               
  //ads.setGain(GAIN_EIGHT);u8x8.clearDisplay();
  //multiplier = 0.015625F;

  ads.setGain(GAIN_SIXTEEN);
  multiplier = 0.0078125F; 

  pinMode(buttonPin, INPUT);

  batteryMonitor();


   /*
  // Draw a bootscreen on the OLED
  u8x8.setFont(u8x8_font_inb21_2x4_r);
  u8x8.setInverseFont(1);
  u8x8.drawString(0,0,"         ");
  u8x8.drawString(0,1," NITROX  ");
  u8x8.setInverseFont(0);
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0,5,"Wouter D'H v1.0");
  u8x8.drawString(6,7,"2020");
  delay(2000);
  */
  
  u8x8.clearDisplay();
  //Serial.print("Setup finished\n");



  beep(1);
}


// Write to the EEPROM 
// We use this to store data accross a reboot
void EEPROMWriteInt(int p_address, int p_value)
{
  byte lowByte = ((p_value >> 0) & 0xFF);
  byte highByte = ((p_value >> 8) & 0xFF);

  EEPROM.write(p_address, lowByte);
  EEPROM.write(p_address + 1, highByte);
}

unsigned int EEPROMReadInt(int p_address)
{
  byte lowByte = EEPROM.read(p_address);
  byte highByte = EEPROM.read(p_address + 1);

  return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
}

int calibrate(float oxGas) {
 
    //Serial.print("calibrating for sampleling gas : ");
    //Serial.print(oxGas);
    //Serial.print(" % Oxygen\n");
  

  // Clear the Running Avarage so we can start fresh
  RA.clear();
  
  u8x8.clearDisplay();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0,1,"AUTO CALIBRATING");


  
  u8x8.setFont(u8x8_font_chroma48medium8_r);


  u8x8.drawTile(0,3, 1, lijn_onder);
  u8x8.drawTile(1,3, 1, lijn_onder);
  u8x8.drawTile(2,3, 1, lijn_onder);  
  u8x8.drawTile(3,3, 1, lijn_onder);
  u8x8.drawTile(4,3, 1, lijn_onder);  
  u8x8.drawTile(5,3, 1, lijn_onder);
  u8x8.drawTile(6,3, 1, lijn_onder);
  u8x8.drawTile(7,3, 1, lijn_onder);
  u8x8.drawTile(8,3, 1, lijn_onder);
  u8x8.drawTile(9,3, 1, lijn_onder);
  u8x8.drawTile(10,3, 1, lijn_onder);
  u8x8.drawTile(11,3, 1, lijn_onder);
  u8x8.drawTile(12,3, 1, lijn_onder);
  u8x8.drawTile(13,3, 1, lijn_onder);
  u8x8.drawTile(14,3, 1, lijn_onder);
  u8x8.drawTile(15,3, 1, lijn_onder);

  u8x8.drawTile(0,4, 1, lijn_links);
  u8x8.drawTile(15,4, 1, lijn_rechts);

  u8x8.drawTile(0,5, 1, lijn_boven);
  u8x8.drawTile(1,5, 1, lijn_boven);
  u8x8.drawTile(2,5, 1, lijn_boven);  
  u8x8.drawTile(3,5, 1, lijn_boven);
  u8x8.drawTile(4,5, 1, lijn_boven);  
  u8x8.drawTile(5,5, 1, lijn_boven);
  u8x8.drawTile(6,5, 1, lijn_boven);
  u8x8.drawTile(7,5, 1, lijn_boven);
  u8x8.drawTile(8,5, 1, lijn_boven);
  u8x8.drawTile(9,5, 1, lijn_boven);
  u8x8.drawTile(10,5, 1, lijn_boven);
  u8x8.drawTile(11,5, 1, lijn_boven);
  u8x8.drawTile(12,5, 1, lijn_boven);
  u8x8.drawTile(13,5, 1, lijn_boven);
  u8x8.drawTile(14,5, 1, lijn_boven);
  u8x8.drawTile(15,5, 1, lijn_boven);


  u8x8.setCursor(2, 7);
  u8x8.print(oxGas);
  u8x8.print(" oxygen");

  double result;

  
  int bar = 1;
  
  for (int cx = 0; cx <= RA_SIZE; cx++) {
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    
    if(cx >= (barsize*bar)) {

      switch (bar) {
        case 1:
          u8x8.drawTile(bar-1,4, 1, firstbar);
          break;
        case 16:
          u8x8.drawTile(bar-1,4, 1, lastbar);
          break;
        default:
          u8x8.drawTile(bar-1,4, 1, middlebar);
          break;
      }
      bar = bar+1;
    }
    result = read_sensor(0);
    delay(50);
  }
  result = RA.getAverage();
  result = abs(result);
  
  //Serial.print("Calibration value o2: ");
  //Serial.print(result);
  //Serial.print("\n");


   

  if (result <= milivolt_low_error) {
    errorState=1;
    return 0;
  } else {
    u8x8.clearDisplay();
    active = 0;

    if ( oxGas == 20.9 ) {

      // This is the 100% linear value of the milivolts expected
      // From the sensor in 1 bar o2 == 100 % oxygen
      oxV1atm = 1/0.209 * result * multiplier;

      //Serial.print("Theoretical linear 100% o2 voltage level: ");
      //Serial.print(oxV1atm);
      //Serial.print(" mV \n");
      
    } else if ( oxGas == 100.0 ) {
      //Serial.print("Voltage level for 100% o2 : ");
      //Serial.print(result * multiplier);
      //Serial.print(" mV \n");
      EEPROMWriteInt(2, result);
    }
    return result;
  }
}

void error(int e) {


  oxVact = read_sensor(0);
  oxVact = oxVact * multiplier;





  

  // Serial.print("mv = ");
  // Serial.print(oxVact);
  // Serial.print("\n");


  u8x8.setFont( u8x8_font_open_iconic_check_2x2 );
  u8x8.drawString(3,0,"D");
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(7,0,"SENSOR");
  u8x8.drawString(7,1,"ERROR");
    

  u8x8.setInverseFont(1);

  if ( e == 1 ) {
     u8x8.drawString(0,4,"  Voltage LOW   ");
   } else if ( e == 2 ) {
     u8x8.drawString(0,4,"  Voltage High   ");
   } else {
     u8x8.drawString(0,4,"  Unknown Err   ");
   }  
  
  u8x8.setInverseFont(0);

  u8x8.setCursor(3, 7);
  u8x8.print(oxVact , 2);
  u8x8.print(" mV ");

  if (oxVact > milivolt_low_error && oxVact < milivolt_high_error ) {
    errorState = 0;
    oxVair = 0;
    result_max = 0;
  } 
}

byte countDigits(int num){
  byte count=0;
  while(num){
    num=num/10;
    count++;
  }
  return count;
}

void batteryMonitor() {
  int battVolt_Value = analogRead(A0);
  battVolt.addValue(battVolt_Value * (5.0 / 1023.0));
  float volt = battVolt.getAverage();

  if ( volt >= 4.1 )  {
    battPcrt = 100;
  }
  if ( volt < 4.1 && volt >=  4.0 ) {
    battPcrt = 90;
  }
  if ( volt < 4.0 && volt >=  3.9 ) {
    battPcrt = 80;
  }
  if ( volt < 3.9 && volt >=  3.8 ) {
    battPcrt = 70;
  }
  if ( volt < 3.8 && volt >=  3.7 ) {
    battPcrt = 60;
  }
  if ( volt < 3.7 && volt >=  3.6 ) {
    battPcrt = 20;
  }
  if ( volt < 3.6 && volt >=  3.5 ) {
    battPcrt = 10;
  }
  if ( volt < 3.5 ) {
    battPcrt = 0;
  }
}

void header(float result) {
      u8x8.setFont(u8x8_font_inb21_2x4_r);      
      u8x8.setCursor(3, 0);
      u8x8.print(result, 1);

      // Draw a lin under the result
      u8x8.drawTile(0,4, 1, lijn_boven);
      u8x8.drawTile(1,4, 1, lijn_boven);
      u8x8.drawTile(2,4, 1, lijn_boven);
      u8x8.drawTile(3,4, 1, lijn_boven);
      u8x8.drawTile(4,4, 1, lijn_boven);
      u8x8.drawTile(5,4, 1, lijn_boven);
      u8x8.drawTile(6,4, 1, lijn_boven);
      u8x8.drawTile(7,4, 1, lijn_boven);
      u8x8.drawTile(8,4, 1, lijn_boven);
      u8x8.drawTile(9,4, 1, lijn_boven);
      u8x8.drawTile(10,4, 1, lijn_boven);
      u8x8.drawTile(11,4, 1, lijn_boven);
      u8x8.drawTile(12,4, 1, lijn_boven);
      u8x8.drawTile(13,4, 1, lijn_boven);
      u8x8.drawTile(14,4, 1, lijn_boven);
      u8x8.drawTile(15,4, 1, lijn_boven);

      u8x8.setFont(u8x8_font_chroma48medium8_r);
}

void analysing(int x, int cal, int cal100) {
  int mod = 0;
  

  float oxVact = read_sensor(0);




  //Serial.print("Oxygen = ");   
  if (cal100 > 0 ) {

 
   result = 20.9 + 79.1*(oxVact - cal)/(cal100 - cal);
   //Serial.print(result);    
   //Serial.print(" %  - Calibrated at air and 100% o2\n");   
  } else {
   result = (float(oxVact)/float(cal) )*20.9; 
   //Serial.print(result);    
   //Serial.print(" %  - Calibrated only on air\n");   
  }

  oxVact = oxVact*multiplier;


  if (result > 99.9) result = 99.9;


  if (oxVact < milivolt_low_error || result <= 0  ) {
    u8x8.clearDisplay();
    errorState=1;
  } else if ( oxVact > milivolt_high_error ) {
    u8x8.clearDisplay();
    errorState=2; 
  } else {
    u8x8.setFont(u8x8_font_chroma48medium8_r);

    if (result >= result_max) {
      result_max = result;
    }




    if ( page == 0 ) {

      header(result);


      u8x8.setCursor(4, 5);
      u8x8.print("| Max  |");

      u8x8.drawTile(4,6, 1, table_left);
      u8x8.drawTile(5,6, 1, lijn_midden);
      u8x8.drawTile(6,6, 1, lijn_midden);
      u8x8.drawTile(7,6, 1, lijn_midden);
      u8x8.drawTile(8,6, 1, lijn_midden);
      u8x8.drawTile(9,6, 1, lijn_midden);
      u8x8.drawTile(10,6, 1, lijn_midden);
      u8x8.drawTile(11,6, 1, table_right);


      u8x8.setCursor(4, 7);
      u8x8.print("|");
      u8x8.setCursor(11, 7);
      u8x8.print("|");
      u8x8.setCursor(6, 7);

      u8x8.setFont(u8x8_font_amstrad_cpc_extended_n);

      u8x8.print(result_max, 1);
      u8x8.print(" ");


      
    }


    // MOD Display
    if ( page == 1 ) {

      header(result);


      // Print table with MOD 1.4 and 1.6 ppO2
      u8x8.setCursor(4, 5);
      u8x8.print("| 1.4 | 1.6 ");

      u8x8.drawTile(4,6, 1, table_left);
      u8x8.drawTile(5,6, 1, lijn_midden);
      u8x8.drawTile(6,6, 1, lijn_midden);
      u8x8.drawTile(7,6, 1, lijn_midden);
      u8x8.drawTile(8,6, 1, lijn_midden);
      u8x8.drawTile(9,6, 1, lijn_midden);
      u8x8.drawTile(10,6, 1, table_center);
      u8x8.drawTile(11,6, 1, lijn_midden);
      u8x8.drawTile(12,6, 1, lijn_midden);
      u8x8.drawTile(13,6, 1, lijn_midden);
      u8x8.drawTile(14,6, 1, lijn_midden);
      u8x8.drawTile(15,6, 1, lijn_midden);

      u8x8.setCursor(0, 7);
      u8x8.print("MOD | ");

      u8x8.setFont(u8x8_font_amstrad_cpc_extended_n);
      u8x8.print(floor(cal_mod(result, 1.4)),0);
      u8x8.print(" ");

      u8x8.setFont(u8x8_font_chroma48medium8_r);
      u8x8.setCursor(10, 7);
      u8x8.print("|");

      u8x8.setFont(u8x8_font_amstrad_cpc_extended_n);
      u8x8.setCursor(12, 7);
      u8x8.print(floor(cal_mod(result, 1.6)),0);
      u8x8.print(" ");      
    }


    // Display Only large result
    if ( page == 2 ) {
      u8x8.setFont(u8x8_font_inb46_4x8_n);  
      u8x8.setCursor(0, 0);
      u8x8.print((float) result, 1);   
    }

    // Page with technical info
    // This page is NOT saved to EEPROM
    if ( page == 3 ) {

      u8x8.setCursor(0, 0);
      u8x8.print(" TECHNICAL INFO");
      u8x8.setCursor(0, 1);
      
      u8x8.drawTile(1,1, 1, lijn_midden);
      u8x8.drawTile(2,1, 1, lijn_midden);
      u8x8.drawTile(3,1, 1, lijn_midden);
      u8x8.drawTile(4,1, 1, lijn_midden);
      u8x8.drawTile(5,1, 1, lijn_midden);
      u8x8.drawTile(6,1, 1, lijn_midden);
      u8x8.drawTile(7,1, 1, lijn_midden);
      u8x8.drawTile(8,1, 1, lijn_midden);
      u8x8.drawTile(9,1, 1, lijn_midden);
      u8x8.drawTile(10,1, 1, lijn_midden);
      u8x8.drawTile(11,1, 1, lijn_midden);
      u8x8.drawTile(12,1, 1, lijn_midden);
      u8x8.drawTile(13,1, 1, lijn_midden);
      u8x8.drawTile(14,1, 1, lijn_midden);
      u8x8.drawTile(15,1, 1, lijn_midden);
      


      u8x8.setCursor(6, 2);
      u8x8.print("|");

      u8x8.setCursor(0, 3);

      u8x8.print("Cell  | ");
      u8x8.print( oxVact , 2); 
      u8x8.print(" mV");

      u8x8.setCursor(0, 4);
      u8x8.print("Batt  | ");
      u8x8.print(battVolt.getAverage() , 1);
      u8x8.print(" V ");

      u8x8.setCursor(0, 5);
      u8x8.print("Batt  | ");
      u8x8.print(battPcrt);
      u8x8.print(" % ");


      u8x8.setCursor(0, 6);
      if (cal100 > 0 ) {
        u8x8.print("Cal   | Air + o2");
      } else {
        u8x8.print("Cal   | Air    ");
      }
      u8x8.setCursor(0, 7);
      u8x8.print("Firmw | 1.1");
    }
  }
}


void max_clear() {
  result_max = 0;
  beep(1);
  battVolt.clear();
  active = 0;
}



void loop(void) {
  // Enable the Watchdog
  // Reset arduino if watchdog did not receive a reset after 8 seconds
  wdt_enable(WDTO_8S);
  batteryMonitor();

 
 int current = digitalRead(buttonPin);
 
  if (current == HIGH && previous == LOW && (millis() - firstTime) > 200) {
    firstTime = millis();
  }

  millis_held = (millis() - firstTime);


  if (millis_held > 2) {
    if (current == LOW && previous == HIGH) {
      if (millis_held <= 400) {
        
        page++;
        if ( page >= 4 ) {
          page = 0;
        }

        if ( page != 3 ) { 
          EEPROMWriteInt(1, page);
        }
        
        u8x8.clearDisplay();
      }
      if (millis_held >= 400 && millis_held < 5000 ) {



        // Check if the measured mV is high enough to calibrate on 100% oxygen
        // We allow a difference of 10mV to callibrate to 100%
        // Save it to the memroy, we only need to do this once in a while
        int oxVact = read_sensor(0);
        oxVact = oxVact*multiplier;
        
        if ( (oxVact > (oxV1atm - 10) ) &&  oxVair > 0) {
          oxVmax = calibrate(100.0);
        }else {
          max_clear();
          oxVair = calibrate(20.9);
        }
      }
      
      if (millis_held >= 5000 ) {
         //Serial.print("Clear  100% o2\n");
         max_clear();
         oxVmax=0;
         EEPROMWriteInt(2, 0);
         oxVair = calibrate(20.9);
      }
      
    }
  }


  previous = current;
  prev_secs_held = secs_held;

  if ( errorState > 0 ) {
    error(errorState);
  } else { 
    //calibration forced on boot
    if (oxVair == 0) {
      oxVair = calibrate(20.9);
    }

    analysing(0, oxVair, oxVmax);
  }
  wdt_reset();
  active++;
}
