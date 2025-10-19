#   _    _ _______   __  __ _____ _____ _____   ____  _   _
#  | |  | |__   __| |  \/  |_   _/ ____|  __ \ / __ \| \ | |
#  | |__| |  | |    | \  / | | || |    | |__) | |  | |  \| |
#  |  __  |  | |    | |\/| | | || |    |  _  /| |  | | . ` |
#  | |  | |  | |    | |  | |_| || |____| | \ \| |__| | |\  |
#  |_|  |_|  |_|    |_|  |_|_____\_____|_|  \_\\____/|_| \_|
#  =================== Advanced R&D ========================

#  Copyright (c) 2023 HT Micron Semicondutores S.A.
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#  http://www.apache.org/licenses/LICENSE-2.0
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

# file: debug.py
# brief: Debug script used when VSCODE debugger is running. 
# author: HT Micron Advanced R&D
# link: https://github.com/htmicron
# version: 0.1
# date: February 23, 2023

import os
import sys
import serial.tools.list_ports

def replace_line(file_name, line_num, text):
    lines = open(file_name, 'r').readlines()
    lines[line_num] = text
    out = open(file_name, 'w')
    out.writelines(lines)
    out.close()

def get_bin_name():
    text = "syspath = ../Build/{}/{}.bin\n".format(sys.argv[1], sys.argv[1])
    return text

def get_com_port():
    try:
        with open('Debug/COM.txt', 'r+') as file:
            com_port = file.read()
            file.close()
        
            return com_port
    except FileNotFoundError:
        os.system("python Debug/Scripts/GUI.py")
        
        with open('Debug/COM.txt', 'r') as file:
            com_port = file.read()
            file.close()

        return com_port

def com_port_exists(port):

    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        if p.device == port:
            return True
    return False

if __name__ =="__main__":

    os.system("make.exe -j4 gccall TARGET=qcx212_0h00 PROJECT={}".format(sys.argv[1]))
    replace_line("Debug/htnb32l.ini", 11, get_bin_name())

    com_port = get_com_port()

    if com_port_exists(com_port):
        os.system(".\Debug\FlasherCLI.exe --cfgfile Debug/htnb32l.ini --port {} burnbatch --imglist bootloader system".format(com_port))
    else:
        os.system("python Debug/Scripts/GUI.py")        
        com_port = get_com_port()
        os.system(".\Debug\FlasherCLI.exe --cfgfile Debug/htnb32l.ini --port {} burnbatch --imglist bootloader system".format(com_port))


