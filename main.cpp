#include <iostream>
#include <string>
#include <fstream>
#include <cmath>
#include <vector>
#include <cstdint>
#include "kissFFT/kiss_fft.h"

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
    std::vector<float>  allSamples;

    for(char c: message){
        for(int i =7; i>=0; i--){
            int bit = (c>>i) &1;
            float freq = bit?2000.0f:1000.0f;

            auto chunk = generateTone(freq,0.1f);
            allSamples.insert(allSamples.end(),chunk.begin(),chunk.end());
        }
    }
    writeWAV(outFile, allSamples);

}

int detectBit(std::vector<int16_t> raw, int pos, int chunkSize){
    kiss_fft_cfg cfg = kiss_fft_alloc(chunkSize, 0,0,0);

    std::vector<kiss_fft_cpx> in(chunkSize);
    std::vector<kiss_fft_cpx> out(chunkSize);

    for(int i=0;i<chunkSize;i++){
        in[i].r = raw[pos+i] / 32767.0f;
        in[i].i = 0;
    }

    kiss_fft(cfg,in.data(),out.data());

    //Nyquist limit??
    int bin1000 = 1000* chunkSize/SAMPLE_RATE;
    int bin2000 = 2000* chunkSize/SAMPLE_RATE;

    float mag1 = sqrt(out[bin1000].r*out[bin1000].r + out[bin1000].i*out[bin1000].i);
    float mag2 = sqrt(out[bin2000].r*out[bin2000].r + out[bin2000].i*out[bin2000].i);

    return mag2>mag1?1:0;
}

std::string decodeFromWAV(std::string inFile){
    std::ifstream file(inFile,std::ios::binary);

    file.seekg(44);

    std::vector<int16_t> raw;
    int16_t sample;
    while(file.read((char*)&sample,2)){
        raw.push_back(sample);
    }

    std::string result = "";
    int chunkSize =  4410;
    int bitBuffer = 0;
    int bitCount = 0;

    for(int pos = 0; pos+chunkSize<=raw.size(); pos+=chunkSize){
        int bit = detectBit(raw,pos,chunkSize);

        bitBuffer = (bitBuffer<<1) | bit;
        bitCount++;

        if(bitCount == 8){
            result += (char)bitBuffer;
            bitBuffer = 0;
            bitCount = 0;
        }
    }

    return result;
}

int main(){

    std::cout << std::flush;
    std::cout<<"starting\n";
    encodeToWAV("hello","output.wav");
    std::cout<<"encoded\n";
    std::string waht = decodeFromWAV("output.wav");
    std::cout<<waht<<"\n";
    std::cout<<"done\n";
    return 0;
}