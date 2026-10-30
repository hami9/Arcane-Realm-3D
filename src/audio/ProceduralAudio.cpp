#include "audio/ProceduralAudio.h"
#include "core/GameContext.h"

#include <cmath>
#include <cstdlib>

namespace {
Sound GenSound(int sampleRate, float duration, float (*generator)(float t, float p), float param = 0.0f) {
    int totalSamples = (int)(sampleRate * duration);
    short* data = (short*)malloc(totalSamples * sizeof(short));
    for (int i = 0; i < totalSamples; i++) {
        float t = (float)i / (float)sampleRate;
        float sample = generator(t, param);
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        data[i] = (short)(sample * 32000.0f);
    }
    Wave wave{};
    wave.frameCount = totalSamples;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;
    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return snd;
}

float WaveFireball(float t, float /*param*/) {
    float freq = 200.0f + 700.0f * (1.0f - t / 0.22f);
    float env = expf(-t * 9.0f);
    float s = sinf(2.0f * PI * freq * t) * 0.6f + sinf(2.0f * PI * (freq * 1.5f) * t) * 0.3f;
    float noise = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 0.25f;
    return (s + noise) * env;
}

float WaveExplosion(float t, float /*param*/) {
    float env = expf(-t * 5.0f);
    float noise = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f);
    float boom = sinf(2.0f * PI * (80.0f * expf(-t * 6.0f)) * t);
    return (boom * 0.65f + noise * 0.45f) * env;
}

float WaveFrost(float t, float /*param*/) {
    float env = expf(-t * 6.0f);
    float s1 = sinf(2.0f * PI * 1320.0f * t);
    float s2 = sinf(2.0f * PI * 1760.0f * t);
    float s3 = sinf(2.0f * PI * 2640.0f * t);
    return (s1 + s2 * 0.7f + s3 * 0.4f) * 0.4f * env;
}

float WaveThunder(float t, float /*param*/) {
    float env = expf(-t * 4.0f);
    float crack = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f);
    float rumble = sinf(2.0f * PI * (65.0f * expf(-t * 2.0f)) * t) * 0.8f;
    if (t < 0.05f) return (crack * 0.9f) * env;
    return (rumble + crack * 0.3f) * env;
}

float WaveBlink(float t, float /*param*/) {
    float freq = 900.0f - 600.0f * (t / 0.18f);
    float env = sinf(PI * (t / 0.18f));
    return sinf(2.0f * PI * freq * t) * env * 0.7f;
}

float WaveGem(float t, float /*param*/) {
    float freq = (t < 0.06f) ? 1046.5f : 1567.98f;
    float env = expf(-fmodf(t, 0.06f) * 20.0f);
    return sinf(2.0f * PI * freq * t) * env * 0.5f;
}

float WaveLevelUp(float t, float /*param*/) {
    float freqs[4] = { 523.25f, 659.25f, 783.99f, 1046.50f };
    int step = (int)(t / 0.12f);
    if (step > 3) step = 3;
    float env = expf(-fmodf(t, 0.12f) * 8.0f);
    return sinf(2.0f * PI * freqs[step] * t) * env * 0.6f;
}

float WaveHurt(float t, float /*param*/) {
    float env = expf(-t * 12.0f);
    float s = sinf(2.0f * PI * (140.0f - t * 400.0f) * t);
    return s * env * 0.8f;
}

float WaveButtonClick(float t, float /*param*/) {
    float env = expf(-t * 22.0f);
    return sinf(2.0f * PI * 880.0f * t) * env * 0.4f;
}

} // namespace

void InitializeGameAudio() {
    InitAudioDevice();
    G.sndFireball  = GenSound(44100, 0.22f, WaveFireball);
    G.sndExplosion = GenSound(44100, 0.55f, WaveExplosion);
    G.sndFrost     = GenSound(44100, 0.35f, WaveFrost);
    G.sndThunder   = GenSound(44100, 0.55f, WaveThunder);
    G.sndBlink     = GenSound(44100, 0.20f, WaveBlink);
    G.sndGem       = GenSound(44100, 0.14f, WaveGem);
    G.sndLevelUp   = GenSound(44100, 0.65f, WaveLevelUp);
    G.sndHurt      = GenSound(44100, 0.18f, WaveHurt);
    G.sndClick     = GenSound(44100, 0.08f, WaveButtonClick);

}

void ShutdownGameAudio() {
    UnloadSound(G.sndFireball);
    UnloadSound(G.sndExplosion);
    UnloadSound(G.sndFrost);
    UnloadSound(G.sndThunder);
    UnloadSound(G.sndBlink);
    UnloadSound(G.sndGem);
    UnloadSound(G.sndLevelUp);
    UnloadSound(G.sndHurt);
    UnloadSound(G.sndClick);
    CloseAudioDevice();
}
