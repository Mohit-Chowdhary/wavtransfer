#include <iostream>
#include <string>
#include <fstream>
#include <cmath>
#include <vector>
#include <array>
#include <cstdint>
#include <thread>
#include "kissFFT/kiss_fft.h"
#include "AES/aes.h"

const int SAMPLE_RATE = 44100;
const float PI = 3.1415926f;

std::vector<float> generateTone(float freq, float duration){
    int numSamples = SAMPLE_RATE * duration;

    std::vector<float> samples(numSamples);

    for(int i=0; i<numSamples; i++){
        samples[i] = sin(2*PI*freq*i / SAMPLE_RATE);
    }

    return samples;
}

void writeWAV(const std:: string& outFile, const std::vector<float>& samples){
    /*
    RIFF HEADER (12 bytes)
        4 bytes: "RIFF"
        4 bytes: total file size
        4 bytes: "WAVE"

    FMT SUBCHUNK (24 bytes)
        4 bytes: "fmt "
        4 bytes: 16        <- size of fmt data, always 16 for PCM
        2 bytes: 1         <- audio format, 1 = PCM
        2 bytes: 1         <- channels, 1 = mono
        4 bytes: 44100     <- sample rate
        4 bytes: 88200     <- byte rate = samplerate * channels * bitdepth/8
        2 bytes: 2         <- block align = channels * bitdepth/8
        2 bytes: 16        <- bits per sample

    DATA SUBCHUNK
        4 bytes: "data"
        4 bytes: actual audio data size in bytes
        N bytes: raw samples
    */
    std::ofstream file(outFile, std::ios::binary);

    int dataSize = samples.size() * sizeof(int16_t);
    int chunkSize = dataSize + 36;
    int byteRate = SAMPLE_RATE*2;
    int subchunk1Size = 16;
    int16_t blockAlign = 2;
    int16_t bitsPerSample = 16;
    int16_t audioFormat = 1;
    int16_t numChannels = 1;

    file.write("RIFF",4);
    file.write((char*)&chunkSize,4);
    file.write("WAVE",4);
    file.write("fmt ",4);
    file.write((char*)&subchunk1Size,4);
    file.write((char*)&audioFormat,2);
    file.write((char*)&numChannels,2);
    file.write((char*)&SAMPLE_RATE,4);
    file.write((char*)&byteRate,4);
    file.write((char*)&blockAlign,2);
    file.write((char*)&bitsPerSample,2);
    file.write("data",4);
    file.write((char*)&dataSize,4);

    for(float s: samples){
        int16_t pcm = s*32767;      //int16_t max
        file.write((char*)&pcm,2);
    }
}

void encodeToWAV(const std::string& message,const std:: string& outFile){
    std::vector<int> bits;
    for(char c: message){
        for(int i=7; i>=0; i--){
            bits.push_back((c>>i)&1);
        }
    }
    while(bits.size() % 4 != 0) bits.push_back(0);

    std::vector<int> ch[4];
    for(int i=0;i<bits.size();i++){
        ch[i%4].push_back(bits[i]);
    }

    std::vector<float> samples[4];
    std::thread t[4];

    for(int i=0;i<4;i++){
        t[i] = std::thread([&,i](){
            for(int bit: ch[i]){
                float freq = bit ? (2000+i*2000):(1000+i*2000);
                auto chunk = generateTone(freq,0.01f);
                samples[i].insert(samples[i].end(),chunk.begin(),chunk.end());
            }
        });
    }

    for(int i=0; i<4;i++) t[i].join();

    int len = samples[0].size();
    std::vector<float> mixed(len);
    for(int i=0;i<len;i++){
        mixed[i] = (samples[0][i] + samples[1][i] + samples[2][i] + samples[3][i]) / 4.0f;
    }

    writeWAV(outFile, mixed);

}

std::array<int,4> detectBits(std::vector<int16_t>& raw, int pos, int chunkSize){
    kiss_fft_cfg cfg = kiss_fft_alloc(chunkSize, 0,0,0);

    std::vector<kiss_fft_cpx> in(chunkSize);
    std::vector<kiss_fft_cpx> out(chunkSize);

    for(int i=0;i<chunkSize;i++){
        in[i].r = raw[pos+i] / 32767.0f;
        in[i].i = 0;
    }

    kiss_fft(cfg,in.data(),out.data());

    //Nyquist limit??
    std:: array<int,4> bits;
    for(int ch=0; ch<4; ch++){
        int bin1000 = (1000 +ch*2000) * chunkSize/SAMPLE_RATE;
        int bin2000 = (2000 +ch*2000) * chunkSize/SAMPLE_RATE;

        float mag1 = sqrt(out[bin1000].r*out[bin1000].r + out[bin1000].i*out[bin1000].i);
        float mag2 = sqrt(out[bin2000].r*out[bin2000].r + out[bin2000].i*out[bin2000].i);

        bits[ch] = mag2>mag1?1:0;
    }
    kiss_fft_free(cfg);
    return bits;
}

std::string decodeFromWAV(std::string inFile){
    std::ifstream file(inFile,std::ios::binary);

    file.seekg(44);

    std::vector<int16_t> raw;
    int16_t sample;
    while(file.read((char*)&sample,2)){
        raw.push_back(sample);
    }

    std:: vector<int> allBits;
    int chunkSize =  441;

    for(int pos = 0; pos+chunkSize<=raw.size(); pos+=chunkSize){
        auto bits = detectBits(raw,pos,chunkSize);

        allBits.push_back(bits[0]);
        allBits.push_back(bits[1]);
        allBits.push_back(bits[2]);
        allBits.push_back(bits[3]);
    }

    std::string result = "";
    
    int bitBuffer = 0;
    int bitCount = 0;

    for(auto bit:allBits){
        bitBuffer = (bitBuffer<<1) | bit;
        bitCount++;
        if(bitCount==8){
            result+=(char)bitBuffer;
            bitBuffer = 0;
            bitCount = 0;
        }
    }

    return result;
}

int main(){
    std::cout<<"starting\nEnter string: ";
    std::string s;
    std::getline(std::cin, s);
    std::string cipher = aesEncrypt(s, "mysecretkey12345");
    encodeToWAV(cipher,"output.wav");
    std::cout<<"encoded\n";
    std::string text = decodeFromWAV("output.wav");
    std:: string pt = aesDecrypt(text,"mysecretkey12345");
    std::cout<<"decoded: "<<pt<<"\n";
    return 0;
}



/*
Move AES key to .env
use std::getenv()
remove hardcoded key

Add packet structure
preamble/header
payload length
checksum/hash

Add SHA-256 or CRC
verify message integrity after decode

Add Hamming code
start with Hamming(7,4)
encode before transmission
decode + correct after reception

Add windowing before FFT
Hann/Hamming window
reduce spectral leakage

Improve frequency detection
search nearby FFT bins
don’t rely on exact bin only

Add tolerance thresholds
avoid random noise being decoded as bits

Preallocate vectors
reduce repeated reallocations
optimize inserts
*/