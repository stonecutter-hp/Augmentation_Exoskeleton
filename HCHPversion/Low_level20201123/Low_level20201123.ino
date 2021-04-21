#include <AD7173.h>  // the library for ADC
#include "ADC.h"
#include "Control.h"
#include "IIC.h"
#include "IMU.h"
#include "SerialComu.h"
#include "Timers.h"

/*  Program try 1 for the low-level control of HCHP version exoskelton 
    prototype which is aiming to run at test bench with only one side
    of torque transmission system
Log
20201123
  Creating based on the logical of "test_serial_ADC_timer.ino"
  Setup feedback item and parameters based on test-bench sensor implementation

***Program logic***
1) read desired torque command from PC 
    (if no command recieved, keep sending sensor feedback with fixed frequncy
    and use the initial/former command as reference command)--> 
2) read sensor feedback including: 
    potentiometer for torque feedback, 
    motor driver for motor current and velocity feedback (for potential cascaded control), 
    load cell for cable force feedback,
    and IMU*2 for torque feedback and human back(link) motion -->
3) calculate the actual control commmand for motor -->
4) send sensor feedback to PC.
*/

void setup() {
  /******************************** Serial Initialization ******************************/
  Serial.begin(460800);   // initialize serial set baurd rate

  /*************************** Control parameter Initialization ************************/
  Control_Init();  // initialize the control parameters
  
  /*********** Assign general IO for motor control and LED state indication ************/
  GeneralIO_Init();

  /****************** ADC initialization for channel mode configuration ****************/
  ADC_Init();
  Filter_Init();  // Initial ADC feedback storing/processing vector/matrix

  /******************************** IIC Initialization *********************************/
  IIC_Init();     // IIC initialization for IMU communication  

  /******************************** IMU Initialization *********************************/
  IMU_Init();     // Initial IMU feedback storing vector

  /******************************** Timer Initialization *******************************/
  Timers_Init();

  /***************************** PWM Generation Initialization *************************/
  PWMmode_Init();

  // resume all the timers
  Timer1.resume();     // Motor L PWM
  Timer2.resume();     // Motor R PWM
  Timer3.resume();     // ADC and control update
  Timer4.resume();     // Sending PC update

  /******************* ADC value and IMU angle feedback initialization *****************/
  getADCaverage(1);
  getIMUangleL();
  // yawAngleR20(1);     // Forced trunk yaw angle correction,
  PotentioLP1_InitValue = Aver_ADC_value[PotentioLP1];
  SupportBeamAngleL_InitValue = angleActualA[rollChan];
  delay(5); 
}


void loop() {
//  unsigned long starttime;
//  unsigned long stoptime;
//  unsigned long looptime;
//  starttime = millis();
  receiveDatafromPC();         // receive data from PC
  receivedDataPro();           // decomposite data received from PC
  if(ADC_update) {
  	getADCaverage(1);          // get ADC value
  	ADC_update = false;
  }
  // getIMUangle();            // get rotation angle of both support beam and human back/link
  getIMUangleT();              // get support beam rotation angle from IMU
  // yawAngleR20(0);              // trunk yaw angle correction, should before data sensor feedback processing and sending
  sensorFeedbackPro();         // processing sensor feedback for closed-loop control 
  // MovingAverFilterIMUC(rollChan,3);   // Averaged moving filtered
  if(Control_update) {
  	Control(1);                // calculate controlled command: PWM duty cycles
  	Control_update = false;
  }
  if(SendPC_update) {
  	sendDatatoPC();            // send sensor data to PC and allow next receiving cycle
    SendPC_update = false;
    receiveCompleted = false;  // Mark this correct receiving infomation is used up 
  }
    receiveContinuing = true;  // Enable next time's recieving
    USART_RX_STA = 0;          // Return to zero for receiving buffer 
//  stoptime = millis();
//  looptime = stoptime - starttime;
//  Serial.println(looptime);
//  while(1);
}
