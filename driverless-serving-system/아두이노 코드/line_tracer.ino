#include "HX711.h"
#include <OneWire.h>
#include <SPI.h>
#include <WiFi.h>
#include <stdio.h>
#include <stdlib.h>

// Line tracer

const int ENA = 11; // PWM
const int IN1 = 12;
const int IN2 = 13;

const int ENB = 10; // PWM
const int IN3 = 9;
const int IN4 = 8;

const int right_s = A0; //0;
const int middle_s = A1; //1;
const int left_s = A2;


int sp = 254;

//int plus = 0; // 40

int SR = 0;
int SM = 0;
int SL = 0;

int check_tran = 0;
int check_duple = 0;

int rest = 1500;
int minus = 1000;

int del = 10;

// ultrasonic
int trig = 4;
int echo = 6; // PMW (~)
int distance = 0;

// noise
int buzzer = 5; // PWM

// carol
int speakerPin = 5; //PWM
int leng = 51;
char notes[] = "eeeeeeegcde fffffeeeeddedgeeeeeeegcde fffffeeeggfdc"; 
int beats[] = { 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 4,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2,
                1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 4,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4
              }; 
int tempo = 300;


//temperature
int DS18S20 = 3; // PWM
OneWire ds(DS18S20);
float temperature = 0;

//weight
HX711 scale(A3, A4); // DOUT, SCK
float weight = 0;

//wifi
char ssid[] = "DongHun_Home";
char pass[] = "railgun20";
int keyIndex = 0;
int status = WL_IDLE_STATUS;
IPAddress server(192,168,168,6);
WiFiClient client;
char current_state = 'x';
//char previous_state = 'z';
char temp = 'x';
int change_state = 0;







int weight_temp = 0;









void noising(void)
{
  digitalWrite(buzzer, HIGH);
  delay(50);
  digitalWrite(buzzer, LOW);
  delay(50);
}

void playTone(int tone, int duration)
{
  for (long i = 0; i < duration * 1000L; i += tone * 2)
  {
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(tone);
  }
}

void playNote(char note, int duration)
{
  char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' }; 
  int tones[] = { 1915, 1700, 1519, 1432, 1275, 1136, 1014, 956 };  
  for (int i = 0; i < 8; i++)
  {
    if (names[i] == note)
    {
      playTone(tones[i], duration);
    }
  }
}

void carolsong()
{
  for (int i = 0; i < leng; i++)
  {
    if (notes[i] == ' ')
    {
      delay(beats[i] * tempo); // rest
    }
    else
    {
      playNote(notes[i], beats[i] * tempo);
    }
    delay(tempo / 10);
  }
  delay(500);
}

//temperature
float getTemp() {
  byte data[12];
  byte addr[8];
  if ( !ds.search(addr)) {
    ds.reset_search();
    return -1000;
  }
  if ( OneWire::crc8( addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return -1000;
  }
  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    Serial.print("Device is not recognized");
    return -1000;
  }
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);
  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);
  for (int i = 0; i < 9; i++)  {
    data[i] = ds.read();
  }
  ds.reset_search();
  byte MSB = data[1];
  byte LSB = data[0];
  float tempRead = ((MSB << 8) | LSB);
  float TemperatureSum = tempRead / 16;
  return TemperatureSum;
}










void brake(void)
{
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);
 
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, 0);
}

void backward(void)
{
  brake(); 
  
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, sp);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENB, sp);

  delay(del);
}

void right(void)
{
  brake();

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, sp);

  delay(del);
}

void left(void)
{
  brake();
  
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, sp);
 
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, 0);

  delay(del);
}

void forward(void)
{
  brake();

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, sp);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, sp);

  delay(del);
}









void table_go1(void)
{
  if(check_duple == 0 && check_tran==0)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==1)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==2)
  {
    check_tran++;
    check_duple = 1;
    brake();
    
    carolsong(); // arrive

    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==3)
  {
    check_tran++;
    check_duple = 1;
    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==4)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==5)
  {
    check_tran++;
    check_duple = 1;
    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==6)
  {
    check_tran++;
    check_duple = 1;
    right();
    delay(rest);

  }
  else if(check_duple == 0 && check_tran==7)
  {
    check_tran++;
    check_duple = 1;
    brake();
    check_tran=0;
    change_state=0;
    weight_temp=0;
    temp = 'x';
    delay(5000);
  }
}



