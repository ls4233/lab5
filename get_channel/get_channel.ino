/* This example will read all channels from the AS7341 and print out reported values */

#include <Adafruit_AS7341.h>
Adafruit_AS7341 as7341;

// Reference values
float C_dark[10] = {
  0, 0, 0, 1, 0, 0, 0, 0, 9, 0
};

float C_ref[10] = {
  291, 790, 1051, 2376, 3375, 4428, 4886, 2549, 9062, 692
};

// Moving average buffers for each color band
const int MA_WINDOW = 10;
int f1_buf[MA_WINDOW], f2_buf[MA_WINDOW], f3_buf[MA_WINDOW], f4_buf[MA_WINDOW];
int f5_buf[MA_WINDOW], f6_buf[MA_WINDOW], f7_buf[MA_WINDOW], f8_buf[MA_WINDOW];
int clear_buf[MA_WINDOW], nir_buf[MA_WINDOW];

// Moving average variables
int idx = 0;
bool filled = false;

// Moving average helper
int movingAverage(int *buf, int window) {
  long sum = 0;
  for (int i = 0; i < window; i++) {
    sum += buf[i];
  }
  return sum / window;
}

// Convert raw ADC to reflectance
void to_reflectance(float* R, 
                    const uint16_t* C_sample, 
                    const float* C_dark, 
                    const float* C_ref, 
                    int N)
{
    for (int i = 0; i < N; i++) {

        float sample = (float)C_sample[i];
        float dark   = C_dark[i];
        float ref    = C_ref[i];

        float denom = ref - dark;
        if (fabs(denom) < 1e-9) denom = 1e-9;

        float r = (sample - dark) / denom;

        if (r < 1e-6) r = 1e-6;
        if (r > 1.0)  r = 1.0;

        R[i] = r;
    }
}

void setup() {
  Serial.begin(115200);

  // Wait for communication with the host computer serial monitor
  while (!Serial) {
    delay(1);
  }
  
  if (!as7341.begin()){
    Serial.println("Could not find AS7341");
    while (1) { delay(10); }
  }

  as7341.setATIME(100);
  as7341.setASTEP(999);
  as7341.setGain(AS7341_GAIN_256X);
}

