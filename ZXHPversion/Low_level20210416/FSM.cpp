/***********************************************************************
 * The Finite State Machine Alogrithm for User Intention Detection
 **********************************************************************/

#include "FSM.h"


/* High-level controller program running info */
bool HLControl_update = true;  // high-level control update flag
float HLUpdateFre;             // hihg-level control frequency

/* Status flag for UID strategy */
MotionType mode;               // this time's motion mode flag
MotionType PreMode;            // last time's motion mode flag
AsymSide side;                 // Asymmetric side flag
BendTech tech;                 // bending tech flag  

/* Controller parameters and thresholds for UID strategy */
UIDCont UID_Subject1;          // UID strategy parameters for specific subjects

/* Intermediate auxiliary parameters for UID strategy */
// Parameters Directly feedback from sensor
float HipAngL;                     // Left hip joint angle
float HipAngL_InitValue;           // Auxiliary parameter for left hip joint angle
float HipAngR;                     // Right hip joint angle
float HipAngR_InitValue;           // Auxiliary parameter for right hip joint angle
float TrunkYawAng;                 // Trunk yaw angle
float TrunkYaw_InitValue;          // Auxiliary parameter for trunk yaw angle
float TrunkFleAng;                 // Trunk flexion angle
float TrunkFleAng_InitValue;       // Auxiliary parameter for trunk pitch angle
float TrunkFleVel;                 // Trunk flexion angular velocity
// Parameters calculated from sensor feedback
float HipAngMean;                  // (Left hip angle + right hip angle)/2
float HipAngDiff;                  // (Left hip angle - right hip angle)
float HipAngStd;                   // Std(HipAngMean) within certain time range
float HipAngVel;                   // Velocity of HipAngMean
float ThighAngL;                   // Left thigh angle
float ThighAngR;                   // Right thigh angle
float ThighAngMean;                // (Left thigh angle + right thigh angle)/2
float ThighAng_InitValue;          // Auxiliary parameter for mean thigh angle at T0 moment
float ThighAngStd;                 // Std(ThighAngMean) within certain time range
float HipAngDiffStd;               // Std(HipAngDiff) within certain time range
// A window store the historical HipAngMean value of certain cycle for standard deviation calculation
float HipAngMeanPre[FilterCycles];
float HipAngMeanBar;               // Auxiliary parameter X_bar for standard deviation calculation
// A window store the historical HipAngDiff value of certain cycle for standard deviation calculation
float HipAngDiffPre[FilterCycles];
float HipAngDiffBar;               // Auxiliary parameter X_bar for standard deviation calculation

/**
 * Control parameter initialization for UID strategy
 * Initial controller including: 
 * Sensor feedbacks, status flags, thresholds and auxiliary parameters used for UID strategy
 */
