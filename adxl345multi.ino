#include "math.h";        // include math library for square root function
#define Addr 0x53         // ADXL345 I2C address is 0x53(83)
#define GScaling 0.003901 // 3.9mG per bit A2D Scaling
#define A2DRES 8192       // 2^13 = 8192
#define HALFSCALE 4095    // (2^13)/2 - 1

int x0data[32], x1data[32], y0data[32], y1data[32], z0data[32], z1data[32];
int xdata[32], ydata[32], zdata[32];
int xos, yos, zos;
int xavg, yavg, zavg;
double Adata[8];

int configAcc(String command);  // function declaration for calibration

void setup()
{
  // Declare particle variables and function
  Particle.variable("i2cdevice","ADXL345");
  Particle.variable("xAvg",xavg);
  Particle.variable("yAvg",yavg);
  Particle.variable("ZAvg",zavg);
  Particle.function("AccSetup", configAcc); 

  Wire.begin();                 // Initialise I2C communication as MASTER
  Serial.begin(38400);          // set baud rate

  Wire.beginTransmission(Addr);
  Wire.write(0x2D);             // Select power control register
  Wire.write(0x08);             // disable auto sleep
  Wire.endTransmission();

  Wire.beginTransmission(Addr);
  Wire.write(0x31);             // Select data format register
  Wire.write(0b00001011);       // Select full resolution, +/-16g
  Wire.endTransmission();

  delay(100);
}

