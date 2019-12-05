SoftwareSerial ELM(2, 3); // RX, TX;  ELM will be serial name

void setup()
{
  // enable debugging
//  #define _DEBUG

  len= 8;  //length set at 8 for all messages sent
  extended = 0x0;  //set as not extended format for all messages sent

  // Can bus speeds  (E46 is 500 Kbps)
  // 500 Kbps       = MCP2515_SPEED_500000,
  CAN_begin(MCP2515_SPEED_500000);  // set can baud rate
  CAN_setMode (CAN_MODE_NORMAL); // set can mode

  //set up serial port for debugging with data to PC screen
  Serial.begin(115200);  

  // start serial communication at the adapter defined baudrate
  ELM.begin(38400);  //ELM to Arduino start

  Init();  // initiate ELM327 connection to car

  del = 3; // minimum delay time between sends for Can bus messages (limited by program speed obviously)
  // if using "delay" command it is milliseconds
  //time_PID_o = millis();
  Serial.println("begin main void");
  IGN_status = 1;
  time_CAN_in = millis();
}

void loop()
{
  //The main loop calls subroutines to send the actual messages

  //the commented out section used for Debugging
  //***************************RPM print***************** 
  //  Serial.print(time_PID);
  //  Serial.print(" ");
  //  Serial.println(RPM);
  //****End RPM print ****************************************   

  //***************************MPG Print*****************  
  //  Serial.print(timeMPG);
  //  Serial.print(" ");
  //  Serial.println(RPM);
  //****End MPG print **************************************** 

  //  send_fuel();//ARBID: 545 100ms-400ms
  //  send_temp(); //ARBID: 329 20ms-30ms
  //  send_RPM(); //ARBID: 316, 40ms-60ms

if(IGN_status >0)
{
  if (PID_flag == 0) PID_send(); //send PID for RPM at regular intervals  &&  millis() - time_PID > 50
   if(millis() - time_test > 150) set_testvalues(); //after x second get new values. //debug
}
  if (PID_flag > 0) PID_rec(); //check for message ELM if a PID has been sent (flag > 0).  Use timer or other means instead of flag?

  if (millis() - time_CAN > del) CAN_bus_send(); //send CAN bus messages at regular intervals

  if (CAN_available()) //check to see if MCP2551 has a message
  {
    CAN_getMessage();  //Read the mesage
    if(id == 0x613) CAN_ProcessMessage(); //if ARBID is 615 (from IKE) process message
  } //end if CAN available
if ((millis() - time_CAN_in) > 1000) // if time since last IKE can message was > 1 second
{
 Serial.println("shut down");
 IGN_status = 0;
 sleepNow();     // sleep function called here
}

}// End of Main Void

//***************************************************************************
void CAN_bus_send() // Send CAN bus messages at regular intervals
{//rotates through 3 different messages.  Sends the RPM twice (every other one)

  if(CAN_flag == 0 || CAN_flag == 2) send_RPM(); //ARBID: 316
  if(CAN_flag == 1) send_temp(); //ARBID: 329 20ms
  //if(CAN_flag == 2) send_RPM(); //ARBID: 316
  if(CAN_flag == 3) send_fuel(); //ARBID: 545

  CAN_flag ++; //increment flag
  if(CAN_flag > 3) CAN_flag = 0;  //if flag exceeds max reset to 0 (should this be zero or 1)?
  time_CAN = millis(); //reset timer for RPM send
}

