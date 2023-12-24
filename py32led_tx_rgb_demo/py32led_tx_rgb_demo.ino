// PYLED TX001 V1.0
// RGB & switch Demo
// Github/OSHWHub: @libc0607

// Theory: 
// The transmitter performs a 0%/100% ASK modulation on the ~220kHz carrier
// to transform both RGB data frame and power. On the receiver side, a comparator 
// with a configurable digital filter is used to demodulate data from carrier. 
// Since the receiver needs a stable power supply, the 0% part of any symbol 
// should never be too long, and the overall duty should be large enough.
// So, I decided to encode the data in different time length between 
// a sequence of constant and short 0% ASK pulse. 
// On the receiver side, we only need to set up an interrupt, log a timestamp 
// in each interrupt, and calculate the duration between them.
//
// packet format 1
// 001       aaaaa    rrrrrgggggbbbbb  p
// preamble  address  RGB444           parity

// Pin definition 
#define PIN_TXMOD   PA_0
#define PIN_TXPWREN PA_1
#define PIN_U1TX    PA_2
#define PIN_U1RX    PA_3
#define PIN_FANEN   PA_4
#define PIN_LED     PA_5
#define PIN_KEY     PA_6
#define PIN_VBUS_A  PA_7
#define PIN_SW2     PB_0
#define PIN_TEMP_A  PB_1
#define PIN_SW1     PB_2

// pulse 
#define PULSE_W_US  100
#define PULSE_0_US  150
#define PULSE_1_US  400
#define FRAME_LEN   24

uint32_t status;

// cmd: set address 
void tx_addr(uint8_t cur_addr, uint8_t new_addr)
{
  uint32_t d, i, s = 0;

  d = 0x00600000; // 00000000 011 00000 00000 00000 00000 0
  d |= ((cur_addr & 0x1f)<<16);
  d |= ((new_addr & 0x1f)<<11);
  d |= (((~new_addr) & 0x1f)<<6);
  d |= ((new_addr & 0x1f)<<1);
  for (i=0; i<FRAME_LEN; i++) 
    s += ((d & (0x1<<i))? 1: 0);
  d |= (s & 0x1);

  Serial.printf("TX new addr: %x\n", d);
  for (i=0; i<FRAME_LEN; i++) {
    digitalWrite(PIN_TXMOD, LOW);
    delayMicroseconds(PULSE_W_US);
    digitalWrite(PIN_TXMOD, HIGH);
    delayMicroseconds((d&(0x1<<(FRAME_LEN-1-i)))? PULSE_1_US: PULSE_0_US);
  }
  // ending 
  digitalWrite(PIN_TXMOD, LOW);
  delayMicroseconds(PULSE_W_US);
  digitalWrite(PIN_TXMOD, HIGH);
}

// cmd: set color
// addr: 0~31, rgb: 0~31 each 
void tx_frame(uint8_t addr, uint8_t r, uint8_t g, uint8_t b)
{
  uint32_t d, i, s = 0;

  d = 0x00200000; // 00000000 001 00000 00000 00000 00000 0
  d |= ((addr & 0x1f)<<16);
  d |= ((r & 0x1f)<<11);
  d |= ((g & 0x1f)<<6);
  d |= ((b & 0x1f)<<1);
  for (i=0; i<FRAME_LEN; i++) 
    s += ((d & (0x1<<i))? 1: 0);
  d |= (s & 0x1);

  Serial.printf("TX data: %x\n", d);

  for (i=0; i<FRAME_LEN; i++) {
    digitalWrite(PIN_TXMOD, LOW);
    delayMicroseconds(PULSE_W_US);
    digitalWrite(PIN_TXMOD, HIGH);
    delayMicroseconds((d&(0x1<<(FRAME_LEN-1-i)))? PULSE_1_US: PULSE_0_US);
  }
  // ending 
  digitalWrite(PIN_TXMOD, LOW);
  delayMicroseconds(PULSE_W_US);
  digitalWrite(PIN_TXMOD, HIGH);

  delay(5);
  
  // repeat the command 
  // To-do: add some 'wakeup' command to address this issue?
  for (i=0; i<FRAME_LEN; i++) {
    digitalWrite(PIN_TXMOD, LOW);
    delayMicroseconds(PULSE_W_US);
    digitalWrite(PIN_TXMOD, HIGH);
    delayMicroseconds((d&(0x1<<(FRAME_LEN-1-i)))? PULSE_1_US: PULSE_0_US);
  }
  // ending pulse
  digitalWrite(PIN_TXMOD, LOW);
  delayMicroseconds(PULSE_W_US);
  digitalWrite(PIN_TXMOD, HIGH);

  delay(5);
}

void setup() {
  pinMode(PIN_TXMOD, OUTPUT);
  pinMode(PIN_TXPWREN, OUTPUT);
  pinMode(PIN_U1TX, OUTPUT);
  pinMode(PIN_U1RX, INPUT_PULLUP);
  pinMode(PIN_FANEN, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_KEY, INPUT_PULLUP);
  pinMode(PIN_VBUS_A, INPUT);
  pinMode(PIN_SW2, INPUT_PULLUP);
  pinMode(PIN_TEMP_A, INPUT);
  pinMode(PIN_SW1, INPUT_PULLUP);
  Serial.begin(115200);
  digitalWrite(PIN_TXPWREN, HIGH);
  digitalWrite(PIN_TXMOD, HIGH);
  delay(100);
  status = 0;
  
  // example 1: set all LEDs at address 0x05 RED, delay 1s, then set a new address 0x01
  //tx_frame(0x05, 0x1f, 0x00, 0x00);
  //delay(1000);
  //tx_addr(0x05, 0x01);
  //delay(100);
  
  // example 2: set all LEDs (at broadcast addr) BLUE, delay 1s, then set LEDs at address 0x05 GREEN
  //tx_frame(0x1f, 0x00, 0x00, 0x1f);
  //delay(1000);
  //tx_frame(0x05, 0x00, 0x1f, 0x00);
  //delay(100);
  
  // example 3: set all LEDs (at broadcast addr) a new addr 0x10
  //tx_addr(0x1F, 0x10);
  //delay(100);
  
}


void loop() {

  if (digitalRead(PIN_KEY) == LOW) {
    delay(20);
    if (digitalRead(PIN_KEY) == LOW) {
      while (digitalRead(PIN_KEY) == LOW); 
      status++;
      if (status == 4) {
        status = 0;
      }
      switch (status) {
        case 0:
          tx_frame(0x1f, 0x1f, 0x1f, 0x1f);
        break;
        case 1: 
          tx_frame(0x01, 0x1f, 0x00, 0x00);
          tx_frame(0x02, 0x00, 0x1f, 0x00);
          tx_frame(0x03, 0x00, 0x00, 0x1f);
        break;
        case 2:
          tx_frame(0x02, 0x1f, 0x00, 0x00);
          tx_frame(0x03, 0x00, 0x1f, 0x00);
          tx_frame(0x01, 0x00, 0x00, 0x1f);
        break;
        case 3:
          tx_frame(0x03, 0x1f, 0x00, 0x00);
          tx_frame(0x01, 0x00, 0x1f, 0x00);
          tx_frame(0x02, 0x00, 0x00, 0x1f);
        break;
        default: break;
      }
    }
  }
}