void table_go2(void)
{
  if(check_duple == 0 && check_tran==0)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==1)
  {
    check_tran++;
    check_duple = 1;
    brake();
    
    carolsong(); // arrive

    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==2)
  {
    check_tran++;
    check_duple = 1;
    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==3)
  {
    check_tran++;
    check_duple = 1;
    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==4)
  {
    check_tran++;
    check_duple = 1;
    right();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==5)
  {
    check_tran++;
    check_duple = 1;
    brake();
    check_tran=0;
    change_state=0;
    weight_temp=0;
    temp = 'x';
    delay(5000);
  }
  
}



void table_go3(void)
{
  if(check_duple == 0 && check_tran==0)
  {
    check_tran++;
    check_duple = 1;
    brake();
    
    carolsong(); // arrive

    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==1)
  {
    check_tran++;
    check_duple = 1;
    right();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==2)
  {
    check_tran++;
    check_duple = 1;
    right();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==3)
  {
    check_tran++;
    check_duple = 1;
    right();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==4)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==5)
  {
    check_tran++;
    check_duple = 1;
    brake();
    check_tran=0;
    change_state=0;
    weight_temp=0;
    temp = 'x';
    delay(5000);
  }
  
}


//--------------------------------------------------

void table_go4(void)
{
  if(check_duple == 0 && check_tran==0)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==1)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==2)
  {
    check_tran++;
    check_duple = 1;
    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==3)
  {
    check_tran++;
    check_duple = 1;
    brake();
    
    carolsong(); // arrive

    
    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==4)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==5)
  {
    check_tran++;
    check_duple = 1;
    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==6)
  {
    check_tran++;
    check_duple = 1;
    right();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==7)
  {
    check_tran++;
    check_duple = 1;
    brake();
    check_tran=0;
    change_state=0;
    weight_temp=0;
    temp = 'x';
    delay(5000);
  }
}



void table_go5(void)
{
  if(check_duple == 0 && check_tran==0)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==1)
  {
    check_tran++;
    check_duple = 1;
    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==2)
  {
    check_tran++;
    check_duple = 1;
    brake();
    
    carolsong(); // arrive

    
    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==3)
  {
    check_tran++;
    check_duple = 1;
    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==4)
  {
    check_tran++;
    check_duple = 1;
    right();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==5)
  {
    check_tran++;
    check_duple = 1;
    brake();
    check_tran=0;
    change_state=0;
    weight_temp=0;
    temp = 'x';
    delay(5000);
  }
}



void table_go6(void)
{
  if(check_duple == 0 && check_tran==0)
  {
    check_tran++;
    check_duple = 1;
    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==1)
  {
    check_tran++;
    check_duple = 1;
    brake();
    
    carolsong(); // arrive

    
    right();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==2)
  {
    check_tran++;
    check_duple = 1;
    right();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==3)
  {
    check_tran++;
    check_duple = 1;
    right();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==4)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==5)
  {
    check_tran++;
    check_duple = 1;
    brake();
    check_tran=0;
    change_state=0;
    weight_temp=0;
    temp = 'x';
    delay(5000);
  }
}


//--------------------------------------------



void table_go7(void)
{
  if(check_duple == 0 && check_tran==0)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==1)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==2)
  {
    check_tran++;
    check_duple = 1;
    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==3)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==4)
  {
    check_tran++;
    check_duple = 1;
    brake();
    
    carolsong(); // arrive

    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==5)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==6)
  {
    check_tran++;
    check_duple = 1;
    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==7)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==8)
  {
    check_tran++;
    check_duple = 1;
    right();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==9)
  {
    check_tran++;
    check_duple = 1;
    brake();
    check_tran=0;
    change_state=0;
    weight_temp=0;
    temp = 'x';
    delay(5000);
  }
}



void table_go8(void)
{
  if(check_duple == 0 && check_tran==0)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==1)
  {
    check_tran++;
    check_duple = 1;
    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==2)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==3)
  {
    check_tran++;
    check_duple = 1;
    brake();
    
    carolsong(); // arrive

    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==4)
  {
    check_tran++;
    check_duple = 1;
    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==5)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==6)
  {
    check_tran++;
    check_duple = 1;
    right();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==7)
  {
    check_tran++;
    check_duple = 1;
    brake();
    check_tran=0;
    change_state=0;
    weight_temp=0;
    temp = 'x';
    delay(5000);
  }
}



