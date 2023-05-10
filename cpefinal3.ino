//nicky cruz-nunez
// I had no partners lol.

#include "DHT.h"
#include <LiquidCrystal.h>

#define DHTPIN 2
#define DHTTYPE DHT11

#define tempF 80.0 // fahrenheit temperature
#define tempc 26.6667 // celsius temperature
#define watermax 100

unsigned int currentTicks = 65535;
DHT dht(DHTPIN, DHTTYPE);

unsigned char waterlevelport = 0;

//#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 7, 6, 5, 4, 3);

volatile unsigned char* myTCCR1A = (unsigned char*) 0x80;
volatile unsigned char* myTCCR1B = (unsigned char*) 0x81;
volatile unsigned char* myTCCR1C = (unsigned char*) 0x82;
volatile unsigned char* myTIMSK1 = (unsigned char*) 0x6F;
volatile unsigned int* myTCNT1 = (unsigned int*) 0x84;
volatile unsigned char* myTIFR1 = (unsigned char*) 0x36;

// DIGITAL PORT B REGISTERS
volatile unsigned char* port_b = (unsigned char*) 0x25;
volatile unsigned char* ddr_b = (unsigned char*) 0x24;
volatile unsigned char* pin_b = (unsigned char*) 0x23;

volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

enum CoolerState {
  IDLE,
  RUNNING,
  DISABLED,
  WATER,
  OVERTEMP
};

CoolerState currentState = DISABLED;

void setup() {

  *ddr_b |= 0b00111111;

  lcd.begin(16, 2);

  lcd.setCursor(0, 0);
  lcd.print("Hello, world!");

  Serial.begin(9600);

  currentState = IDLE;

  adc_init();

  dht.begin();
}

void loop() {


  lcd.clear(); // clear the LCD screen
  

  float temp; // declare the variable here

  switch (currentState) {
    case IDLE:
      // Get temperature and humidity readings
      temp = getTemperature(true);
      float humid = getHumidity();

      // Display the current temperature and humidity on the LCD screen
      lcd_th(temp, humid);

      // Check water level
      unsigned int water_lvl = waterLevel();
      if (water_lvl < watermax) {
        currentState = WATER;
        // Display an appropriate message on the LCD screen
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Water level low!");
        lcd.setCursor(0, 1);
        lcd.print("Refill water tank.");
      }
      else if (temp > tempF) {
        currentState = OVERTEMP;
        // Display an appropriate message on the LCD screen
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temperature too high!");
        lcd.setCursor(0, 1);
        lcd.print("Cooling down...");
        // Wait for some time to cool down the device
        delay(5000);
      }
      break;

    case WATER:
      // Check water level
      water_lvl = waterLevel();
      if (water_lvl >= watermax) {
        currentState = IDLE;
        // Display the current temperature and humidity on the LCD screen
        float humid = getHumidity();
        lcd_th(temp, humid);
      }
      break;

    case OVERTEMP:
      // Check temperature
      temp = getTemperature(true);
      if (temp <= tempF) {
        currentState = IDLE;
        // Display the current temperature and humidity on the LCD screen
        float humid = getHumidity();
        lcd_th(temp, humid);
      }
      else {
        // Wait for some time to cool down the device
        delay(5000);
      }
      break;

    case RUNNING:
      // Turn on the cooler
      *port_b |= 0b00000110;

      // Check temperature
      temp = getTemperature(true);
      if (temp <= tempF) {
        currentState = IDLE;
        // Display the current temperature and humidity on the LCD screen
        float humid = getHumidity();
        lcd_th(temp, humid);
        // Turn off the cooler
        *port_b &= 0b11111001;
      }
      break;
  }
}

float getTemperature(bool fahrenheit) {
  float temperature = dht.readTemperature();
  if (isnan(temperature)) {
    Serial.println("failed ");
    return -1.0;
  }
  if (fahrenheit) {
    return temperature * 1.8 + 32.0;
  }
  else {
    return temperature;
  }
}

float getHumidity() {
  float humidity = dht.readHumidity();
  if (isnan(humidity)) {
    Serial.println("Failed ");
    return -1.0;
  }
  return humidity;
}

unsigned int waterLevel() {
  *my_ADMUX = waterlevelport | 0b01000000;
  *my_ADCSRA |= 0b01000000; // start conversion
  while (*my_ADCSRA & 0b01000000) {} // wait for conversion to finish
  return *my_ADC_DATA;
}

void adc_init()
{
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}
unsigned int adc_read(unsigned char adc_channel_num)
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}



void lcd_th(float temp, float humid) {
lcd.setCursor(0,0);
lcd.print("Temp: Humidity:");
lcd.setCursor(0,1);
lcd.print(temp);
lcd.setCursor(7,1);
lcd.print(humid);
}

float temp_to_c(float temp_f) {
  return (temp_f - 32.0) / 1.8;
}





