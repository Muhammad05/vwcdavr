/*

  atmega328 only

  mega + serial console:

  next track button       =
  previous track button   -
  next CD                 ]
  previous CD             [
  play/stop               p
  CD1                     1
  CD2                     2
  CD3                     3
  CD4                     4
  CD5                     5
  CD6                     6
  seek forward            f
  seek rewind             r
  scan mode               s
  shuffle mode            l
  help                    h

  CDC sniffer
  - emulate RADIO DataOut signals
  - receive and print CD changer responce to serial console

  DataOut -> arduino pin 5
  Clk     -> arduino pin 3
  DataIn  -> arduino pin 4

*/
#define DataOut 5
#define Clk 3
#define DataIn 4
#define ClkInt 1
#define V12 8 // do not forget resistor devider!!!
#define ACC 7 // do not forget resistor devider!!!


#define CDC_END_CMD 0x14
#define CDC_END_CMD2 0x38
#define CDC_PLAY 0xE4
#define CDC_STOP 0x10 //go to RADIO MODE
#define CDC_NEXT 0xF8
#define CDC_PREV 0x78
#define CDC_SEEK_FWD 0xD8
#define CDC_SEEK_RWD 0x58
#define CDC_CD1 0x0C
#define CDC_CD2 0x8C
#define CDC_CD3 0x4C
#define CDC_CD4 0xCC
#define CDC_CD5 0x2C
#define CDC_CD6 0xAC
#define CDC_CD7 0x6C
#define CDC_CD8 0xEC
#define CDC_CD9 0x1C
#define CDC_SCAN 0xA0
#define CDC_SFL 0x60
#define CDC_PLAY_NORMAL 0x08
#define CDC_PREV_CD 0x18
#define CDC_RANDOM6CD 0xE0
#define CDC_POWERON 0xA4
#define CDC_CDCHANGE 0x01
#define CDC_TP 0x30


//#include <LiquidCrystal.h>
//
////lcd pins
//LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
//
//int adc_key_val[5] ={
//  30, 150, 360, 535, 760 };
//int NUM_KEYS = 5;
//int adc_key_in;
//int key=-1;
//int oldkey=-1;
//int cd[6]={
//  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC};
int cd = 0;
//int cdpointer=0;
int play = 0;
int tr = 0;
int minutes = 0;
int sec = 0;
int mode = 0;
int ack = 0;
int ack_cd = 0;
volatile uint64_t cmd = 0;
volatile uint64_t prev_cmd = 0;
volatile int cmdbit = 0;
volatile uint8_t newcmd = 0;
int incomingByte = 0;

uint8_t V12_ON=0;
uint8_t ACC_ON=0;

int verbose = 1;

#define TX_BUFFER_END  12
uint16_t txbuffer[TX_BUFFER_END];
uint8_t txinptr = 0;
uint8_t txoutptr = 0;
static void Enqueue(uint16_t num);

void setup()
{

  //time to catche start of transmition form emulator
  cli();//stop interrupts
  //for atmegax8 micros
  //TODO: cpu detection ...
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCCR1C = 0;
  TCNT1  = 0;//initialize counter value to 0;
  TIMSK1 |= _BV(OCIE1A);
  OCR1A = 700;//700*64us=44,8ms
  //OCR1A = 778;//50048us=50,048ms
  TCCR1B |= _BV(CS12) | _BV(CS10); //prescaler 1024*1/16000000 -> 64us tick
  sei();//allow interrupts
  //  TIFR1 |= _BV(TOV1);//clear overflow flag
  Serial.begin(9600);
  Serial.println("vw group radio emulator");
  pinMode(DataOut, OUTPUT);
  digitalWrite(DataOut, LOW);
  pinMode(Clk, INPUT);
  pinMode(DataIn, INPUT);
//  pinMode(V12, INPUT);
//  pinMode(ACC, INPUT);
  
  //  lcd.begin(16,2);
  //  lcd.home();
  //  lcd.clear();
  //  lcd.print("cd ");
  //  lcd.print(cdpointer+1);
  //  lcd.print(" pause");
  //  lcd.setCursor(0, 1);
  //  lcd.print("tr ");
  //  lcd.print(tr);

  attachInterrupt(ClkInt, readDataIn, FALLING);
}

