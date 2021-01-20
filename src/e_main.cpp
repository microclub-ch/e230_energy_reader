/* 	e_main.cpp
	------------
	YM: 31.12.2020
  01.01.2021: add min stack
  12.01.2021: typos
  13.01.2021: process in case of error
  18.01.2021: use of unsigned ms()
  19.01.2021: clock is incremented in pstate active
*/
#ifndef MAIN
#define MAIN

#include <Arduino.h>
#include <Bridge.h>

#include "e230.h"
#include "e_menu.h"

bool time_to_store, store_done; // storage process control
bool print_val; // cmd 'p' active print command

// display an info for a short time, and not memorized
void log_info(const char * msg)
{
  Serial.print(F("#info:")); Serial.println(msg);
  tempo_msg = 10;
  display_at_ln(msg);
}


/* ----------------------------------------------------- */

void setup()
{
  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  pinMode(LED_J, OUTPUT);
  pinMode(LED_R, OUTPUT);  
  pinMode(LED_X, OUTPUT);

  LED_ON(LED_R);

	// init an array of button control
	sw[0] = new Sw(SW1);
	sw[1] = new Sw(SW2);

  min_stack = 9999;   //set higher enough
	menu = 0;           // clear menu setting
  ser_copy = 0;

  // I2C
  Wire.begin();

	// LCD
	lcd = new jm_LCM2004A_I2C();  // Add. 0X27 standard
	lcd->begin();  lcd->clear_display();
	lcd->print(F(__PROG__ "  " VERSION));  // on line 0
  display_at_ln("LCD init done", 1);     

	// we use serial for log messages
	Serial.begin(9600); 
	display_at_ln("Serial started", 2);

  // Bridge startup
  Bridge.begin();

  display_at_ln("Read time...", 3);
  strcpy(dateTimeStr, ("2021")); // assure century
  // init_linux_time_process();
  getTimeStamp(dateTimeStr+2, sizeof(dateTimeStr)-3);
  display_at_ln(dateTimeStr,0);

  delay(3000);
  
  FileSystem.begin();

	// set stream_in hardware...
#if defined AltSoftSerial_h
	STREAM_IN.begin(300);
#else
	STREAM_IN.begin(300, SERIAL_7E1);
#endif
  display_at_ln("Serial e230 started", 1); 

  p_e230 = new(E230_S);
  if (p_e230 != NULL)
    display_at_ln("e230 On", 2);

	// stabilize STREAM_IN and STREAM_OUT RX/TX lines after reset/power up...
	delay(3000);

	err_act = false;  // no error

  log_msg_SD( __PROG__ " " VERSION );

  time_to_store = false;

  menu = 0; // clear menu setting

  LED_OFF(LED_R);

} // end setup()

/* ----------------------------------------------------- */

/*  void serial_cmd()
    -----------------
    Get key onto serial
    Global var modified:
    - menu
    - ser_copy

    The menu number is modified as the hardware switch[+] or [-].
    Limits and display is managed by e_menu.cpp
    The ser_copy byte use 4 bits as flags to control witch line of display (0..3)
    should be copied onto serial.
*/
void serial_cmd()
{
  int c;
  c = Serial.read();
  if (c == -1)
    return;

  //Serial.println(c);
  menu_changed = true;

  switch(c)
  {
    case '+':
      menu++;
      ser_copy = LN_all;        
    break;

    case '-':
      menu--;
      ser_copy = LN_all;  
    break;

    case '0': // menu base - energie actuelle
      menu = 0;
      ser_copy = LN_all;      
    break;

    case '1': // condensé énergie cons - prod - U - I - P
      menu = 1;
      ser_copy = LN2+LN3;
    break;

    case '2': // U, I, P des 3 phases
      menu = 2;
      ser_copy = LN1+LN2+LN3;      
    break;

    case '3': // erreurs
      menu = 3;
      ser_copy = LN1+LN2+LN3;        
    break;

    case '4': // version
      ser_copy = LN1;      
      menu = 4;
    break;

    case '5': // dernier boot
      ser_copy = LN1+LN2+LN3;      
      menu = 4;
    break;

    case ' ':
      ser_copy = LN_all;
    break;

    case 'd':
      // Serial.println(F("Store datas"));
    break;

    case 'm':
      Serial.print(F("\nFree mem:")); Serial.println(freeMemory());
    break;

    case 'p':
      print_val = true;
      //data.print();
    break;

    case 's':
    Serial.print(F("\nStack min:")); Serial.println(min_stack);
    break;

    case 't':
    Serial.println(dateTimeStr);
    break;

    case 'x': // force termination
      pstate = getv;
    break;

    case '?':
    default:
      Serial.println(F("\ncmd: d(ate, p(rint, m(em, s(tack, menu 1..4."));
    break;
  }
} // serial_cmd()