//***************************************************************************
void PID_send() // ELM_send("01 XX 1\r");  //Sends pid to ELM XX being the PID
{
  if(PID_flag_o == 0) ELM_send("01 0C 1\r"); //RPM send PID to ELM
  if(PID_flag_o == 1) ELM_send("01 05 1\r");  //Sends pid ENGINE_COOLANT_TEMP
  if(PID_flag_o == 2) ELM_send("01 0C 1\r"); //RPM send PID to ELM
  if(PID_flag_o == 3) ELM_send("01 10 1\r");  //Sends pid ENGINE_maf
  if(PID_flag_o == 4) ELM_send("01 0C 1\r"); //RPM send PID to ELM
  if(PID_flag_o == 5) ELM_send("01 01 1\r");  //Sends pid ENGINE_MIL status
  if(PID_flag_o == 6) ELM_send("01 0C 1\r"); //RPM send PID to ELM
  if(PID_flag_o == 7) ELM_send("01 11 1\r");  //Sends pid ENGINE_TPS status 


  PID_flag_o ++; //Pid >1 set PID flag so program will listen for response.  Increment do send next message
  if(PID_flag_o > 7) PID_flag_o = 0; //if flag exceeds max reset to 0.  back to start
  PID_flag = 1; //set flag to know to wait for ELM response
  time_PID = millis(); //reset timer for RPM send
}
/*
void PID_send_others() // ELM_send("01 0C 1\r");  //Sends pid to ELM 
 {
 if(PID_flag_o == 0) ELM_send("01 05 1\r");  //Sends pid ENGINE_COOLANT_TEMP
 if(PID_flag_o == 1) ELM_send("01 10 1\r");  //Sends pid ENGINE_maf
 if(PID_flag_o == 2) ELM_send("01 01 1\r");  //Sends pid ENGINE_MIL status
 if(PID_flag_o == 3) ELM_send("01 11 1\r");  //Sends pid ENGINE_TPS status
 
 PID_flag = 1; //set PID flag so program will listen for response
 PID_flag_o ++; //increment flag
 if(PID_flag_o > 3) PID_flag_o = 0;  //if flag exceeds max reset to 0
 time_PID_o = millis(); //reset timer for RPM send
 }*/

//***************************************************************************
void send_RPM() //ARBID 316, RPM
{
  id = 0x316;  //From ECU
  data[0]= 0x05;
  data[1]= 0x62;
  data[2]= rLSB;  //RPM LSB0xFF
  data[3]= rMSB; //RPM MSB "0x0F" = 600 RPM0x4F
  data[4]= 0x65;
  data[5]= 0x12;
  data[6]= 0x0;
  data[7]= 0x62;
  CAN_send();   
}//end RPM Loop  

void send_temp() //ARBID 329,  Temperature, Throttle position
{ 
  //********************temp sense  *******************************
  //  The sensor and gauge are not linear. 
  //  data[1] values and correlating needle position:
  //  0x56 = First visible movment of needle starts
  //  0x5D = Begining of Blue section
  //  0x80 = End of Blue Section
  //  0xA9 = Begin Straight up
  //  0xC1 = Midpoint of needle straight up values
  //  0xDB = Needle begins to move from center
  //  0xE6 = Beginning of Red section
  //  0xFE = needle pegged to the right
  // MAP program statement: map(value, fromLow, fromHigh, toLow, toHigh)

  //Can bus data packet values to be sent
  id = 0x329;  //From ECU
  data[0]= 0xD9;
  data[1]= tempValue; //temp bit tdata  From OBDII
  data[2]= 0xB2;
  data[3]= 0x19;
  data[4]= 0x0;
  data[5]= 0xEE; //Throttle position currently just fixed value
  data[6]= 0x0;
  data[7]= 0x0;
  CAN_send();

}//end Temperature Loop

void send_fuel()//ARBID 545
//From ECU, MPG, MIL, overheat light, Cruise
// ErrorState variable controls:
//Check engine(binary 10), Cruise (1000), EML (10000)
//Temp light Variable controls temp light.  Hex 08 is on zero off
{

  //**************** set Error Lights & cruise light ******
  if(tempValue>0xE6){ // lights light if value is 229 (hot)
    tempLight = 8;  // hex 08 = Overheat light on
  }
  else {
    tempLight = 0; // hex 00 = overheat light off
  }

  // int z = 0x60; // + y;  higher value lower MPG
  if (LSBdata + MPG > 0xFF){ //if adding will go past max value
    MSBdata = MSBdata + 0x01;  //increment MSB
  }
  LSBdata=LSBdata + MPG; //increment values based on inj input

  //For debugging
  //   Serial.print(LSBdata);
  //   Serial.print(" ");
  //   Serial.println(MPG);

  //Can bus data packet values to be sent
  id = 0x545;  //From ECU, MPG, MIL, overheat light, Cruise
  data[0]= ErrorState;  //set error byte to error State variable
  data[1]= LSBdata;  //LSB Fuel consumption
  data[2]= MSBdata;  //MSB Fuel Consumption
  data[3]= tempLight;  //Overheat light
  data[4]= 0x7E;
  data[5]= 0x10;
  data[6]= 0x00;
  data[7]= 0x18;

  //Time captures for MPG gauge
  //  timeMPGp1 = micros();
  //  timeMPG = timeMPGp1 - timeMPGp2;
  //  timeMPGp2 = timeMPGp1;

  CAN_send();  //can send 545

} //end send_Fuel if

