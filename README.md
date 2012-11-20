readerboard
===========

Some programmable LED signs have proprietary protocols, relegating these
otherwise fine and useful pieces of hardware to the scrapheap. This project
aims to replace the hardware and software in these signs, giving them new
life.

![Readerboard running Church of Robotron](https://raw.github.com/sharebrained/readerboard/master/documentation/robotron_readerboard.jpg)

Go troll eBay, buy a "broken" LED sign, and join the party!

Sub-Projects
============

* hardware/readerboard_avr8: Atmel AVR8-based replacement for Data Display
  Corporation controller board.

* firmware/readerboard_avr8: Atmel AVR8-based firmware for USB control of the
  hardware/readerboard_avr8 board.
  
* software/readerboard.py: Example host software for controlling the
  readerboard_avr8 hardware. This was the code that ran at the Church of
  Robotron at ToorCamp 2012.

Status
======

The AVR8-related board, firmware, and software have all been tested and work
well.

Requirements
============

* [EAGLE 5.11 Freeware, Light, Hobbyist, Standard, or Professional]
  (http://www.cadsoftusa.com/shop/pricing/)

Other Projects / Prior Art
==========================

[http://noisybox.net/electronics/LED_sign/]
[http://churchofrobotron.com/]

License
=======

Each sub-project specifies its own license.

Contact
=======

ShareBrained Technology, Inc.

<http://www.sharebrained.com/>
