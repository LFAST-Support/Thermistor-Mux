"""
/*******************************************************************************
Copyright 2021
Steward Observatory Engineering & Technical Services, University of Arizona

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*******************************************************************************
Author: Rory Scobie (scobier@email.arizona.edu)
Brief: Sparkplug/MQTT client to test the VCM nodes.  Capable of reading data
published from them, and publishing commands to them.

Adapted from python client example at https://github.com/eclipse/tahu
"""

import time
import datetime
import threading
import sys
import random
import csv


#import paho.mqtt.client as mqtt
#from sparkplug_b import *

# Application constants
APP_VERSION             = '2.4'
COMMS_VERSION           = 2
COMMS_VERSION_METRIC    = 'Properties/Communications Version'
BIRTH_DEATH_SEQ_METRIC  = 'bdSeq'
TESTBENCH_STATUS_METRIC = 'Properties/Test Bench Status'
NODE_ID                 = 'ADC'
TEST_BENCH_DEVICE_ID    = 'TESTBENCH'
NUM_MODULES             = 6
NUM_ADCS                = 12
NUM_DACS                = 12
MIN_DAC_VOLTAGE         = 0.0
MAX_DAC_VOLTAGE         = 1.0
DEFAULT_BROKER_URL      = '192.168.1.91'
DEFAULT_BROKER_PORT     = 1883
DEFAULT_MODULE_ID       = 0
SHOW_OPTIONS            = [ 'none', 'errors', 'topic', 'changed', 'all' ]

module_is_alive      = False
compatible_version   = False
gui_controls_created = False
message_seq          = 0

date_string = datetime.datetime.now().strftime( '%Y-%m-%d' )
LOG_FILENAME = f'vcm_test_log_{date_string}.csv'


# This package isn't available on all platforms, so only attempt to load it
# if running the GUI
from PySide6.QtCore import *
from PySide6.QtWidgets import *
from PySide6.QtGui import *

# Return an hbox with the specified widget centered horizontally within it
def center_widget( widget ):
    h_box = QWidget()
    h_box_layout = QHBoxLayout()
    h_box.setLayout( h_box_layout )
    h_box_layout.addWidget( QLabel( '' ), 10 )
    h_box_layout.addWidget( widget, 0 )
    h_box_layout.addWidget( QLabel( '' ), 10 )
    return h_box

app = QApplication( [] )
main_window = QMainWindow()
main_window.setWindowTitle( f'Thermistor Mux Client v{APP_VERSION} - Module {option_module_id}' )

window = QWidget()
v_box_layout = QVBoxLayout()
v_box_layout.setSpacing( 0 )
window.setLayout( v_box_layout )

main_window.setCentralWidget( window )

# The Change Module button
change_module_controls = QWidget()
h_box_layout = QHBoxLayout()
change_module_controls.setLayout( h_box_layout )
change_module_input = QLineEdit( f'{option_module_id}' )
only_int = QIntValidator()
change_module_input.setValidator( only_int )
h_box_layout.addWidget( change_module_input )
change_module_button = QPushButton( 'Change Module' )
change_module_button.clicked.connect( change_module_button_handler )
h_box_layout.addWidget( change_module_button )
v_box_layout.addWidget( center_widget( change_module_controls ) )

# The Reboot button
reboot_button = QPushButton( 'Reboot' )
reboot_button.clicked.connect( reboot_button_handler )
v_box_layout.addWidget( center_widget( reboot_button ) )

# Data received from the module
outputs = QWidget()
output_grid = QGridLayout()
outputs.setLayout( output_grid )

# Node data
output_grid.addWidget( QLabel( 'Name' ),      0, 0 )
output_grid.addWidget( QLabel( 'Timestamp' ), 0, 1 )
output_grid.addWidget( QLabel( 'Value' ),     0, 2 )
name_outputs  = QLabel( '' )
time_outputs  = QLabel( '' )
value_outputs = QLabel( '' )
output_grid.addWidget( name_outputs,  1, 0 )
output_grid.addWidget( time_outputs,  1, 1 )
output_grid.addWidget( value_outputs, 1, 2 )


# The logging controls
log_button = QPushButton( '' )
log_button.clicked.connect( log_button_handler )
v_box_layout.addWidget( center_widget( log_button ) )
if option_log:
log_button.setText( 'Toggle Logging Off' )
else:
log_button.setText( 'Toggle Logging On' )

# The Test Bench controls
test_bench_label = QLabel( 'Test Bench Controls' )
test_bench_label.setStyleSheet( 'font-weight: bold; text-decoration: underline; font-size: 16;' )
v_box_layout.addWidget( center_widget( test_bench_label ) )

# Controls to set the DAC voltages
set_DAC_label = QLabel( f'Select DAC and voltage to set' )
v_box_layout.addWidget( center_widget( set_DAC_label ) )
set_DAC_controls = QWidget()
set_DAC_layout = QGridLayout()
set_DAC_controls.setLayout( set_DAC_layout )

# DAC Number entry
set_DAC_layout.addWidget( QLabel( 'DAC Number' ),   0, 0 )
set_DAC_number_input = QLineEdit( '' )
set_DAC_layout.addWidget( set_DAC_number_input,     0, 1 )
set_DAC_layout.addWidget( QLabel( f'(0-{NUM_DACS - 1}, or "all" for all DACs)' ), 0, 2 )

# DAC Voltage entry
set_DAC_layout.addWidget( QLabel( 'DAC Voltage' ),  1, 0 )
set_DAC_voltage_input = QLineEdit( '1.0' )
set_DAC_layout.addWidget( set_DAC_voltage_input,    1, 1 )
set_DAC_layout.addWidget( QLabel( f'({MIN_DAC_VOLTAGE:.1f} to {MAX_DAC_VOLTAGE:.1f}, or "random")' ), 1, 2 )

# Set DAC button
set_DAC_button = QPushButton( 'Set DAC' )
set_DAC_button.clicked.connect( set_DAC_button_handler )
set_DAC_layout.addWidget( set_DAC_button,           2, 1 )
v_box_layout.addWidget( center_widget( set_DAC_controls ) )

# The diagnostic text view
diagnostic_text = QPlainTextEdit()
diagnostic_text.setReadOnly( True )
diagnostic_text.setMaximumBlockCount( 500 )
normal_format = diagnostic_text.currentCharFormat()
error_format = QTextCharFormat()
error_format.setForeground( Qt.red )
v_box_layout.addWidget( diagnostic_text )

gui_controls_created = True

# Populate the data widgets with the full list of metrics
display_metrics( None, None, False )

# Run the GUI
main_window.show()
exit_code = app.exec()
close_thread = True
sys.exit( exit_code )

