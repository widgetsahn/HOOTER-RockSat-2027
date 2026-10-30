# This directory is broken down by the following:

**codecs/**: contains methods for encoding/decoding.

**common_methods/**:  error-correction codes, each with encode and decode in
one file pair

**decoder/**: receive pipeline. Runs on the ground station PC

**encoder/**: transmit pipeline. Runs on  ESP32 and builds frames from sensor.

**tests/**: contains test data and test scripts.

**AA_block_diagram.pptx**: contains a conceptual block diagram laying out the
design behind the downlink, from data acquisition to unpacking the data on land
