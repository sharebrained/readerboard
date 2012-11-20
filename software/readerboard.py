#!/usr/bin/env python

# Copyright 2012 ShareBrained Technology, Inc.
#
# This file is part of readerboard.
#
# readerboard is free software: you can redistribute
# it and/or modify it under the terms of the GNU General
# Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# readerboard is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE. See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General
# Public License along with readerboard. If not, see
# <http://www.gnu.org/licenses/>.

import usb
import time
import struct
import csv

class Readerboard(object):
    led_req_type = (0 << 7) | (2 << 5) | (0 << 0)
    
    def __init__(self):
        self.device = usb.core.find(idVendor=0x8080, idProduct=0x6464)
        self.device.set_configuration()
        self.cfg = self.device.get_active_configuration()
        self.intf = self.cfg[(0, 0)]
        self.back_buffer = 0

    def set_line(self, row, data_r, data_g, buffer_n=None):
        buffer_n = self.back_buffer if buffer_n is None else buffer_n
        if buffer_n not in (0, 1):
            raise RuntimeError("set_line: invalid buffer_n value: " + buffer_n)
        data = struct.pack("BB", 0, row) + data_r
        self.device.ctrl_transfer(self.led_req_type, buffer_n, 0, 0, data)
        #self.device.ctrl_transfer(self.led_req_type, buffer_n, 0x100 | row, 0, data_g)

    def show_buffer(self, buffer_n=None):
        buffer_n = self.back_buffer if buffer_n is None else buffer_n
        self.device.ctrl_transfer(self.led_req_type, 2, buffer_n, 0)
        self.back_buffer = 1 - buffer_n

    def clear_buffer(self, buffer_n=None):
        buffer_n = self.back_buffer if buffer_n is None else buffer_n
        self.device.ctrl_transfer(self.led_req_type, 3, buffer_n, 0)
    
    def draw_text(self, x, y, message, buffer_n=None):
        buffer_n = self.back_buffer if buffer_n is None else buffer_n
        data = struct.pack("BB", x, y) + message
        self.device.ctrl_transfer(self.led_req_type, 4, buffer_n, 0, data)
        
    def scroll_left(self, frames_per_pixel, pixel_count, buffer_n=None):
        buffer_n = self.back_buffer if buffer_n is None else buffer_n
        data = struct.pack("BB", frames_per_pixel, pixel_count)
        self.device.ctrl_transfer(self.led_req_type, 5, buffer_n, 0, data)

    def scroll_right(self, frames_per_pixel, pixel_count, buffer_n=None):
        buffer_n = self.back_buffer if buffer_n is None else buffer_n
        data = struct.pack("BB", frames_per_pixel, pixel_count)
        self.device.ctrl_transfer(self.led_req_type, 6, buffer_n, 0, data)

def read_leaderboard():
    f = csv.reader(open('/home/mutant/mcor/leaderboard/data/leaderboard.txt', 'r'))
    result = []
    for row in f:
        d = {
            'initials': row[0],
            'score': int(row[1]),
        }
        result.append(d)
    return tuple(sorted(result, lambda x, y: cmp(y['score'], x['score'])))

def message_sequence(board, score_data):
    board.clear_buffer()
    board.draw_text(9, 0, "CHURCH OF ROBOTRON")
    board.show_buffer()
    time.sleep(1.0)
    board.scroll_left(0, 120)
    time.sleep(3.0)
    
    board.clear_buffer()
    board.draw_text(32, 0, "INSERT COIN")
    board.show_buffer()
    time.sleep(2.0)

    for i in range(5):
        board.clear_buffer()
        board.draw_text(0, 0, "PREPARE FOR JUDGEMENT")
        board.show_buffer()
        time.sleep(0.3)
        board.clear_buffer()
        board.show_buffer()
        time.sleep(0.3)

    board.clear_buffer()
    board.show_buffer()
    time.sleep(1.0)

    if score_data:    
        board.clear_buffer()
        board.draw_text(24, 0, "MUTANT SAVIOR")
        board.show_buffer()
        time.sleep(2.0)
    
        board.clear_buffer()
        board.draw_text(24, 0, "TOP CANDIDATE")
        board.show_buffer()
        time.sleep(2.0)
    
        board.clear_buffer()
        d = score_data[0]
        board.draw_text(32, 0, "%(score)s %(initials)s" % d)
        board.show_buffer()
        time.sleep(2.0)
        board.scroll_right(0, 120)
        time.sleep(3.0)

score_data = None

while True:
    try:
        board = Readerboard()
        #score_data = read_leaderboard()
        message_sequence(board, score_data)
    except Exception, e:
        print(e)
        time.sleep(5.0)