void table_go9(void)
{
  if(check_duple == 0 && check_tran==0)
  {
    check_tran++;
    check_duple = 1;
    left();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==1)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==2)
  {
    check_tran++;
    check_duple = 1;
    brake();
    
    carolsong(); // arrive

    right();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==3)
  {
    check_tran++;
    check_duple = 1;
    right();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==4)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==5)
  {
    check_tran++;
    check_duple = 1;
    right();
    delay(rest);
  }
  else if(check_duple == 0 && check_tran==6)
  {
    check_tran++;
    check_duple = 1;
    forward();
    delay(rest-minus);
  }
  else if(check_duple == 0 && check_tran==7)
  {
    check_tran++;
    check_duple = 1;
    brake();
    check_tran=0;
    change_state=0;
    weight_temp=0;
    temp = 'x';
    delay(5000);
  }
}







//wifi
void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}



void httpRequest() //여기서 주고받음
{
    
    client.print("H");
    
    while(client.available()) 
    {
      temp = client.read();
      
      if(temp=='a' || temp=='b' || temp=='c'|| temp=='d'|| temp=='e'|| temp=='f'|| temp=='g'|| temp=='h'|| temp=='i')
      {
        current_state = temp;
        change_state = 1;
      }
      Serial.print(temp);
    }
}







void setup() 
{
// line tracer
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(left_s, INPUT);
  pinMode(right_s, INPUT);
  pinMode(middle_s, INPUT);

//초음파
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);

// buzzer
  pinMode(buzzer, OUTPUT);

//weight
  scale.set_scale(2280.f);
  scale.tare();

  Serial.begin(9600);

//wifi

  while (!Serial) {
  }
  
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present"); 
    while(true);
  } 
  
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
  
    delay(5000);
  }
   
  Serial.println("Connected to wifi");
  printWifiStatus();
  
  Serial.println("\nStarting connection to server…");
  
   if (client.connect(server, 3000)) {
    Serial.println("connected to server");
    client.println("H");
    //client.println("Host: www.google.com");
    client.println();
   
  }
  
}


void loop()
{
    if(change_state==0){
      httpRequest();
      delay(3000);
      if(!client.connected()) {
        Serial.println();
        Serial.println("disconnecting from server.");
        delay(5000);
        while(true);
      }
    }
    else
    {
      if(weight_temp==0)
      {
        temperature = getTemp();

        scale.power_up();
        weight = scale.get_units();
        scale.power_down();
        
        Serial.println(temperature);
        Serial.println(weight);
        
        if(temperature >= 24 || weight <= -2000)
        {
          //Serial.println("abc");
          weight_temp = 1;
        }
      }
      else{


      if(change_state==1)
      {
      
         SR = digitalRead(right_s);
         SM = digitalRead(middle_s);
         SL = digitalRead(left_s);

         digitalWrite(trig, HIGH);
         delayMicroseconds(1);//10
         digitalWrite(trig, LOW);

         distance = pulseIn(echo, HIGH)*17/1000;
         Serial.println(distance);
         if(distance <= 12)
         {
           brake();
           noising();
         }
         else
         {
           if (SL == HIGH && SM == LOW && SR == HIGH) // 101 - transaction
           {
             //table_go1();
              
              if(current_state=='a'){
                table_go1();
              }
              else if(current_state=='b'){
                table_go2();
              }
              else if(current_state=='c'){
                table_go3();
              }
              else if(current_state=='d'){
                table_go4();
              }
              else if(current_state=='e'){
                table_go5();
              }
              else if(current_state=='f'){
                table_go6();
              }
              else if(current_state=='g'){
                table_go7();
              }
              else if(current_state=='h'){
                table_go8();
              }
              else if(current_state=='i'){
                table_go9();
              }
              else{
               brake();
              }
              
           }
           else if (SL == LOW && SM == LOW && SR == LOW)  // 000 - error
           {
             backward();
           }
           else if (SL == LOW && SM == LOW && SR == HIGH)  // 001 - error
           {
             right();
           }
           else if (SL == LOW && SM == HIGH && SR == LOW)  // 010
           {
             forward();
             check_duple = 0;
           }
           else if (SL == LOW && SM == HIGH && SR == HIGH) // 011
           {
             right();
           }
           else if (SL == HIGH && SM == LOW && SR == LOW)  // 100 - error
           {
             left();
           }
           else if (SL == HIGH && SM == HIGH && SR == LOW) // 110
           {
              left();
           }
           else if (SL == HIGH && SM == HIGH && SR == HIGH)  // 111
           {
              forward();
           }
          
         }
      }
      }
    }
    

}