void loop() {
//  if (V12_ON != digitalRead(V12)){
//    V12_ON = digitalRead(V12);
//    if(V12_ON) {
//      Serial.println("12V ON");
//    } else {
//      Serial.println("12V OFF");
//    }
//  }
//
//  if (ACC_ON != digitalRead(ACC)){
//    ACC_ON = digitalRead(ACC);
//    if(ACC_ON) {
//      Serial.println("ACC ON");
//    } else {
//      Serial.println("ACC OFF");
//    }
//  }

  //    delay(50);          // wait for debounce time
  //    adc_key_in = analogRead(0);    // read the value from the sensor
  //    key = get_key(adc_key_in);                  // convert into key press
  //    if (key != oldkey)
  if (Serial.available() > 0)
  {
    // read the incoming byte:
    incomingByte = Serial.read();
    //     oldkey = key;
    switch (incomingByte)
    {
      case '=':
        Serial.println("next track");
        // NEXT SONG
        send_cmd(CDC_NEXT);
        send_cmd(CDC_END_CMD);
        break;
      case ']':
        Serial.println("next cd");
        // NEXT CD
        send_cmd(CDC_END_CMD2);
        send_cmd(CDC_END_CMD);
        break;
      case '[':
        Serial.println("previous cd");
        // PREVIOUS CD
        send_cmd(CDC_PREV_CD);
        send_cmd(CDC_END_CMD);
        break;
      case '-':
        Serial.println("previous track");
        // PREVIOUS SONG
        send_cmd(CDC_PREV);
        send_cmd(CDC_END_CMD);
        break;
      case 'p':
        // PLAY/PAUSE
        play = !play;
        if (play) {
          Serial.println("play");
          send_cmd(CDC_PLAY);
          send_cmd(CDC_END_CMD);
        }
        else
        {
          Serial.println("pause");
          send_cmd(CDC_STOP);
          send_cmd(CDC_END_CMD);
        }
        break;
      case '1':
        Serial.println("cd1");
        // CD 1
        send_cmd(CDC_CD1);
        send_cmd(CDC_END_CMD2);
        break;
      case '2':
        Serial.println("cd2");
        // CD 2
        send_cmd(CDC_CD2);
        send_cmd(CDC_END_CMD2);
        break;
      case '3':
        Serial.println("cd3");
        // CD 3
        send_cmd(CDC_CD3);
        send_cmd(CDC_END_CMD2);
        break;
      case '4':
        Serial.println("cd4");
        // CD 4
        send_cmd(CDC_CD4);
        send_cmd(CDC_END_CMD2);
        break;
      case '5':
        Serial.println("cd5");
        // CD 5
        send_cmd(CDC_CD5);
        send_cmd(CDC_END_CMD2);
        break;
      case '6':
        Serial.println("cd6");
        // CD 6
        send_cmd(CDC_CD6);
        send_cmd(CDC_END_CMD2);
        break;
      case '7':
        Serial.println("cd7");
        // CD 7
        send_cmd(CDC_CD7);
        send_cmd(CDC_END_CMD2);
        break;
      case '8':
        Serial.println("cd8");
        // CD 8
        send_cmd(CDC_CD8);
        send_cmd(CDC_END_CMD2);
        break;
      case '9':
        Serial.println("cd9");
        // CD 9
        send_cmd(CDC_CD9);
        send_cmd(CDC_END_CMD2);
        break;
      // seek forward            f
      // seek rewind             r
      // scan mode               s
      // shuffle mode            h
      case 'f':
        Serial.println("seek forward");
        //  seek forward
        send_cmd(CDC_SEEK_FWD);
        play = 0;
        //send_cmd(CDC_PLAY);
        //send_cmd(CDC_END_CMD2);
        break;
      case 'r':
        Serial.println("seek rewind");
        // seek rewind
        send_cmd(CDC_SEEK_RWD);
        play = 0;
        //send_cmd(CDC_PLAY);
        //send_cmd(CDC_END_CMD2);
        break;
      case 's':
        Serial.println("togle scan");
        // scan
        send_cmd(CDC_SCAN);
        break;
      case 'l':
        Serial.println("toogle shuffle 1 cd");
        // shuffle
        send_cmd(CDC_SFL);
        break;
      case 'k':
        Serial.println("toogle shuffle all cd");
        // shuffle 6cd
        send_cmd(CDC_RANDOM6CD);
        break;
      case 'v': //verbose
        Serial.println("change verbose output");
        verbose = !verbose;
        break;
      case 'c':
        Serial.println("change cd");
        send_cmd(CDC_CDCHANGE);
        send_cmd(CDC_END_CMD);
        break;
      case 'o':
        Serial.println("power on");
        send_cmd(CDC_POWERON);
        send_cmd(CDC_END_CMD);
        break;
      case 't':
        Serial.println("TP-INFO");
        send_cmd(CDC_TP);
        //send_cmd(CDC_END_CMD);
        play = 0;
        //send_cmd(CDC_END_CMD);
        break;
      case 'x':
        Serial.println("random data send to changes");
        send_cmd(0x40);
        break;
      case 'y':
        Serial.println("CDC_PLAY_NORMAL");
        send_cmd(CDC_PLAY_NORMAL);
        break;
      case 'h': //help
        Serial.println("power on                o");
        Serial.println("change cd               c");
        Serial.println("next track button       =");
        Serial.println("previous track button   -");
        Serial.println("next CD                 ]");
        Serial.println("previous CD             [");
        Serial.println("play/stop               p");
        Serial.println("CD1                     1");
        Serial.println("CD2                     2");
        Serial.println("CD3                     3");
        Serial.println("CD4                     4");
        Serial.println("CD5                     5");
        Serial.println("CD6                     6");
        Serial.println("CD7                     7");
        Serial.println("CD6                     8");
        Serial.println("CD9                     9");
        Serial.println("TP                      t");
        Serial.println("seek forward            f");
        Serial.println("seek rewind             r");
        Serial.println("scan mode               s");
        Serial.println("shuffle 1cd             l");
        Serial.println("shuffle 6cd             k");
        Serial.println("help                    h");
    }
    //    lcd.home();
    //    lcd.clear();
    //    if (play){
    //      lcd.print("cd ");
    //      lcd.print(cdpointer+1);
    //      lcd.print(" play");
    //    }
    //    else
    //    {
    //      lcd.print("cd ");
    //      lcd.print(cdpointer+1);
    //      lcd.print(" pause");
    //    }
    //    lcd.setCursor(0, 1);
    //    lcd.print("tr ");
    //    lcd.print(tr);
  }

  if (newcmd && prev_cmd != cmd && cmd != 0)
  {
    prev_cmd = cmd;
    newcmd = 0;
    // TIMSK1 |= _BV(OCIE1A);
    //    for(int b=56;b>-1;b=b-8){
    //      uint8_t temp=((cmd>>b) & 0xFF);
    //      Serial.print(temp,HEX);
    ////      Serial.print(" ");
    //    }
    uint8_t temp;
    if (verbose) {
      temp = ((cmd >> 56) & 0xFF);
      Serial.print(temp, HEX);
      Serial.print(" ");
      temp = ((cmd >> 48) & 0xFF);
      Serial.print(temp, HEX);
      Serial.print(" ");
      cd = temp;
      temp = ((cmd >> 40) & 0xFF);
      Serial.print(temp, HEX);
      Serial.print(" ");
      tr = temp;
      temp = ((cmd >> 32) & 0xFF);
      Serial.print(temp, HEX);
      Serial.print(" ");
      minutes = temp;
      temp = ((cmd >> 24) & 0xFF);
      Serial.print(temp, HEX);
      Serial.print(" ");
      sec = temp;
      temp = ((cmd >> 16) & 0xFF);
      Serial.print(temp, HEX);
      Serial.print(" ");
      mode = temp;
      temp = ((cmd >> 8) & 0xFF);
      Serial.print(temp, HEX);
      Serial.print(" ");
      temp = (cmd & 0xFF);
      Serial.print(temp, HEX);
      Serial.print("               ");
      Serial.print("   CD: ");
      Serial.print(cd ^ 0xBF, HEX);
      Serial.print(" track: ");
      Serial.print(tr ^ 0xFF, HEX);
      Serial.print("   min: ");
      Serial.print(minutes ^ 0xFF, HEX  );
      Serial.print(" sek: ");
      Serial.print(sec ^ 0xFF, HEX);
      Serial.print(" mode: ");
      Serial.println(mode, HEX);
    }
    else
    {
      Serial.print("   CD: ");
      Serial.print(int (((cmd >> 48) & 0xFF) ^ 0xBF), HEX);
      Serial.print(" track: ");
      Serial.print(int (((cmd >> 40) & 0xFF) ^ 0xFF), HEX);
      Serial.print("   min: ");
      Serial.print(int (((cmd >> 32) & 0xFF) ^ 0xFF), HEX);
      Serial.print(" sek: ");
      Serial.print(int (((cmd >> 24) & 0xFF) ^ 0xFF), HEX);
      Serial.print(" mode: ");
      Serial.print(int (((cmd >> 16) & 0xFF)), HEX);
      Serial.println();
    }

  }

  if (newcmd && prev_cmd == cmd)
  {
    newcmd = 0;
  }

  if (newcmd && cmd == 0)
  {
    newcmd = 0;
  }

  while (txoutptr != txinptr)

  {

    Serial.println(txbuffer[txoutptr]);


    txoutptr++;

    if (txoutptr == TX_BUFFER_END)

    {
      txoutptr = 0;

    }

  }
  // Serial.println("loop");

}

