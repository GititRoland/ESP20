#include <Arduino.h>

// Declare a pin for built-in LED
#define LED_PIN 2     // Initiate Internal LED Pin
#define LASER_PIN 15  // Initiate Laser Pin
#define LIGHTSENSOR_PIN 36 // Read only PIN.
#define HALL_PIN 35   // Read only PIN for Magnetic Sensor

// TIME KEEPING:
struct timeStruct {
  uint8_t Hours;
  uint8_t Minutes;
  uint32_t Seconds;
  uint32_t MilliSeconds;
  uint32_t MilliSecondsActualCount;
};
volatile timeStruct timeKeeping; // Initiate the timekeeping struct as volatile

// FREQUENCY KEEPING:
struct frequencyStruct {
  uint32_t frequency_array_mHz[10]; // Store the last 10 frequencies in mHz
  uint8_t frequency_index;
  uint8_t frequency_count;
  uint64_t frequency_sum_mHz; // Store the sum in a fixed point notation. 
};
frequencyStruct HallFreqTracking = { {0}, 0, 0, 0}; // Initate all with 0 value. 

// FUNCTION DELCARATIONS:
int togglePIN(int state, int PIN) { 
    if (state) {
      digitalWrite(PIN, LOW);
      return 0;
    } else {
      digitalWrite(PIN, HIGH);
      return 1; 
    }
}

// -- TIME FUNCTIONS:
void updateTime(void) {
  timeKeeping.MilliSecondsActualCount = millis();                              // Get the millisecond count.
  timeKeeping.Hours = timeKeeping.MilliSecondsActualCount / 3600000;           // (60*60*1000)
  timeKeeping.Minutes = (timeKeeping.MilliSecondsActualCount / 60000) % 60;    // (60*1000)
  timeKeeping.Seconds = (timeKeeping.MilliSecondsActualCount / 1000) % 60;     // Seconds
  timeKeeping.MilliSeconds = timeKeeping.MilliSecondsActualCount % 1000;       // MilliTseconds
}

void printTime(void) {
  Serial.printf("%02lu:%02lu:%02lu:%03lu\n",
    timeKeeping.Hours,
    timeKeeping.Minutes,
    timeKeeping.Seconds,
    timeKeeping.MilliSeconds
  );
}

// -- FREQUENCY DECLARATIONS:
void sumFrequencySamples(uint32_t FreqSample_mHz) {
  if (HallFreqTracking.frequency_count < 10) { // Check if 10 samples have been made - Sub 10 Scenario
    HallFreqTracking.frequency_array_mHz[HallFreqTracking.frequency_index] = FreqSample_mHz; // Add the sample to the array. 
    HallFreqTracking.frequency_sum_mHz += FreqSample_mHz; // Add sample to the sum.
    HallFreqTracking.frequency_count++; // Keep count frequencies added.
  } else { // If there are 10 or more frequencies -- Count >= 10 Scenario. 
    HallFreqTracking.frequency_sum_mHz -= HallFreqTracking.frequency_array_mHz[HallFreqTracking.frequency_index]; // Remove the last oldest (indexed) from the sum.
    HallFreqTracking.frequency_array_mHz[HallFreqTracking.frequency_index] = FreqSample_mHz; // Replace the oldest (indexed) with the new. 
    HallFreqTracking.frequency_sum_mHz += FreqSample_mHz; // Add the frequency to the sample.
  }
  HallFreqTracking.frequency_index = (HallFreqTracking.frequency_index + 1) % 10; // Increment Index
}

uint32_t getAverageFrequency_mHz(void) {
  if (HallFreqTracking.frequency_count == 0) {// Check if count is empty to avoid division by zero. 
    return 0; // Return 0 as the average 
  }
  return (uint32_t)(HallFreqTracking.frequency_sum_mHz / HallFreqTracking.frequency_count); // Return the average
}

void printFrequency_mHz(uint32_t Freq_mHz) {
  uint32_t wholeNumberPart = Freq_mHz / 1000; // Get everything else besides last 3 digits. 
  uint32_t fractionalNumberPart = Freq_mHz % 1000; // Gets last 3 digits
  Serial.printf("%lu.%03lu", wholeNumberPart, fractionalNumberPart);
}

void printFrequencies(void) {
  Serial.print("Frequencies: ");
  // Print all the samples taken, and then the average. 
  for (uint8_t i = 0; i < 10; i++) {
    if (i < HallFreqTracking.frequency_count) {
      printFrequency_mHz(HallFreqTracking.frequency_array_mHz[i]);
    } else { 
      Serial.print("-");
    }

    if (i < 9) {
      Serial.print(", ");
    }
  }

  Serial.print(" -- ");
  printFrequency_mHz(getAverageFrequency_mHz());
  Serial.println();
}

