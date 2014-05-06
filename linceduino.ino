#include <LiquidCrystal.h>
#include <math.h>
#include <SD.h>
#define REFRESCO_SD 4 //Frecuencia de guardado
#define CS 4 // pin que marca el fabricante de SD Shield
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

float radio=236.82; // radio de la rueda en mm hay que poner la real del lince
volatile float tiempopaso, tiempovuelta;//tiempo (hasta la activación del sensor y de una vuelta)
volatile float velocidad, velocidadm; //velocidades (instantánea y media)
volatile float distancia; //distancia recorrida desde que empieza a circular
volatile int lineasBuffer = 0; // lineas de buffer guardadas
volatile int nvueltas = 0; //vuelta de rueda
volatile int marcha,paro;// estado de pulsadores de impulso o paro
String minutos, segundos; //minutos y segundos desde el inicio
char s_string [10];
char m_string [10];
unsigned long tiempo1=0; //tiempo inicial
int vueltas; //vuelta del circuito
String buffer=""; // Almacenamiento temporal de datos recogidos de ISR, para la tarjeta SD
volatile int botonpulsado=0;// Botón del lcdkeypad
File dataFile; //Fichero para guardar datos
bool c = true; //Booleano para la representación

void setup() {
attachInterrupt(2, calculos, RISING);//pin de interrupción 21 arduino mega para sensor hall
attachInterrupt(4, calculosmarcha, RISING);//pin de interrupción 19 arduino mega: marcha
attachInterrupt(5, calculosparo, RISING);//pin de interrupción 18 arduino mega: paro
lcd.begin(16, 2); //iniciamos la pantalla lcd
pinMode(CS, OUTPUT);
SD.begin(CS);//inicializa la SD (tiene que estar insertada) sólo lo hace una vez
}

void calculos() {
//Calcula los datos a representar
//if (nvueltas==1) {tiempo1=millis();}
nvueltas++;
tiempovuelta=millis()-tiempopaso;//tiempo por vuelta
tiempopaso=millis();//tiempo desde que se produjo la interrupcion
velocidad=(TWO_PI*radio*3.6)/tiempovuelta; // (km/h), [mm/ms*3,6 para pasar a km/h]
distancia=(nvueltas*TWO_PI*radio)/1000000; //distancia total (km)
vueltas= int((distancia)/1.626)+1; // el circuito tiene 1626 m= 1.626 Km
//solo guarda en buffer y por tanto guarda en fichero cuando si se ha pulsado el boton select
guardabuffer();
}

void calculosmarcha() {
//Interrupción para pulsador 'marcha'
marcha=1;
guardabuffer();
}

void calculosparo() {
//Interrupción para pulsador 'paro'
paro=1;
guardabuffer();
}

void guardabuffer(){
//Guarda los datos temporalmente en un buffer
   if(botonpulsado==5) {
   buffer += String(int((millis()-tiempo1)/1000));buffer += ";";
   buffer += nf(velocidad,2,2);buffer += ";";
   buffer += nf(velocidadm,2,2);buffer += ";";
   buffer += nf(distancia,2,2);buffer += ";";
   buffer += vueltas;buffer += ";";
   buffer += marcha;buffer += ";";
   buffer +=paro; buffer += "\n";
   lineasBuffer++;
 }
}

void guardadatos(){
//Guarda el buffer en un archivo
dataFile = SD.open("tele.txt", FILE_WRITE);
dataFile.print(buffer);
lineasBuffer = 0;
buffer = "";
dataFile.close();

lcd.setCursor(15,1);
c = !c;
if(c) {lcd.print("|");}
else {lcd.print("-");}
}

String nf(float n, int cifras, int decimales) {
char numero [10];
dtostrf(n,(cifras+decimales+1),decimales,numero);
int punto = String(numero).indexOf(".");
String parte_entera = String(numero).substring(0,punto);
String parte_decimal = String(numero).substring(punto+1);
return parte_entera + "," + parte_decimal;
}

String tiempo() {
// Devuelve el tiempo de la forma ‘000:00’
dtostrf((millis()-tiempo1)/60000,3,0,m_string);
dtostrf(((millis()-tiempo1) % 60000)/1000,1,0,s_string);
minutos = String(m_string);
segundos = String(s_string);
if (segundos.length() < 2) {segundos = "0" + segundos;}
return minutos + ":" + segundos;
}

void leebotones (int valorsensor) {
// A cada botón le corresponde un rango de valores
if (valorsensor <1000 || valorsensor>1100) {botonpulsado=0;} //ningun boton pulsado
if (valorsensor<10) { botonpulsado=1;}
if (valorsensor>90 && valorsensor<110) {botonpulsado=2;}
if (valorsensor>200 && valorsensor<300) {botonpulsado=3;}
if (valorsensor>400 && valorsensor<450) {botonpulsado=4;} 
if (valorsensor>600 && valorsensor<650) {botonpulsado=5;SD.begin(CS);} //boton select
}

void loop() {
//Bucle principal. Representa y reinicia variables

leebotones(analogRead(0)); //comprueba que botón ha sido pulsado o sino se ha pulsado (0)

if (nvueltas==1) {tiempo1=millis();}
if (lineasBuffer >= REFRESCO_SD) {guardadatos();}

velocidadm=(TWO_PI*radio*3.6*nvueltas)/(millis() - tiempo1);

lcd.setCursor(0,0);
if((millis()-tiempopaso) < 3000) {if (velocidad<100) {lcd.print(nf(velocidad,2,1));}}
else {lcd.print(" 0.0");}//velocidad instantanea 0 si en 3 segundos no da vueltas
//si la velocidad es > 100 no representa

lcd.setCursor(5,0);
if (velocidadm<100) {lcd.print(nf(velocidadm,2,1));}

lcd.setCursor(15,0);
if (botonpulsado==5) {lcd.print("G");}
else {lcd.print("N");}

//lcd.setCursor(14,0);
//lcd.print(marcha);

//lcd.setCursor(15,0);
//lcd.print(paro);

lcd.setCursor(10,0);
if (vueltas<=9) {
lcd.print("V:");
lcd.print(vueltas);}
else {lcd.print("FINAL");}

lcd.setCursor(0,1);
lcd.print(nf(distancia,3,2));

lcd.setCursor(7,1);
lcd.print(tiempo());

lcd.setCursor(14,1);
if(velocidad>100 || velocidadm>100 ){lcd.print("E");}
else{lcd.print(" ");}

marcha=0;
paro=0;

delay(200);
}
