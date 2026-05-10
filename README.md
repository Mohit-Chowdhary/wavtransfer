# WAVTransfer — FSK Audio Steganography in C++

## What It Does
Encodes encrypted text into audio frequencies, transmits as a WAV file,
and decodes it back to the original message.

## Pipeline
text → AES-128 encrypt → FSK encode → WAV file → FSK decode → AES decrypt → text

## How It Works

### FSK Modulation
Each bit is encoded as a sine wave tone:
- `0` → 1000 Hz
- `1` → 2000 Hz

Each tone is 0.1 seconds at 44100 Hz sample rate.
This is the same principle used in dial-up modems (DTMF/FSK).

### FFT Decoding
The decoder splits the WAV into 4410-sample chunks.
KissFFT runs on each chunk to detect the dominant frequency bin,
recovering the original bit stream.

### AES-128 Encryption
Implemented from scratch in C++ including:
- Key expansion (10 rounds)
- SubBytes via S-box
- ShiftRows
- MixColumns via Galois Field GF(2^8) arithmetic
- AddRoundKey

## Build
```bash
g++ main.cpp AES/aes.cpp kissFFT/kiss_fft.c -o main.exe -static
./main.exe
```

## Dependencies
- KissFFT (included) — FFT computation
- C++ stdlib only

## Usage
Run the program, enter a string, it encodes to output.wav and decodes back.

## Concepts Demonstrated
- FSK modulation/demodulation
- FFT frequency analysis
- AES-128 from scratch
- WAV file format (binary I/O)
- Signal processing in C++