void UID_Init(void) {
  /* Initialize sensor feedbacks */
  // Parameters for UID strategy directly feedback from sensor
  HipAngL = 0;                     // Left hip joint angle
  HipAngL_InitValue = 0;           // Auxiliary parameter for left hip joint angle
  HipAngR = 0;                     // Right hip joint angle
  HipAngR_InitValue = 0;           // Auxiliary parameter for right hip joint angle
  TrunkYawAng = 0;                 // Trunk yaw angle
  TrunkYaw_InitValue = 0;          // Auxiliary parameter for trunk yaw angle
  TrunkFleAng = 0;                 // Trunk flexion angle
  TrunkFleAng_InitValue = 0;       // Auxiliary parameter for trunk pitch angle
  TrunkFleVel = 0;                 // Trunk flexion angular velocity
  // Parameters for UID strategy calculated from sensor feedback
  HipAngMean = 0;                  // (Left hip angle + right hip angle)/2
  HipAngDiff = 0;                  // (Left hip angle - right hip angle)
  HipAngStd = 0;                   // Std(HipAngMean) within certain time range
  HipAngVel = 0;                   // Velocity of HipAngMean
  ThighAngL = 0;                   // Left thigh angle
  ThighAngR = 0;                   // Right thigh angle
  ThighAngMean = 0;                // (Left thigh angle + right thigh angle)/2
  ThighAng_InitValue = 0;          // Auxiliary parameter for thigh angle
  ThighAngStd = 0;                 // Std(ThighAngMean) within certain time range
  HipAngDiffStd = 0;               // Std(HipAngDiff) within certain time range
  
  /* Initialize UID status flags */  
  mode = StopState;   // Motion detection mode, default is Stop state
  PreMode = mode;
  side = none;        // Asymmetric side, default is no asymmetric
  
  /* Initialize thresholds for specific subject */
  // Parameters for UID strategy
  UID_Subject1.ThrTrunkFleAng = 15;    // deg, Threshold for trunk flexion angle
  UID_Subject1.ThrTrunkFleVel = 50;    // deg/s, Threshold for trunk flexion velocity
  UID_Subject1.ThrHipAngMean = 30;     // deg, Threshold for mean value of summation of left and right hip angle  
  UID_Subject1.ThrHipAngDiff = 10;     // deg, Threshold for difference between left and right hip angle
  UID_Subject1.ThrHipAngStd = 10;      // deg, Threshold for standard deviation of mean hip angle
  UID_Subject1.ThrHipVel = 10;         // deg/s, Threshold for mean hip angle velocity
  UID_Subject1.ThrThighAngMean = 10;   // deg, Threshold for mean value of summation of left and right thigh angle
  // Notice that ThrThighAngMean should not be larger than ThrHipAngMean - ThrTrunkFleAng if ThighAng comes from
  // calculation of (HipAng - TrunkFleAng) instead of measurement
  if(UID_Subject1.ThrThighAngMean > UID_Subject1.ThrHipAngMean - UID_Subject1.ThrTrunkFleAng) {
    UID_Subject1.ThrThighAngMean = UID_Subject1.ThrHipAngMean - UID_Subject1.ThrTrunkFleAng;
  }
  UID_Subject1.ThrThighAngStd = 10;    // deg, Threshold for standard deviation of mean thigh angle
  UID_Subject1.ThrThighAngVel = 30;    // deg/s, Threshold for mean thigh angle velocity
  UID_Subject1.ThrHipAngDiffStd = 10;  // deg, Threshold for standard deviation of difference between left and right hip angle
  UID_Subject1.StdRange = 10;          // Standard deviation calculation range

  // Initialize auxiliary parameters for Std calculation
  for(int i=0; i<FilterCycles; i++) {
    HipAngMeanPre[i] = 0;
    HipAngDiffPre[i] = 0;
  }
  HipAngMeanBar = 0;
  HipAngDiffBar = 0;
}

/**
 * Pre-processing for sensor feedback related to high-level controller 
 * to make sure the initial status of sensor is good for calibration
 */
void HLPreproSensorInit() {
  int8_t SensorReady;
  int8_t SensorReady_1;
  int8_t SensorReady_2;
  int8_t SensorReady_3;
  SensorReady = 0;
  while(SensorReady == 0){
    // Initialize present yaw angle as 0 reference. Notice inside the function info will be collected
    // from IMUC simultaneously including: TrunkAng, TrunkVel
    yawAngleR20(ForcedInit,OperaitonAloIMUC);
    delay(1);
    // Collect info from ADC including: Potentiometets(HipAng), ForceSensors(Interaction force)
    // and Motor status 
    getADCaverage(1);
    delay(1);
    // Initialize the inital value for each sensor feedback
    // Notice to check the Initial value is ADC raw data or Processed data
    HipAngL_InitValue = Aver_ADC_value[PotentioLP1]/PotentioLP1_Sensitivity;
    HipAngR_InitValue = Aver_ADC_value[PotentioRP2]/PotentioRP2_Sensitivity;
    TrunkFleAng_InitValue = angleActualC[rollChan];
    // Here place program to check if these initial value of each sensor is near the expected position. 
    // If not, recalibration the initial value of the sensor feedback 
    if(HipAngL_InitValue > HipAngL_CaliValue + HipAngL_Tol || HipAngL_InitValue < HipAngL_CaliValue - HipAngL_Tol)
      SensorReady_1 = 0;
    else SensorReady_1 = 1;
    if(HipAngR_InitValue > HipAngR_CaliValue + HipAngR_Tol || HipAngR_InitValue < HipAngR_CaliValue - HipAngR_Tol)
      SensorReady_2 = 0;
    else SensorReady_2 = 1;
    if(TrunkFleAng_InitValue > TrunkFleAng_CaliValue + TrunkFleAng_Tol || TrunkFleAng_InitValue < TrunkFleAng_CaliValue - TrunkFleAng_Tol)
      SensorReady_3 = 0;
    else SensorReady_3 = 1;
    SensorReady = SensorReady_1*SensorReady_2*SensorReady_3;
  }
  Serial.println("Sensor Ready for High-level Controller.");
}

