# Lightboard
Lightboard is a software to build an electronic whiteboard with a Wiimote and an IR pen.
We are working to remake Lightboard from zero, with a new engine, new graphic, new features and to make it more compatible and flexible.

### Why Lightboard?
Old Lightboard is my degree project, born from a consideration from Renzo Davoli according to which python-whiteboard was too heavy for low-resources computers (e.g. Raspberry Pi).

The experiments confirmed the hypothesis, python-whiteboard is unusable on Raspberry Pi.

After many studies about how to improve the software, we decide to create a new engine in C to make the numerous calculations in a more efficient way.
Old Lightboard uses 24.7% of the CPU and 29.5% of the RAM, compared to python-whiteboard.

But Old Lightboard has many defects because of the fast and inexperienced development.
So, with financial help from [ILS](http://www.ils.org/) I can work on a new version of Lightboard, with its interface, new features and more.

Lightboard migrates from cwiid to xwiimote for a better resources management, single thread engine execution and more support.

Old project: [here](https://github.com/GiovanniIncammicia/old_lightboard)

### HOW TO USE

Dependencies:
* qt4-default
* libxtst-dev
* libbluetooth-dev
* libudev-dev
* libxwiimote-dev
* bluez-tools
* bluez-utils
* bluez-compat

1. open a terminal and move to Lightboard directory, launch "qmake && make"  
2. execute Lightboard with "sudo ./lightboard"


Note that sudo is necessary because xwiimote library uses udev system and talks with Wiimote at kernel level.

For every request, bug report, please open an issue [here](https://github.com/GiovanniIncammicia/lightboard/issues).

E-mail: giovanni.incammicia@gmail.com