//no signal on ISP clock line for more then 45ms => next change is first bit of packet ...
ISR(TIMER1_COMPA_vect) {
  //Enqueue(3);
  cmdbit = 0;
  newcmd = 0;
  cmd = 0;


}

// Convert ADC value to key number
//int get_key(unsigned int input)
//{
//  int k;
//  for (k = 0; k < NUM_KEYS; k++)
//  {
//    if (input < adc_key_val[k])
//    {
//      return k;
//    }
//  }
//  if (k >= NUM_KEYS)
//    k = -1;     // No valid key pressed
//  return k;
//}

void shiftOutPulse(uint8_t dataPin, uint8_t val)
{
  uint8_t i;
  for (i = 0; i < 8; i++)  {
    digitalWrite(dataPin, HIGH);
    delayMicroseconds(550);
    digitalWrite(dataPin, LOW);
    if (!!(val & (1 << (7 - i))) == 1)
    { // logic 1 = 1700us
      delayMicroseconds(1650);
    }
    else
    {
      delayMicroseconds(540);
    }
  }
  digitalWrite(dataPin, HIGH);
  //  delay(1);
}

void send_cmd(uint8_t cmd)
{
  digitalWrite(DataOut, LOW);
  //  delay(1);
  digitalWrite(DataOut, HIGH);
  delay(9); //9000us
  digitalWrite(DataOut, LOW);
  delay(4);
  delayMicroseconds(500); //4500us :)
  shiftOutPulse(DataOut, 0x53);
  shiftOutPulse(DataOut, 0x2C);
  shiftOutPulse(DataOut, cmd);
  shiftOutPulse(DataOut, 0xFF ^ cmd);
  digitalWrite(DataOut, HIGH);
  delayMicroseconds(550);
  digitalWrite(DataOut, LOW);
  //  delayMicroseconds(550);
  //  digitalWrite(DataOut,HIGH);
  Serial.println("---------------------------------------------");
  Serial.print(0x53, HEX);
  Serial.print(0x2C, HEX);
  Serial.print(cmd, HEX);
  Serial.println(0xFF ^ cmd, HEX);
  Serial.println("---------------------------------------------");
}

void readDataIn()
{

  //if (!digitalRead(Clk)){

  TIMSK1 &= ~_BV(OCIE1A);
  TCNT1 = 0; //disable and reset counter while we recieving data ...
  if (!newcmd)
  {

    if (digitalRead(DataIn))
    { //1
      cmd = (cmd << 1) | 1;
    }
    else
    { //0
      cmd = (cmd << 1);
    }
    cmdbit++;
    TIMSK1 |= _BV(OCIE1A); //enable counter
  }

  //Enqueue(cmdbit);
  if (cmdbit == 64)
  {
    newcmd = 1;
    cmdbit = 0;
  }
  //}

}


static void Enqueue(uint16_t num)

{

  txbuffer[txinptr] = num;

  txinptr++;

  if (txinptr == TX_BUFFER_END)

  {

    txinptr = 0;

  }

}