/* ------------------------------------------*/

/*  poll_loop_X_ms()
    ----------------
    Compute the state of switches
    Modified var: intern of object Sw.
    The polling time must be between 10..50 ms
    Return value: -
*/
void poll_loop_X_ms()
{
  // scan all switches
  for(short i = 0; i < SW_NB; i++)
  {
    sw[i]->scan();
  }
    
  menu_select(); // follow user choice
  serial_cmd();

  // get the minimum aivable Ram space
 GET_MIN_STACK();

}


static unsigned char rec_count; // record time, for manage timeout

void poll_loop_1_s()
{
  if (!pstate)
  {
    getTimeStamp(dateTimeStr+2, sizeof(dateTimeStr)-3);
  }
  else
  {
    char *ps = dateTimeStr + TIME_S;  // point second digit
    *ps == '9' ? *ps = '0' : ++*(ps);   // increment it, if < 9
  }

  display_menu(); // rest of display depends of the menu choice

  LED_ON(LED_X);
  Serial.print(dateTimeStr);
  Serial.print(F(" pstate:")); Serial.println(pstate);
  LED_OFF(LED_X);

  // ask energy all 15 seconds
  if (IsSyncTime_x_seconds(15) && pstate == wait)
  {
    p_e230->begin();
    p_e230->start();
    rec_count = 0;
    Serial.println(F("*Start "));
    LED_OFF(LED_R);
    pstate = ask;
  }
  else if(!p_e230->ready() && pstate == ask)
  {
    Serial.println(F("*Ask"));
    pstate = rec;    
  }
  else if (pstate == rec )
  {
    Serial.print(p_e230->e_state()); Serial.print(" ! "); 
    Serial.print(p_e230->started()); Serial.print(" ! ");  

    if (rec_count++ > 6)  // receive does not take more 5 seconds
    {
      const  char msg_timeout[] = "*Read timeout!";
      log_info(msg_timeout);
      log_msg_SD(msg_timeout);
      err_timeout = true;
      rec_count = 0;
      pstate = wait;
      Serial.println("*E*");
    }
    else if (p_e230->errored())  // Q: driver encoutered an error?
    {                     // A: yes show it
      log_info("*Read error!");
      err_act = true; 
      pstate = wait;      // abort read process
    }
    else if (p_e230->ready())    // Q: reading done?
    {
      pstate = getv;      // A: yes, next
    }
      
  } // pstate rec
  else if (p_e230->started() == false && pstate == getv) // Q: time to get values?
  {                                                      // A: yes, get it
    Serial.println(F("*Get values"));
    get_all_values(p_e230->_buf);
    err_act = false;
    err_timeout = false;
    pstate = calc;
  }
  else if(pstate == calc) // Q: calculation done?
  {                       // A: yes, wait 
    Serial.println(F("*Wait"));
    pstate = wait;
  }
  else if(pstate == wait)         // Q: no transaction?
  {
    if (IsSyncTime_15_minutes())  // Q: time window to store data?
    {                             // A: Yes, set the marker
      if (!time_to_store)         // Q: not already done?
      {                           // A: yes, flag it
        Serial.println(F("*Store"));
        time_to_store = true;         
      }
    }
    else                  // A: not in window, reset marker
    {
      if (time_to_store)  // Q: flag active?
      {
        time_to_store = false; // A: yes, clear flag   
      }
    }

    if (print_val == true)
    {
      data.print();
      print_val = false;
    }
  }// psate == wait
  
  if (pstate)
    LED_ON(LED_J);
  else
    LED_OFF(LED_J);


  if (err_file | err_act | err_timeout  )
    LED_ON(LED_R);
  else
    LED_OFF(LED_R);
  
/* ----------------------------------------------------- */

  if (time_to_store) // Q: is time to store datas?
  {
    if (!store_done)  // Q: already done?
    {                 // A: no, store datas
      store_datas(fname); 
      data.energy_diff();    // compute the delta energy
      store_done = true;
    }
  }
  else
  { // out of window time for storage
    store_done = false;
  }

}

/* main loop of Arduino
*/
#define SHORT_CYCLE 20  // 20 ms
//#define MAIN_CYCLE 1000  // 1 sec.
#define MAIN_CYCLE 1000

void loop()
{
  static unsigned fastPreviusMs, mainPreviusMs;

  unsigned currentMs = ms();
  if (currentMs - fastPreviusMs > SHORT_CYCLE)
  {
    fastPreviusMs = currentMs;
    poll_loop_X_ms();
  }

  if (currentMs - mainPreviusMs > MAIN_CYCLE)
  {
    mainPreviusMs = currentMs;
    poll_loop_1_s();
  }  

  CYCLE();
  
}// loop()

#endif // MAIN