void setup() {
  // Initiate the Internal LED PIN
  pinMode(LED_PIN, OUTPUT);

  // Initiate the Laser PIN
  pinMode(LASER_PIN, OUTPUT);

  // Initiate the Light Sensor PIN
  pinMode(LIGHTSENSOR_PIN, INPUT_PULLDOWN);

  // Initate the HALL Sensor PIN
  pinMode(HALL_PIN, INPUT);

  // Picocom setup - picocom -b 9600 /dev/cu.usbserial-1240
  Serial.begin(115200); // Initiate Printing to the Terminal with BAUD 9600
  Serial.println("Booting Setup"); // Print To the terminal
}

void loop() {
  // LASER AND LED STATES - Static
  static int stateLED = 0;     // LED ON
  static int stateLASER = 1;   // LASER ON
  static int stateSENSOR = 0;  // NO LASER DETECTED = 0
  static int LASER_DETECTED_COUNT = 0; 
  static int HALL_LAST_STATE = HIGH; // SET HALL STATE TO HIGH (1)
  static int HALL_COUNT = 0;         // SET HALL COUNT OT ZERO.
  static int HALL_FREQUENCY = 0;     // SET FREQUENCY DEFAULT TO ZERO. (OLD)
  static uint32_t HALL_TIMESTAMP = millis(); // TAKE HALL TIMESTAMP. - ms
  static uint32_t HALL_TIMESTAMP_MICROS = micros(); // TAKE HALL TIMESTAMP - us

  // TOGGLE LED EVERY 1 SECONDS
  stateLED = togglePIN(stateLED, LED_PIN);

  // Check the analog read every 1 seconds. -- Remove comment if needed. 
  /* Serial.print("Analog Value: ");
  Serial.println(analogRead(LIGHTSENSOR_PIN)); */

  // Time Tracking
  updateTime(); // Only gets updated here or at the Laser Detection. 
  // printTime(); -- Uncomment if needed. 

  // MAKE A LOOP TO MAKE THE LASER DIODE SHOOT FAST
  for (int i = 0; i < 5000; i++) { 
    // TOGGLE LASER
    stateLASER = togglePIN(stateLASER, LASER_PIN);

    /* // READ THE ANALOG VALUE
    int analogSensorValue = analogRead(LIGHTSENSOR_PIN);

    // CHECK SENSOR 
    if (analogSensorValue < 100) { // CHECK IF THE SENSOR HAS BEEN READ
      stateSENSOR = 1; // SET THE STATE TO TRUE
      LASER_DETECTED_COUNT++;

      // Time Tracking
      updateTime();
      printTime();

      // SEE WHAT THE SENSOR READS
      Serial.print(" -- Analog Value: ");
      Serial.print(analogSensorValue);
      Serial.print(" -- LASER DETECTED COUNT: "); // PRINT NORMAL
      Serial.println(LASER_DETECTED_COUNT);   // PRINT NEW LINE
    } */ 

    // Hall Sensor Detection 
    int HALL_CURRENT_STATE = digitalRead(HALL_PIN); // READ CURRENT STATE
    if (HALL_CURRENT_STATE == LOW && HALL_LAST_STATE == HIGH) { // SEE IF WE WENT FROM NO MAGNET TO MAGNET.
      HALL_COUNT++;                 // ADD TO THE TOTAL COUNT
      // Serial.println("MAGNET DETECTED");
      // Serial.print("MANGET DETECTED COUNT:");
      // Serial.println(HALL_COUNT);
      HALL_FREQUENCY++;             // ADD TO THE FREQUENCY COUNT

      // Hall Frequency Detection -- With Averages (NEW CODE)
      uint32_t HALL_CURRENT_TIME_MICROS = micros(); // TAKE THE TIME NOW
      if (HALL_TIMESTAMP_MICROS != 0) {
        uint32_t HALL_PERIOD_MICROS = HALL_CURRENT_TIME_MICROS - HALL_TIMESTAMP_MICROS; // CALCULATE THE PERIOD.
        if (HALL_PERIOD_MICROS > 1000) { // Only add if period is longer than 1 ms ( To avoid for too fast detection/gliches)
          uint32_t Calculated_Freq_mHz = 1000000000 / HALL_PERIOD_MICROS;  // f_mHz = 1000000000/T_us F.eks: 1000000000/(10^6)
          sumFrequencySamples(Calculated_Freq_mHz); // Add the Frequency Sample
          printFrequencies(); // Print the Frequencies
        }
      }
      HALL_TIMESTAMP_MICROS = HALL_CURRENT_TIME_MICROS; // RESET THE TIMESTAMP
    }
    
    // Hall Frequency Detection -- Next thing to work on could be an average, or getting a more precise float count by comparing period (from previous to last detection)
    if (millis() - HALL_TIMESTAMP >= 1000) {
      // PRINT FIRST, THEN RESET
      Serial.print("Magnet Detections Per Second: ");
      Serial.println(HALL_FREQUENCY);

      // RESET VALUES
      HALL_FREQUENCY = 0;           // RESET THE FREQUENCY COUNT
      HALL_TIMESTAMP = millis();    // RESET THE TIMESTAMP
    }
    
    HALL_LAST_STATE = HALL_CURRENT_STATE; // UPDATE LAST STATE FOR NEXT LOOP.
  }
}