/**
 * Set the yaw angle of human trunk to zero
 * @param unsigned char - Yaw init mode: 1-force to set, other number-logic set
 * @param IMUAlo - IMU operation algorithm
 */
void yawAngleR20(uint8_t yawInitmode, IMUAlo aloMode){
  // Roughly yaw angle return to zero logic: 
  // Detect mode 1 and premode is larger than 3
  // i.e., From bending back to other motion
  if(yawInitmode == ForcedInit) {
    if(aloMode == IMU9Axis) {
      getIMUangleT();
      TrunkYaw_InitValue = angleActualC[yawChan];
    }
    else if(aloMode == IMU6Axis) {
      // set2zeroL();
      // set2zeroR();
      set2zeroT();
      getIMUangleT();
      TrunkYaw_InitValue = 0;
    }
  }
  else {
    if(mode == Standing && PreMode > Lowering) {
      if(aloMode == IMU9Axis) {
        getIMUangleT();
        TrunkYaw_InitValue = angleActualC[yawChan];
      }
      else if(aloMode == IMU6Axis) {
        // set2zeroL();
        // set2zeroR();
        set2zeroT();
        getIMUangleT();
        TrunkYaw_InitValue = 0;        
      }
    }
  }
}

/**
 * Processing sensor feedback for High-level closed-loop control and data sending
 */
void HLsensorFeedbackPro() {
//  // Smooth the hip angle feedback
//  MovingAverageFilter(PotentioLP1,5);
//  MovingAverageFilter(PotentioRP2,5);
  // Directly feedback from sensor
  HipAngL = Aver_ADC_value[PotentioLP1]/PotentioLP1_Sensitivity - HipAngL_InitValue;
  HipAngR = Aver_ADC_value[PotentioRP2]/PotentioRP2_Sensitivity - HipAngR_InitValue;
  TrunkFleAng = TrunkFleAng_InitValue - angleActualC[rollChan];
  TrunkYawAngPro();
  TrunkFleVel = velActualC[rollChan];
  // Calculated from sensor feedback
  HipAngMean = (HipAngL+HipAngR)/2;
  HipAngDiff = HipAngL-HipAngR;
  HipAngStd = HipAngStdCal(UID_Subject1.StdRange);
  ThighAngL = HipAngL - TrunkFleAng;
  ThighAngR = HipAngR - TrunkFleAng;
  ThighAngMean = (ThighAngL+ThighAngR)/2-ThighAng_InitValue;
//  ThighAngStd = ThighAngStdCal(UID_Subject1.StdRange);
  HipAngDiffStd = HipAngDiffStdCal(UID_Subject1.StdRange);
}

/**
 * Yaw angle processing for practical control
 */
void TrunkYawAngPro() {
  // Trunk yaw angle feedback info procesisng for high-level controller
  TrunkYawAng = angleActualC[yawChan] - TrunkYaw_InitValue;
  if(TrunkYawAng > 180) {
    TrunkYawAng = TrunkYawAng-360;
  }
  else if(TrunkYawAng < -180) {
    TrunkYawAng = TrunkYawAng+360;
  }
}

