/*
  e_menu.cpp
  ---------
  20.11.2020 - ymasur@microclub.ch
*/
#ifndef _MENU_CPP
#define _MENU_CPP

#include <Arduino.h>
#include "e230.h"
#include "e_menu.h"

extern bool print_log;

/*
    menu_select()
    -------------
    Follow the menu number to display informations.
    The menu can be modified by push button or via serial command.
*/
void menu_select()
{

  if (sw[B_PLUS]->getActivated()) 
  {
    menu++; // 0..N_MENUS menus
    if (menu > NB_MENUS) menu = 0;

    menu_changed = true;
  }
   
  if (sw[B_MINUS]->getActivated()) 
  {
    menu--; // 0..N_MENUS menus
    if (menu < 0 ) menu = NB_MENUS;

    menu_changed = true;
  }

  if (menu == 0 && menu_changed)
  {
    tempo_msg = 0;
  }
   
  /*
  if (menu == 1) // --- MENU 1 ---
  {

  };

  if (menu == 2) // --- MENU 2
  {
    
  }; 

  if (menu == 3) // --- MENU 3
  {
    
  };
  */
} // fct menu_select() ;

// --- DISPLAY PART ----------------------------------------------------------

/*  display_at_ln(String info, u8 ln)
    ---------------------------------
    4 lines (0..3) of the display can show 20 char of information.
    Because the display is not consistent, we must adress lines specifically.
    In order to reproduce the same cotent to the serial, a mecanism is set to avoid
    the refresh onto it.
    Command to the serial set a bit to specify wich line cotent is to be copied.
    After the copy, the bit is reset.

    Because of frequent call and timing to receive e230 datas, this function made the 
    cycle() call between operations.

    Parameters:
    - info: strig to be displayed
    - ln: line number of the LCD (0..3)
    Vars used/modified:
    - ser_copy: bit of the line to be copied onto serial
*/
void display_at_ln(const char * info, u8 ln)
{
  char buf[20+1];

	snprintf(buf, sizeof(buf), "%-20s", info);  
   byte ser_copy_bit = 1 << ln;  //get the copy bit flag for Serial

  lcd->set_cursor(0, ln); // column, line
  CYCLE();
  lcd->print(buf);
  CYCLE();

  if (ser_copy_bit && ser_copy && print_log)  //Q: is info to be copied onto serial?
  {
    Serial.println(); Serial.print(info);    //A: yes, print it
    ser_copy &= ~ser_copy_bit;    // remove flag
    CYCLE();
  }

  GET_MIN_STACK();
}

/*  display_clock(void)
    ------------------
    Write the value of date & time on line 0 of LCD
    Global var modified:
    - bootTime, using to get values of RTC
    - dateTimeStr, char array sets with the new value
*/
void display_clock()
{
  display_at_ln(dateTimeStr, 0);
}

/*  LCD display info (standard: line 1)
    Parameters:
    - info: string to be displayed (max 20 chrs...)
    - tempo: time duration of the info, in seconds

    Global vars used/modified:
    - tempo_msg: set to the duration wanted
*/
void display_info(const char * info, u8 tempo)
{
  tempo_msg = tempo;
  display_at_ln(info, 1); 
}

/* msg_manage()
  -------------
  Management of messages.
  - Info msg are displayed x seconds
  - Error msg are diplayed continuously and stored
  Var used:
  - tempo_msg;  value of info displayed
  - err_msg[21]; storage of last error message (if active)
*/
void msg_manager()
{
  if (tempo_msg)  // Q: is a waiting message displayed?
  {
    tempo_msg--;
    return;
  }
  
  if (err_act)
  {
    display_at_ln(last_log);
  }
  else
  {            
    display_at_ln("Marche normale");
  }
}

/*  Menu display part
    -----------------
    The menus are displayed continously
*/
// menu 0 - energy()
void display_energy()
{
  char buf[21];
 // dtostrf(VAR, UNIT, DECIM, BUF)
  //           01234567890
  strcpy(buf, "Conso kWh ");
  dtostrf(data.e_consumed.val, 10, 3, buf+10);
  display_at_ln(buf, 2);

  if (pstate)	  p_e230->cycle();

  strcpy(buf, "Prod  kWh ");
  dtostrf(data.e_producted.val, 10, 3, buf+10);
  display_at_ln(buf, 3);
}

