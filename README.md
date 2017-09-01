# Industrial-SMS
The Industrial Spectrum Monitoring System (SMS) is an application based on Gnu Radio that monitors the radio frequency (RF) spectrum and reports metrics on discovered signals. These metrics include the frequency, bandwith, and power of discovered signals, with timestamps. 

(How it is intended to be used, the industrial environment targeted)

(Include a more complete description about the basis on Gnu Radio, the hardware tested, limitations, example)

## Installation

(How ton install it, e.g., what Ubuntu version you need, installing Gnu Radio, HackRF, firmware)
### Ubuntu
The SMS has been tested on Ubuntu Linux 14.04 LTS. Installation instructions for Ubuntu can be found on [help.ubuntu.com/14.04/installation-guide](https://help.ubuntu.com/14.04/installation-guide/).

### Gnu Radio
(How to install Gnu Radio)
### HackRF
(How to get and install HackRF)

## Running the SMS

(How to run it, i.e., what you type at the command line)

Run the SMS on the command line like this:
Here is how to run the software:
```
gr-scan -x 2400 -y 2500
```
Options include: 

```
-x <starting frequency>
-y <ending frequency>
```
Then, in another windows, run the monitor: 
```
monitor
```
(Explain how to use it, e.g., the zoomming, other features you can use during a run)

### Sample Output

(include some sample output, and screen shots)

# Principles of Operation
The SMS runs continously, measuring the power at samples in a small frequency range over a short period of time, and computing a fast Fourier transform (FFT) to identify frequency components present. 
(Detail the operation of the SMS, including what is configurable)
* start frequency
* stop frequency
* step size
* scan bandwidth)
* dwell time
* FFT size
## Calibration
(How to calibrate it, with indications to what command-line options or source code variables to set)
## Thresholding
(How thresholding is done to miniimze false alarms, and how it is configured)
## Adataptive Gain Control
(How AGC is done)

# Limitations and Future Work
(Here is where you would show how the SMS would figure as a node in a larger system; what other hardware could be used)