/**
 * Calculate standard deviation for HipAngMean within certain cycles
 * @param int - cycles: 1~FilterCycles
 * @param return - calculated standard deviation
 */
double HipAngStdCal(int cycles) {
  double interMean;
  double interValue;
  interValue = 0.0;
  if(cycles > FilterCycles) {
    cycles = FilterCycles;
  }
  else if(cycles < 1) {
    cycles = 1;
  }
  // get this times' mean value
  interMean = HipAngMeanBar + (HipAngMean - HipAngMeanPre[FilterCycles-cycles])/cycles;
  // update the data in the moving window
  for(int j=0; j<FilterCycles-1; j++) {
    HipAngMeanPre[j] = HipAngMeanPre[j+1];
  }
  HipAngMeanPre[FilterCycles-1] = HipAngMean;
  // store the last time mean value
  HipAngMeanBar = interMean;
  // calculate standard deviation
  for(int j=FilterCycles-cycles; j<FilterCycles; j++) {
    interValue = interValue + pow(HipAngMeanPre[j]-interMean,2);
  }
  interValue = sqrt(interValue/cycles);
  return interValue;  
}

/**
 * Calculate standard deviation for HipAngDiff within certain cycles
 * @param int - cycles: 1~FilterCycles
 * @param return - calculated standard deviation
 */
double HipAngDiffStdCal(int cycles) {
  double interMean;
  double interValue;
  interValue = 0.0;
  if(cycles > FilterCycles) {
    cycles = FilterCycles;
  }
  else if(cycles < 1) {
    cycles = 1;
  }
  // get this times' mean value
  interMean = HipAngDiffBar + (HipAngDiff - HipAngDiffPre[FilterCycles-cycles])/cycles;
  // update the data in the moving window
  for(int j=0; j<FilterCycles-1; j++) {
    HipAngDiffPre[j] = HipAngDiffPre[j+1];
  }
  HipAngDiffPre[FilterCycles-1] = HipAngDiff;
  // store the last time mean value
  HipAngDiffBar = interMean;
  // calculate standard deviation
  for(int j=FilterCycles-cycles; j<FilterCycles; j++) {
    interValue = interValue + pow(HipAngDiffPre[j]-interMean,2);
  }
  interValue = sqrt(interValue/cycles);
  return interValue;    
}

/**
 * Conduct simple user intention detection and torque generation calculation as reference torque 
 * for low-level control based on sensor information feedback from force sensors, IMUs, Potentiometers
 * and Motor driver
 * @para unsigned char - control mode 1 for user intenntion detection: 1-xx algorithm, 2-xx algorithm
 *       unsigned char - control mode 2 for torque generation strategy: 1-xx strategy, 2-xx strategy
 */
void HLControl(uint8_t UIDMode, uint8_t RTGMode) {
  HL_UserIntentDetect(UIDMode);
  HL_ReferTorqueGenerate(RTGMode);
  // ------------------------ Motor status updating -------------------
  // if high-level command stop state
  if(mode == StopState) {
    // Set zero for reference torque
    desiredTorqueR = 0;
    desiredTorqueL = 0;
    // disable motor control
    digitalWrite(MotorEnableL,LOW);
  }
  else {
    digitalWrite(MotorEnableL,HIGH); //Enable motor control 
  }
}

/**
 * Conduct simple user intention detection 
 * @para unsigned char - control mode for user intenntion detection: 1-xx algorithm, 2-xx algorithm
 */
void HL_UserIntentDetect(uint8_t UIDMode) {
  // User intent detection strategy v1 (Referring to the 'Thoughts Keeping notbook')
  if(UIDMode == 1) {
    if(HipAngL < UID_Subject1.ThrHipAngMean) {
      // A illustration of motion mode update
      mode = Standing; 
      side = none;  
    }
    // ----- Here replace for detailed user intention detection alogrithm -------
  }
   
}