void loop()
{
  for(int f = 0; f < 8; f++)      // step through 8 frequencies starting with ODR = 25Hz
  {
    Wire.beginTransmission(Addr); // Start I2C Transmission
    Wire.write(0x2C);             // Select bandwidth rate register
    Wire.write(8 + f);            // 25, 50, 100, 200, 400, 800, 1.6k, 3.2k
    Wire.write(0x38);             // Set FIFO Control register
    Wire.write(0b01011111);       // FIFO mode
    Wire.endTransmission();
    delay(100);

    for(int i = 0; i < 32; i++)       // Read all 32 values from FIFO
    {
      Wire.beginTransmission(Addr);
      Wire.write(0x32);               // Point to the DATAX0 register
      Wire.requestFrom(Addr,6);       // request 6 bytes of data (X0 - Z1)
      while(Wire.available())
      {
        x0data[i] = Wire.read();      // put x,y,z data into arrays
        x1data[i] = Wire.read();
        y0data[i] = Wire.read();
        y1data[i] = Wire.read();
        z0data[i] = Wire.read();
        z1data[i] = Wire.read();
      }
      Wire.endTransmission();
    }

    for (int i=1; i<32; i++)    // read all 32 values from arrays to calculate amplitude
    {
      xdata[i] = ((x1data[i]) & 0x1F)<<8 | (x0data[i] & 0xFF);    // combine 5 MSB and 8 LSB into one variable
      ydata[i] = ((y1data[i]) & 0x1F)<<8 | (y0data[i] & 0xFF);
      zdata[i] = ((z1data[i]) & 0x1F)<<8 | (z0data[i] & 0xFF);

      if (xdata[i]>HALFSCALE) xdata[i] -= A2DRES;  // convert to signed value
      if (ydata[i]>HALFSCALE) ydata[i] -= A2DRES;  // convert to signed value
      if (zdata[i]>HALFSCALE) zdata[i] -= A2DRES;  // convert to signed value

      xdata[i] -= xos;                             // subtract offset
      ydata[i] -= yos;
      zdata[i] -= zos;

// calculate amplitude by summing the squares, divide vectors by 100 to keep number manageable
      Adata[f] += (xdata[i]*xdata[i])/100 + (ydata[i]*ydata[i])/100 + (zdata[i]*zdata[i])/100;
      xavg += xdata[i];     // add samples for average, this is only useful for calibrating system at rest
      yavg += ydata[i];     
      zavg += zdata[i];
    }
    Adata[f] = sqrt(Adata[f]);  // take the squareroot of the sume of the squares
    xavg = xavg>>5;             // divide by 32 to get averages for offset 
    yavg = yavg>>5;
    zavg = zavg>>5;
  }
  String devID = System.deviceID();

// publish 8 A(f) values in json format
  Particle.publish("Adata", "{ \"f1\": \"" + String(Adata[0]) + "\"," +
   "\"f2\": \"" + String(Adata[1]) + "\"," +
   "\"f3\": \"" + String(Adata[2]) + "\"," +
   "\"f4\": \"" + String(Adata[3]) + "\"," +
   "\"f5\": \"" + String(Adata[4]) + "\"," +
   "\"f6\": \"" + String(Adata[5]) + "\"," +
   "\"f7\": \"" + String(Adata[6]) + "\"," +
   "\"f8\": \"" + String(Adata[7]) + "\" }", PRIVATE);

   String xvalues = String(String(xdata[1]) +
    ", " + String(xdata[2]) + ", " + String(xdata[3]) +
    ", " + String(xdata[4]) + ", " + String(xdata[5]) +
    ", " + String(xdata[6]) + ", " + String(xdata[7]) +
    ", " + String(xdata[8]) + ", " + String(xdata[9]) +
    ", " + String(xdata[10]) + ", " + String(xdata[11]) +
    ", " + String(xdata[12]) + ", " + String(xdata[13]) +
    ", " + String(xdata[14]) + ", " + String(xdata[15]) +
    ", " + String(xdata[16]) + ", " + String(xdata[17]) +
    ", " + String(xdata[18]) + ", " + String(xdata[19]) +
    ", " + String(xdata[20]) + ", " + String(xdata[21]) +
    ", " + String(xdata[22]) + ", " + String(xdata[23]) +
    ", " + String(xdata[24]) + ", " + String(xdata[25]) +
    ", " + String(xdata[26]) + ", " + String(xdata[27]) +
    ", " + String(xdata[28]) + ", " + String(xdata[29]) +
    ", " + String(xdata[30]) + ", " + String(xdata[31]));
   Particle.publish("xvalues:", xvalues);

   String yvalues = String(String(ydata[1]) +
    ", " + String(ydata[2]) + ", " + String(ydata[3]) +
    ", " + String(ydata[4]) + ", " + String(ydata[5]) +
    ", " + String(ydata[6]) + ", " + String(ydata[7]) +
    ", " + String(ydata[8]) + ", " + String(ydata[9]) +
    ", " + String(ydata[10]) + ", " + String(ydata[11]) +
    ", " + String(ydata[12]) + ", " + String(ydata[13]) +
    ", " + String(ydata[14]) + ", " + String(ydata[15]) +
    ", " + String(ydata[16]) + ", " + String(ydata[17]) +
    ", " + String(ydata[18]) + ", " + String(ydata[19]) +
    ", " + String(ydata[20]) + ", " + String(ydata[21]) +
    ", " + String(ydata[22]) + ", " + String(ydata[23]) +
    ", " + String(ydata[24]) + ", " + String(ydata[25]) +
    ", " + String(ydata[26]) + ", " + String(ydata[27]) +
    ", " + String(ydata[28]) + ", " + String(ydata[29]) +
    ", " + String(ydata[30]) + ", " + String(ydata[31]));
   Particle.publish("yvalues:", yvalues);

   String zvalues = String(String(zdata[1]) +
    ", " + String(zdata[2]) + ", " + String(zdata[3]) +
    ", " + String(zdata[4]) + ", " + String(zdata[5]) +
    ", " + String(zdata[6]) + ", " + String(zdata[7]) +
    ", " + String(zdata[8]) + ", " + String(zdata[9]) +
    ", " + String(zdata[10]) + ", " + String(zdata[11]) +
    ", " + String(zdata[12]) + ", " + String(zdata[13]) +
    ", " + String(zdata[14]) + ", " + String(zdata[15]) +
    ", " + String(zdata[16]) + ", " + String(zdata[17]) +
    ", " + String(zdata[18]) + ", " + String(zdata[19]) +
    ", " + String(zdata[20]) + ", " + String(zdata[21]) +
    ", " + String(zdata[22]) + ", " + String(zdata[23]) +
    ", " + String(zdata[24]) + ", " + String(zdata[25]) +
    ", " + String(zdata[26]) + ", " + String(zdata[27]) +
    ", " + String(zdata[28]) + ", " + String(zdata[29]) +
    ", " + String(zdata[30]) + ", " + String(zdata[31]));
   Particle.publish("zvalues:", zvalues);

  delay(60000);                     // wait 1 minute
}

int configAcc(String command)       // Particle function for accelerometer calibration
{
    if (command == "calibrate")     // if function call = calibrate, then offsets = averages
    {
      xos = xavg;
      yos = yavg;
      zos = zavg;
      return 1;
    }
    else if (command == "reset")    // if function call = reset, then offsets = 0
    {
      xos = 0;
      yos = 0;
      zos = 0;
      return 0;
    }
    else return -1;
}