//********************* Process message ***************
void CAN_ProcessMessage()
{
  time_CAN_in= millis(); //set time for last time message recieved
  //  Serial.println("recd 0x615");  //debug
  Serial.print(data[0],HEX);  //debug
    Serial.print(" ");  //debug
  Serial.print(data[1],HEX);  //debug
      Serial.print(" ");  //debug
  Serial.print(data[2],HEX);  //debug
      Serial.print(" ");  //debug
  Serial.print(data[3],HEX);  //debug
      Serial.print(" ");  //debug
  Serial.print(data[4],HEX);  //debug
      Serial.print(" ");  //debug
  Serial.print(data[5],HEX);  //debug
      Serial.print(" ");  //debug
    Serial.print(data[6],HEX);  //debug
        Serial.print(" ");  //debug
    Serial.println(data[7],HEX);  //debug
  
    //----------------- look for Ignition on -----------------
  if(bitRead(data[4],0) == 1) //look for IGN on bit
  {//IGN is on
    //do something
    IGN_status = 1; //set flag for IGN status
    Serial.println("IGN on");
  }
  else  //AC message from E46 is IGN off
  {
    //Stop ELM and stop sending can (Put elm and MCP's to sleep?)
    Serial.println("IGN off");
  }
  
  

  //----------------- look for AC on -----------------
  if(bitRead(data[0],7) == 1) //look for ac on bit
  {//AC message from E46 is AC on
    //set pin x low (AC on)
//    digitalWrite(AC_pin, LOW); //set AC to on;  this to be changed to send can message to LS3
    Serial.println("ac just turned on");
  }
  else  //AC message from E46 is AC off
  {
    //Set pin x high (AC off)
//    digitalWrite(AC_pin, HIGH); //set AC to OFF;  this to be changed to send can message to LS3
    Serial.println("ac just turned off");
  }

} //end process message

void sleepNow()
{
    /* Now is the time to set the sleep mode. In the Atmega8 datasheet
     * http://www.atmel.com/dyn/resources/prod_documents/doc2486.pdf on page 35
     * there is a list of sleep modes which explains which clocks and 
     * wake up sources are available in which sleep modus.
     *
     * In the avr/sleep.h file, the call names of these sleep modus are to be found:
     *
     * The 5 different modes are:
     *     SLEEP_MODE_IDLE         -the least power savings 
     *     SLEEP_MODE_ADC
     *     SLEEP_MODE_PWR_SAVE
     *     SLEEP_MODE_STANDBY
     *     SLEEP_MODE_PWR_DOWN     -the most power savings
     *
     *  the power reduction management <avr/power.h>  is described in 
     *  http://www.nongnu.org/avr-libc/user-manual/group__avr__power.html
     */  
     
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here

  sleep_enable();          // enables the sleep bit in the mcucr register
                             // so sleep is possible. just a safety pin 
  
  power_adc_disable();
  power_spi_disable();
  power_timer0_disable();
  power_timer1_disable();
  power_timer2_disable();
  power_twi_disable();
  
  
  sleep_mode();            // here the device is actually put to sleep!!
 
                             // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
  sleep_disable();         // first thing after waking from sleep:
                            // disable sleep...

  power_all_enable();
   
}