// menu 1 - short data
void display_datas()
{
  char buf[21];
                // 01234567890123456789
  display_at_ln("P in / out  U-- I---", 2); if (pstate)	  p_e230->cycle();
  
  dtostrf(data.p_consumed, 5,2, buf);     buf[5] = ' '; // replace the '\0'
  dtostrf(data.p_producted, 5,2, buf+6);  buf[11] = ' ';
  dtostrf(data.u, 3, 0, buf+12);          buf[15] = ' ';
  dtostrf(data.i, 4, 2, buf+16);

  display_at_ln(buf, 3);

}

// menu 2 - values of 3 ph
void display_ph_datas()
{
  char buf[21];

  //                      01234567890123456789
  const char pattern[] = "230V 00.00A  00.00kW";
  strcpy(buf, pattern);
 
  for (int i = 0; i < 3; i++)   // for 3 phases
  {
    S_V *pval; // point on the array of values

    pval = data.values[i];
    dtostrf((pval)->val, 3, 0, buf);  // set voltage val
    *(buf+3) = 'V'; // replace the '\0'

    pval = data.values[i+3];      
    dtostrf((pval)->val, 5, 2, buf+5);
    *(buf+10) = 'A';

    pval = data.values[i+6]; 
    dtostrf((pval)->val, 5, 2, buf+13);
    *(buf+18) = 'k';

    display_at_ln(buf, 1+i);
  } 
}

/* menu 3 - erreurs
*/
void display_err()
{
  char buf[21];

            //  012345678901234567890
  sprintf(buf, "file:      "); buf[5] = err_file ? '1':'0'; 
              //   01234567890
  sprintf(buf+10, "FATAL: "); buf[16] = err_fatal ? '1':'0'; 
  display_at_ln(buf, 1);

            //  012345678901234567890
  sprintf(buf, "Timeout:   "); buf[8] = err_timeout ? '1':'0';
               //  012345678901234567890
  sprintf(buf+10, "Log: " ); buf[14] = print_log ?  '1':'0';
  display_at_ln(buf, 2);

  sprintf(buf, "CRC: "); buf[4] = err_act ? '1':'0';
  display_at_ln(buf, 3);

}

/* menu 4 - boot, soft version
  Convert and shows the date/time of boot
*/
void display_last_boot_vers()
{
  tempo_msg = 1;
  display_at_ln(("Version:" VERSION), 1);  
  display_at_ln(last_log, 2);
}

/*  display_menu()
    --------------
    Called periodically at 1 sec.
    Display menu part, according to state of menu
    Then, shows satus

    Globals vars used:
    - menu: main menu level
    Vars modified:
    - nothing
    Return: -
 */
void display_menu()
{
  // char ln_menu[LN_LCD_LENGHT+1]; // used to prepare a line written to the LCD

  lcd->set_cursor(0, 2); // we use lines 3 and 4
      
  if (menu_changed)
  {
    menu_changed = false;
    err_act = false;  // manually clear the log error flag
    if (print_log)
    { 
      Serial.print(F("\n*menu: "));  
      Serial.println((int)menu); 
    }
    
  }
  
  switch(menu)
  {
    /* ---------------- BASE MENU -------------- */
    case 0:
    display_clock();  // alway displayed on line 0
    msg_manager();  // message to be displayed
    display_energy();  // energy
    break; // base menu

    /* -------------- MENU 1 ------------- */
    case 1: // short view of datas
    display_clock();
    msg_manager();  // message to be displayed
    display_datas(); ; 
    break; // menu 1

    /* -------------- MENU 2 ------------- */
    case 2: // U - I - P of 3 phases
    display_clock();
    display_ph_datas();   
    break; // menu 2

    /* -------------- MENU 3 ------------- */
    case 3: // show error status
    display_clock();
    display_err();
    break;

    /* -------------- MENU 4 ------------- */
    case 4: // last log
    display_clock();  
    display_last_boot_vers();
    break;  // menu 4

    default:  // nothing to do
    break;
  } // switch(menu)

} // fct display_menu()

#endif