void loop() {
  // Read all channels at the same time and store in as7341 object
  if (!as7341.readAllChannels()){
    Serial.println("Error reading all channels!");
    return;
  }

  // Read raw values
  int f1  = as7341.getChannel(AS7341_CHANNEL_415nm_F1);
  int f2  = as7341.getChannel(AS7341_CHANNEL_445nm_F2);
  int f3  = as7341.getChannel(AS7341_CHANNEL_480nm_F3);
  int f4  = as7341.getChannel(AS7341_CHANNEL_515nm_F4);
  int f5  = as7341.getChannel(AS7341_CHANNEL_555nm_F5);
  int f6  = as7341.getChannel(AS7341_CHANNEL_590nm_F6);
  int f7  = as7341.getChannel(AS7341_CHANNEL_630nm_F7);
  int f8  = as7341.getChannel(AS7341_CHANNEL_680nm_F8);
  int clr = as7341.getChannel(AS7341_CHANNEL_CLEAR);
  int nir = as7341.getChannel(AS7341_CHANNEL_NIR);

  // Print raw values
  Serial.print("Raw Channels: ");
  Serial.print("F1=");  Serial.print(f1);
  Serial.print("  F2="); Serial.print(f2);
  Serial.print("  F3="); Serial.print(f3);
  Serial.print("  F4="); Serial.print(f4);
  Serial.print("  F5="); Serial.print(f5);
  Serial.print("  F6="); Serial.print(f6);
  Serial.print("  F7="); Serial.print(f7);
  Serial.print("  F8="); Serial.print(f8);
  Serial.print("  CLR="); Serial.print(clr);
  Serial.print("  NIR="); Serial.print(nir);
  Serial.println();

  // Store into buffers
  f1_buf[idx] = f1;  f2_buf[idx] = f2;  f3_buf[idx] = f3;  f4_buf[idx] = f4;
  f5_buf[idx] = f5;  f6_buf[idx] = f6;  f7_buf[idx] = f7;  f8_buf[idx] = f8;
  clear_buf[idx] = clr;
  nir_buf[idx]   = nir;

  // Advance circular index for moving average
  idx++;
  if (idx >= MA_WINDOW) {
    idx = 0;
    filled = true;
  }

  // Wait to compute averages after there are enough samples
  int window = filled ? MA_WINDOW : idx;

  // Compute moving averages
  int s1  = movingAverage(f1_buf, window);
  int s2  = movingAverage(f2_buf, window);
  int s3  = movingAverage(f3_buf, window);
  int s4  = movingAverage(f4_buf, window);
  int s5  = movingAverage(f5_buf, window);
  int s6  = movingAverage(f6_buf, window);
  int s7  = movingAverage(f7_buf, window);
  int s8  = movingAverage(f8_buf, window);
  int sClr = movingAverage(clear_buf, window);
  int sNir = movingAverage(nir_buf, window);

  // Print smoothed values
  // Serial.print("F1 415nm : "); Serial.println(s1);
  // Serial.print("F2 445nm : "); Serial.println(s2);
  // Serial.print("F3 480nm : "); Serial.println(s3);
  // Serial.print("F4 515nm : "); Serial.println(s4);
  // Serial.print("F5 555nm : "); Serial.println(s5);
  // Serial.print("F6 590nm : "); Serial.println(s6);
  // Serial.print("F7 630nm : "); Serial.println(s7);
  // Serial.print("F8 680nm : "); Serial.println(s8);
  // Serial.print("Clear    : "); Serial.println(sClr);
  // Serial.print("Near IR  : "); Serial.println(sNir);
  // Serial.println();


  // Concat moving average values into C_sample
  uint16_t C_sample[10] = {
    (uint16_t)s1,
    (uint16_t)s2,
    (uint16_t)s3,
    (uint16_t)s4,
    (uint16_t)s5,
    (uint16_t)s6,
    (uint16_t)s7,
    (uint16_t)s8,
    (uint16_t)sClr,
    (uint16_t)sNir
  };

  // Convert to reflectance
  float R[10];
  to_reflectance(R, C_sample, C_dark, C_ref, 10);
  Serial.print("Reflectance: ");
  for (int i = 0; i < 10; i++) {
      Serial.print(R[i], 6);   // print with 6 decimal places
      Serial.print(i < 9 ? ", " : "\n");
  }
  delay(1000);

  // // Print out the stored values for each channel
  // Serial.print("F1 415nm : ");
  // Serial.println(as7341.getChannel(AS7341_CHANNEL_415nm_F1));
  // Serial.print("F2 445nm : ");
  // Serial.println(as7341.getChannel(AS7341_CHANNEL_445nm_F2));
  // Serial.print("F3 480nm : ");
  // Serial.println(as7341.getChannel(AS7341_CHANNEL_480nm_F3));
  // Serial.print("F4 515nm : ");
  // Serial.println(as7341.getChannel(AS7341_CHANNEL_515nm_F4));
  // Serial.print("F5 555nm : ");
  // Serial.println(as7341.getChannel(AS7341_CHANNEL_555nm_F5));
  // Serial.print("F6 590nm : ");
  // Serial.println(as7341.getChannel(AS7341_CHANNEL_590nm_F6));
  // Serial.print("F7 630nm : ");
  // Serial.println(as7341.getChannel(AS7341_CHANNEL_630nm_F7));
  // Serial.print("F8 680nm : ");
  // Serial.println(as7341.getChannel(AS7341_CHANNEL_680nm_F8));

  // Serial.print("Clear    : ");
  // Serial.println(as7341.getChannel(AS7341_CHANNEL_CLEAR));

  // Serial.print("Near IR  : ");
  // Serial.println(as7341.getChannel(AS7341_CHANNEL_NIR));

  Serial.println("");
}