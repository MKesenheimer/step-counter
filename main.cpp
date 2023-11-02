#include <stdio.h>
#include <stdint.h>
#include <string>
#include <sstream>
#include <iostream>
#include <time.h>
#include "logo.hpp"
#include "GFX.hpp"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"

#include <vector>
#include "fft.h"

// By default these devices  are on bus address 0x68
static int addr = 0x68;

static void mpu6050_reset() {
    // Two byte reset. First byte register, second byte data
    // There are a load more options to set up the device in different ways that could be added here
    uint8_t buf[] = {0x6B, 0x80};
    i2c_write_blocking(i2c_default, addr, buf, 2, false);
}

static void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp) {
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.

    uint8_t buffer[6];

    // Start reading acceleration registers from register 0x3B for 6 bytes
    uint8_t val = 0x3B;
    i2c_write_blocking(i2c_default, addr, &val, 1, true); // true to keep master control of bus
    i2c_read_blocking(i2c_default, addr, buffer, 6, false);

    for (int i = 0; i < 3; i++) {
        accel[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
    }

    // Now gyro data from reg 0x43 for 6 bytes
    // The register is auto incrementing on each read
    val = 0x43;
    i2c_write_blocking(i2c_default, addr, &val, 1, true);
    i2c_read_blocking(i2c_default, addr, buffer, 6, false);  // False - finished with bus

    for (int i = 0; i < 3; i++) {
        gyro[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);;
    }

    // Now temperature from reg 0x41 for 2 bytes
    // The register is auto incrementing on each read
    val = 0x41;
    i2c_write_blocking(i2c_default, addr, &val, 1, true);
    i2c_read_blocking(i2c_default, addr, buffer, 2, false);  // False - finished with bus

    *temp = buffer[0] << 8 | buffer[1];
}

clock_t clock()
{
    return (clock_t) time_us_64() / 1000000;
}

using namespace math;

int main() {
    // setup
    stdio_init_all();

    // Initialize I2C on i2c1 port with 400kHz
    i2c_init(i2c1, 400000);
    gpio_set_function(2, GPIO_FUNC_I2C);
    gpio_set_function(3, GPIO_FUNC_I2C);
    gpio_pull_up(2);
    gpio_pull_up(3);

    // Declare oled instance
    GFX oled(0x3C, size::W128xH64, i2c1);

    // Display bitmap
    oled.display(logo);

    // This example will use I2C0 on the SDA and SCL pins 20 and 21
    i2c_init(i2c0, 400000);
    gpio_set_function(20, GPIO_FUNC_I2C);
    gpio_set_function(21, GPIO_FUNC_I2C);
    gpio_pull_up(20);
    gpio_pull_up(21);

    mpu6050_reset();

    int16_t acceleration[3], gyro[3], temp;
    const size_t numberOfChannels = 256;
    std::vector<double> accbuf(numberOfChannels, 0);
    std::vector<double> accbufI(numberOfChannels, 0);
    size_t pos = 0, maxI = 0;
    size_t counter = 0;
    double executionTime = 0;
    clock_t startTime = clock(), endTime;
    double measTime = 0;
    double charactFreq = 0;
    const double moveThreshold = 50;
    const double accThreshold = 11;
    unsigned int steps = 0;
    unsigned int ds1 = 0;
    unsigned int ds2 = 0;
    bool trigger = false;
    double maxv = 0;
    while(true) {
        mpu6050_read_raw(acceleration, gyro, &temp);
        double ax = (double)acceleration[0] / 2487;
        double ay = (double)acceleration[1] / 2487;
        double az = (double)acceleration[2] / 2487;

        /*
        // These are the raw numbers from the chip, so will need tweaking to be really useful.
        // See the datasheet for more information
        printf("Acc. X = %d, Y = %d, Z = %d\n", acceleration[0], acceleration[1], acceleration[2]);
        printf("Gyro. X = %d, Y = %d, Z = %d\n", gyro[0], gyro[1], gyro[2]);
        // Temperature is simple so use the datasheet calculation to get deg C.
        // Note this is chip temperature.
        printf("Temp. = %f\n", (temp / 340.0) + 36.53);
        */

        //sleep_ms(1000);
        // clear buffer
        oled.clear();
        std::stringstream sout;
        sout << "Acc. x = " << ax;
        oled.drawString(0, 0, sout.str());
        sout.str(std::string());

        sout << "Acc. y = " << ay;
        oled.drawString(0, 10, sout.str());
        sout.str(std::string());

        sout << "Acc. z = " << az;
        oled.drawString(0, 20, sout.str());
        sout.str(std::string());

        double aabs = std::pow(std::pow(ax, 2) + std::pow(ay, 2) + std::pow(az, 2), 0.5);
        
        // count a step if the acceleration is above a certain threshold
        if (!trigger && aabs >= accThreshold) {
            ds2++;
            trigger = true;
        }
        if (trigger && aabs < accThreshold) {
            trigger = false;
        }

        // store the acceleration data to later calculate the fourier transformation
        accbuf[counter++] = aabs;

        // end of interval: do the calculation
        if (counter >= numberOfChannels) {
            endTime = clock();
            // measurement time of 'numberOfChannels'
            measTime = (double)(endTime - startTime);

//#define DEBUG
#ifdef DEBUG
            std::cout << "data:" << std::endl;
            for (const auto& v : accbuf) std::cout << v << ", ";
            std::cout << std::endl;
#endif

            // calculate the fourier transform (the results are stored in accbuf (real part) and accbufI (imaginary part))
            accbufI.assign(numberOfChannels, 0);
            utilities::FFT::fft(accbuf, accbufI);
            for (size_t i = 0; i < numberOfChannels; ++i)
                accbuf[i] = std::pow(std::pow(accbuf[i], 2) + std::pow(accbufI[i], 2), 0.5);

#ifdef DEBUG
            std::cout << "fft:" << std::endl;
            for (const auto& v : accbuf) std::cout << v << ", ";
            std::cout << std::endl;
#endif

            // determine the position of the maximum element (leave out the first elements)
            // only consider frequency in a certain range (low- and high-pass filter)
            size_t windowLow  = (size_t)std::max<double>(0.5 * measTime, 2);
            size_t windowHigh = (size_t)std::min<double>(10 * measTime, numberOfChannels / 2);
            const auto maxi = std::max_element(accbuf.begin() + windowLow, accbuf.begin() + windowHigh);
            maxv = *maxi;

            // if the strength of the frequency is above a certain threshold
            if (maxv > moveThreshold && measTime > 0) {
                ds1 = std::distance(accbuf.begin(), maxi);
                charactFreq = ds1 / measTime;
            } else {
                ds1 = 0;
                charactFreq = 0;
            }

             if (std::abs<double>(ds1 - ds2) / ds1 < 0.1) {
                // the step increments from the different algorithms differ not much
                // -> the signal is periodically und the steps probably valid
                steps += ds1;
            } else if (ds2 < ds1) {
                // the signal is not periodically
                steps += ds2;
            }

#ifdef DEBUG
            std::cout << "steps = " << steps << std::endl;
            std::cout << "freq = " << charactFreq << std::endl;
            std::cout << "maxv = " << maxv << std::endl;
            std::cout << "pos = " << pos << std::endl;
            std::cout << "time = " << measTime << std::endl;
            std::cout << std::endl;
#endif

            ds2 = 0;
            startTime = endTime;
            counter = 0;
        }

        sout << "ds = (" << ds1 << ", " << ds2 << ")";
        oled.drawString(0, 30, sout.str());
        sout.str(std::string());

        sout << "maxv = " << maxv;
        oled.drawString(0, 40, sout.str());
        sout.str(std::string());

        sout << "steps = " << steps;
        oled.drawString(0, 50, sout.str());
        sout.str(std::string());

        //oled.drawProgressBar(0, oled.getHeight() - 5, oled.getWidth(), 5, rand() % 100 + 1);
        // send buffer to screen
        oled.display();   
    }
    return 0;
}