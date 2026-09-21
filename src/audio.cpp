#include "audio.hpp"
#include <SDL_mixer.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {

// =============================================================================
// Constants
// =============================================================================

constexpr double PI = 3.14159265358979323846;

constexpr int DEFAULT_SAMPLE_RATE = 48000;

constexpr float CLIP_PEAK = 0.82f;
constexpr float MIX_LIMIT = 0.95f;

constexpr float STEP_DISTANCE = 0.85f;

// =============================================================================
// Lightweight random generator
//
// Avoids depending on random()/rand() for audio variation.
// This is only used for harmless sound variation, not game logic.
// =============================================================================

std::uint32_t audioRng = 0x91e10da5u;

std::uint32_t nextRandom()
{
    audioRng ^= audioRng << 13;
    audioRng ^= audioRng >> 17;
    audioRng ^= audioRng << 5;

    return audioRng;
}

float random01()
{
    return static_cast<float>(
        nextRandom() & 0xffffu
    ) / 65535.0f;
}

float randomRange(float minValue, float maxValue)
{
    return minValue +
        (maxValue - minValue) *
        random01();
}

// =============================================================================
// Soft limiter
//
// Normal levels pass unchanged.
// Only peaks approaching clipping are compressed.
// =============================================================================

float softLimit(float value)
{
    const float absolute =
        std::abs(value);

    if (absolute <= 0.85f)
        return value;

    const float excess =
        absolute - 0.85f;

    const float compressed =
        0.85f +
        0.10f *
        (
            1.0f -
            std::exp(
                -excess / 0.10f
            )
        );

    return std::copysign(
        std::min(compressed, MIX_LIMIT),
        value
    );
}

// =============================================================================
// Procedural synthesizer
// =============================================================================

struct Synth {

    std::vector<float> samples;

    int rate = DEFAULT_SAMPLE_RATE;

    std::uint32_t rng = 0x53ac19u;

    Synth(float seconds, int sampleRate)
        : samples(
            static_cast<std::size_t>(
                std::max(
                    0.0f,
                    seconds
                ) *
                sampleRate
            ),
            0.0f
        ),
          rate(sampleRate)
    {
    }

    // -------------------------------------------------------------------------
    // Noise generator
    // -------------------------------------------------------------------------

    float noise()
    {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;

        return
            static_cast<float>(
                rng & 0xffffu
            ) /
            32767.5f -
            1.0f;
    }

    // -------------------------------------------------------------------------
    // Sine tone with optional frequency sweep and second harmonic
    // -------------------------------------------------------------------------

    void tone(
        float start,
        float duration,
        float hz,
        float gain,
        float endHz = 0.0f,
        float harmonic = 0.0f)
    {
        if (
            duration <= 0.0f ||
            gain <= 0.0f ||
            hz <= 0.0f ||
            rate <= 0 ||
            start < 0.0f
        ) {
            return;
        }

        const int total =
            std::max(
                1,
                static_cast<int>(
                    duration * rate
                )
            );

        const std::size_t firstSample =
            static_cast<std::size_t>(
                start * rate
            );

        // Attack/release adapt to short sounds.
        const float attack =
            std::min(
                0.005f,
                duration * 0.20f
            );

        const float release =
            std::min(
                0.018f,
                duration * 0.30f
            );

        harmonic =
            std::clamp(
                harmonic,
                0.0f,
                0.6f
            );

        for (int i = 0; i < total; ++i) {

            const std::size_t index =
                firstSample +
                static_cast<std::size_t>(i);

            if (index >= samples.size())
                break;

            const float t =
                static_cast<float>(i) /
                static_cast<float>(rate);

            const float u =
                std::clamp(
                    t / duration,
                    0.0f,
                    1.0f
                );

            const float attackEnvelope =
                attack > 0.0f
                    ? std::min(
                        1.0f,
                        t / attack
                    )
                    : 1.0f;

            const float releaseEnvelope =
                release > 0.0f
                    ? std::min(
                        1.0f,
                        (duration - t) /
                        release
                    )
                    : 1.0f;

            // Keeps the original character but prevents abrupt cut-off.
            const float decay =
                std::exp(
                    -2.0f * u
                );

            const float envelope =
                attackEnvelope *
                releaseEnvelope *
                decay;

            // Linear chirp.
            const double sweep =
                endHz > 0.0f
                    ? (
                        static_cast<double>(
                            endHz - hz
                        ) *
                        t *
                        t /
                        (
                            2.0 *
                            duration
                        )
                    )
                    : 0.0;

            const double phase =
                2.0 *
                PI *
                (
                    hz * t +
                    sweep
                );

            float wave =
                static_cast<float>(
                    std::sin(phase)
                );

            // A little harmonic content stops effects sounding like
            // perfectly clean test signals.
            if (harmonic > 0.0f) {

                const float second =
                    static_cast<float>(
                        std::sin(
                            phase * 2.0
                        )
                    );

                wave =
                    (
                        wave +
                        second * harmonic
                    ) /
                    (
                        1.0f +
                        harmonic
                    );
            }

            samples[index] +=
                gain *
                envelope *
                wave;
        }
    }

    // -------------------------------------------------------------------------
    // Filtered noise
    //
    // smoothing:
    //   high value -> sharp / bright
    //   low value  -> muffled / soft
    // -------------------------------------------------------------------------

    void noiseBurst(
        float start,
        float duration,
        float gain,
        float smoothing)
    {
        if (
            duration <= 0.0f ||
            gain <= 0.0f ||
            rate <= 0 ||
            start < 0.0f
        ) {
            return;
        }

        smoothing =
            std::clamp(
                smoothing,
                0.001f,
                1.0f
            );

        const int total =
            std::max(
                1,
                static_cast<int>(
                    duration * rate
                )
            );

        const std::size_t firstSample =
            static_cast<std::size_t>(
                start * rate
            );

        const float attack =
            std::min(
                0.004f,
                duration * 0.15f
            );

        const float release =
            std::min(
                0.025f,
                duration * 0.35f
            );

        float filtered = 0.0f;

        for (int i = 0; i < total; ++i) {

            const std::size_t index =
                firstSample +
                static_cast<std::size_t>(i);

            if (index >= samples.size())
                break;

            const float t =
                static_cast<float>(i) /
                static_cast<float>(rate);

            const float u =
                std::clamp(
                    t / duration,
                    0.0f,
                    1.0f
                );

            filtered +=
                smoothing *
                (
                    noise() -
                    filtered
                );

            const float attackEnvelope =
                attack > 0.0f
                    ? std::min(
                        1.0f,
                        t / attack
                    )
                    : 1.0f;

            const float releaseEnvelope =
                release > 0.0f
                    ? std::min(
                        1.0f,
                        (duration - t) /
                        release
                    )
                    : 1.0f;

            const float decay =
                std::exp(
                    -3.0f * u
                );

            const float envelope =
                attackEnvelope *
                releaseEnvelope *
                decay;

            samples[index] +=
                gain *
                filtered *
                envelope;
        }
    }

    // -------------------------------------------------------------------------
    // Short mechanical impact
    // -------------------------------------------------------------------------

    void click(
        float start,
        float gain = 0.2f)
    {
        noiseBurst(
            start,
            0.018f,
            gain,
            0.90f
        );

        tone(
            start,
            0.025f,
            1850.0f,
            gain * 0.35f,
            900.0f,
            0.20f
        );
    }

    // -------------------------------------------------------------------------
    // Normalize + trim
    //
    // Scaling is preferable to simply clipping every sample at +/- 0.8.
    // -------------------------------------------------------------------------

    void finalize()
    {
        float peak = 0.0f;

        for (float sample : samples) {
            peak =
                std::max(
                    peak,
                    std::abs(sample)
                );
        }

        if (peak > CLIP_PEAK) {

            const float scale =
                CLIP_PEAK / peak;

            for (float& sample : samples)
                sample *= scale;
        }

        // Remove completely inactive tail.
        while (
            !samples.empty() &&
            std::abs(samples.back()) <
                0.00001f
        ) {
            samples.pop_back();
        }

        // Ensures interpolation always ends on zero.
        if (!samples.empty())
            samples.back() = 0.0f;
    }
};

} // namespace

// =============================================================================
// SOUND SYNTHESIS
// =============================================================================

std::vector<float> synthesizeSound(
    Sound sound,
    int sampleRate)
{
    const int soundIndex =
        static_cast<int>(sound);

    if (
        sampleRate < 8000 ||
        sampleRate > 192000 ||
        soundIndex < 0 ||
        soundIndex >=
            static_cast<int>(Sound::Count)
    ) {
        return {};
    }

    float length = 1.20f;

    switch (sound) {

        case Sound::Smoke:
            length = 2.05f;
            break;

        case Sound::Drink:
            length = 1.90f;
            break;

        default:
            break;
    }

    Synth s(
        length,
        sampleRate
    );

    switch (sound) {

        case Sound::Startup:
            // Original ascending handheld-style chime, with soft odd harmonics.
            s.tone(0.0f, .16f, 523.25f, .24f, 523.25f, .25f);
            s.tone(.13f, .17f, 783.99f, .22f, 783.99f, .25f);
            s.tone(.29f, .65f, 1046.50f, .25f, 1046.50f, .3f);
            s.tone(.29f, .65f, 1567.98f, .09f);
            break;

        // =====================================================================
        // FOOTSTEP
        // =====================================================================

        case Sound::Step:
        {
            // Low body impact.
            s.tone(
                0.000f,
                0.145f,
                96.0f,
                0.20f,
                52.0f,
                0.12f
            );

            // Initial sole contact.
            s.noiseBurst(
                0.000f,
                0.070f,
                0.11f,
                0.11f
            );

            // Slight secondary scrape.
            s.noiseBurst(
                0.052f,
                0.125f,
                0.055f,
                0.040f
            );

            // Small sole click.
            s.tone(
                0.018f,
                0.045f,
                210.0f,
                0.035f,
                110.0f
            );

            break;
        }

        // =====================================================================
        // DRINK
        // =====================================================================

        case Sound::Drink:
        {
            // Opening/contact sound.
            s.click(
                0.000f,
                0.30f
            );

            s.tone(
                0.010f,
                0.070f,
                1700.0f,
                0.11f,
                720.0f,
                0.14f
            );

            // Initial fizz / liquid movement.
            s.noiseBurst(
                0.040f,
                0.230f,
                0.13f,
                0.16f
            );

            constexpr float gulps[] = {
                0.28f,
                0.65f,
                1.02f,
                1.39f
            };

            for (float at : gulps) {

                // Main low movement.
                s.tone(
                    at,
                    0.17f,
                    310.0f,
                    0.20f,
                    108.0f,
                    0.05f
                );

                // Higher transient.
                s.tone(
                    at + 0.025f,
                    0.12f,
                    545.0f,
                    0.075f,
                    185.0f
                );

                // Liquid/noise texture.
                s.noiseBurst(
                    at,
                    0.18f,
                    0.145f,
                    0.055f
                );
            }

            break;
        }

        // =====================================================================
        // SMOKE EFFECT
        // =====================================================================

        case Sound::Smoke:
        {
            // Existing stylized ignition / air noise sequence.
            s.noiseBurst(
                0.000f,
                0.045f,
                0.48f,
                0.82f
            );

            s.tone(
                0.000f,
                0.038f,
                2200.0f,
                0.10f,
                900.0f,
                0.12f
            );

            s.noiseBurst(
                0.100f,
                0.060f,
                0.34f,
                0.66f
            );

            s.noiseBurst(
                0.160f,
                0.300f,
                0.16f,
                0.22f
            );

            s.noiseBurst(
                0.500f,
                0.52f,
                0.20f,
                0.030f
            );

            s.noiseBurst(
                1.20f,
                0.72f,
                0.27f,
                0.050f
            );

            break;
        }

        // =====================================================================
        // CASH REGISTER
        // =====================================================================

        case Sound::Cash:
        {
            // Drawer/mechanical hit.
            s.noiseBurst(
                0.000f,
                0.085f,
                0.22f,
                0.24f
            );

            s.click(
                0.012f,
                0.18f
            );

            // Register bell.
            s.tone(
                0.025f,
                0.65f,
                1850.0f,
                0.27f,
                0.0f,
                0.18f
            );

            s.tone(
                0.025f,
                0.52f,
                2731.0f,
                0.105f,
                0.0f,
                0.22f
            );

            // Mechanical drawer movement.
            s.noiseBurst(
                0.18f,
                0.28f,
                0.14f,
                0.12f
            );

            for (float at : {
                0.27f,
                0.36f,
                0.48f
            }) {
                s.tone(
                    at,
                    0.08f,
                    3200.0f,
                    0.075f,
                    0.0f,
                    0.12f
                );
            }

            // Drawer closing.
            s.noiseBurst(
                0.65f,
                0.075f,
                0.24f,
                0.28f
            );

            s.click(
                0.67f,
                0.12f
            );

            break;
        }

        // =====================================================================
        // CARD TERMINAL
        // =====================================================================

        case Sound::Card:
        {
            // Contact / button.
            s.noiseBurst(
                0.000f,
                0.09f,
                0.08f,
                0.16f
            );

            s.click(
                0.025f,
                0.08f
            );

            // Confirmation sequence.
            s.tone(
                0.18f,
                0.10f,
                1050.0f,
                0.18f,
                0.0f,
                0.08f
            );

            s.tone(
                0.48f,
                0.13f,
                1450.0f,
                0.18f,
                0.0f,
                0.08f
            );

            s.tone(
                0.65f,
                0.24f,
                1900.0f,
                0.18f,
                0.0f,
                0.06f
            );

            break;
        }

        // =====================================================================
        // COMPUTER
        // =====================================================================

        case Sound::Computer:
        {
            // Key/button.
            s.click(
                0.000f,
                0.18f
            );

            // Electronic background.
            s.noiseBurst(
                0.04f,
                0.66f,
                0.055f,
                0.022f
            );

            // Ascending UI confirmation.
            s.tone(
                0.12f,
                0.16f,
                440.0f,
                0.14f,
                0.0f,
                0.10f
            );

            s.tone(
                0.30f,
                0.17f,
                660.0f,
                0.14f,
                0.0f,
                0.08f
            );

            s.tone(
                0.50f,
                0.35f,
                880.0f,
                0.15f,
                0.0f,
                0.06f
            );

            break;
        }

        // =====================================================================
        // PICKUP
        // =====================================================================

        case Sound::Pickup:
        {
            // Physical contact.
            s.noiseBurst(
                0.000f,
                0.055f,
                0.20f,
                0.22f
            );

            // Quick descending thunk.
            s.tone(
                0.010f,
                0.135f,
                650.0f,
                0.095f,
                330.0f,
                0.08f
            );

            s.click(
                0.018f,
                0.06f
            );

            break;
        }

        // =====================================================================
        // DELIVERY
        // =====================================================================

        case Sound::Delivery:
        {
            // Positive two-note notification.
            s.tone(
                0.000f,
                0.22f,
                660.0f,
                0.18f,
                0.0f,
                0.10f
            );

            s.tone(
                0.24f,
                0.30f,
                880.0f,
                0.18f,
                0.0f,
                0.08f
            );

            // Physical package arrival.
            s.noiseBurst(
                0.55f,
                0.12f,
                0.18f,
                0.14f
            );

            s.tone(
                0.56f,
                0.10f,
                125.0f,
                0.12f,
                70.0f
            );

            break;
        }

        default:
            break;
    }

    s.finalize();

    return std::move(s.samples);
}

// =============================================================================
// INITIALIZATION
// =============================================================================

bool Audio::initialize()
{
    if (device)
        return true;

    if (
        SDL_InitSubSystem(
            SDL_INIT_AUDIO
        ) != 0
    ) {
        return false;
    }

    // One stereo device mixes streamed music and the procedural effects.
    if (Mix_OpenAudioDevice(DEFAULT_SAMPLE_RATE, AUDIO_F32SYS, 2, 1024,
                            nullptr, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE) != 0)
        return false;
    device = true;
    int sampleRate = 0, channels = 0;
    Uint16 format = 0;
    Mix_QuerySpec(&sampleRate, &format, &channels);
    if (format != AUDIO_F32SYS || channels != 2 || sampleRate < 8000) {
        Mix_CloseAudio();
        device = false;
        return false;
    }

    // -------------------------------------------------------------------------
    // Generate clips using the REAL device sample rate.
    // -------------------------------------------------------------------------

    for (
        std::size_t i = 0;
        i < clips.size();
        ++i
    ) {
        clips[i] =
            synthesizeSound(
                static_cast<Sound>(i),
                sampleRate
            );
    }

    // Reset voices before starting callback.
    for (auto& voice : voices)
        voice = {};

    stepDistance = 0.0f;
    walking = false;

    effectsPaused = false;
    Mix_SetPostMix(&Audio::callback, this);

    return true;
}

// =============================================================================
// SHUTDOWN
// =============================================================================

void Audio::shutdown()
{
    if (!device)
        return;

    Mix_SetPostMix(nullptr, nullptr);
    Mix_CloseAudio();

    device = 0;

    // Callback no longer exists, so no lock is necessary.
    for (auto& voice : voices)
        voice = {};

    stepDistance = 0.0f;
    walking = false;
}

// =============================================================================
// MIXER CALLBACK
// =============================================================================

void Audio::callback(
    void* user,
    Uint8* stream,
    int bytes)
{
    if (
        !user ||
        !stream ||
        bytes <= 0
    ) {
        return;
    }

    auto& audio =
        *static_cast<Audio*>(user);

    auto* out =
        reinterpret_cast<float*>(
            stream
        );

    std::lock_guard<std::mutex> lock(audio.mixMutex);
    if (audio.effectsPaused) return;
    const int count = bytes / static_cast<int>(sizeof(float) * 2);

    // -------------------------------------------------------------------------
    // Mix active voices
    // -------------------------------------------------------------------------

    for (auto& voice : audio.voices) {

        if (!voice.clip)
            continue;

        if (voice.clip->empty()) {
            voice = {};
            continue;
        }

        for (int i = 0; i < count; ++i) {

            if (
                !voice.clip ||
                voice.cursor < 0.0f
            ) {
                voice = {};
                break;
            }

            const std::size_t index =
                static_cast<std::size_t>(
                    voice.cursor
                );

            if (
                index >=
                voice.clip->size()
            ) {
                voice = {};
                break;
            }

            const std::size_t next =
                std::min(
                    index + 1,
                    voice.clip->size() - 1
                );

            const float fraction =
                static_cast<float>(
                    voice.cursor -
                    static_cast<float>(
                        index
                    )
                );

            const float a =
                (*voice.clip)[index];

            const float b =
                (*voice.clip)[next];

            // Linear interpolation permits smooth playback-rate changes.
            const float sample =
                a +
                (
                    b - a
                ) *
                fraction;

            const float effect = sample * voice.gain * audio.volume;
            out[i*2] += effect;
            out[i*2+1] += effect;

            voice.cursor +=
                voice.rate;
        }
    }

    // -------------------------------------------------------------------------
    // Limit the combined stereo stream without changing music volume.
    // -------------------------------------------------------------------------
    for (int i = 0; i < count*2; ++i)
        out[i] = softLimit(out[i]);

}

// =============================================================================
// PLAY SOUND
// =============================================================================

void Audio::play(
    Sound sound,
    float gain,
    float rate)
{
    const int rawIndex =
        static_cast<int>(sound);

    if (
        !device ||
        rawIndex < 0 ||
        rawIndex >=
            static_cast<int>(
                clips.size()
            )
    ) {
        return;
    }

    const std::size_t index =
        static_cast<std::size_t>(
            rawIndex
        );

    if (clips[index].empty())
        return;

    gain =
        std::clamp(
            gain,
            0.0f,
            1.0f
        );

    rate =
        std::clamp(
            rate,
            0.50f,
            2.0f
        );

    if (gain <= 0.0f)
        return;

    std::lock_guard<std::mutex> lock(mixMutex);

    // -------------------------------------------------------------------------
    // Prefer an unused voice.
    // -------------------------------------------------------------------------

    auto voice =
        std::find_if(
            voices.begin(),
            voices.end(),
            [](const Voice& v)
            {
                return !v.clip;
            }
        );

    // -------------------------------------------------------------------------
    // Voice stealing.
    //
    // Previously a sound simply disappeared if all voices were occupied.
    // Now the voice closest to completion is replaced.
    // -------------------------------------------------------------------------

    if (voice == voices.end()) {

        voice =
            std::max_element(
                voices.begin(),
                voices.end(),

                [](const Voice& a,
                   const Voice& b)
                {
                    auto progress =
                        [](const Voice& v)
                        {
                            if (
                                !v.clip ||
                                v.clip->empty()
                            ) {
                                return 1.0f;
                            }

                            return
                                static_cast<float>(
                                    v.cursor
                                ) /
                                static_cast<float>(
                                    v.clip->size()
                                );
                        };

                    return
                        progress(a) <
                        progress(b);
                }
            );
    }

    if (voice != voices.end()) {

        *voice = {
            &clips[index],
            0.0f,
            rate,
            gain
        };
    }


}

// =============================================================================
// PAYMENT
// =============================================================================

bool Audio::payment()
{
    const bool card =
        (nextRandom() & 1u) == 0u;

    play(
        card
            ? Sound::Card
            : Sound::Cash
    );

    return card;
}

// =============================================================================
// PLAYER MOVEMENT / FOOTSTEPS
// =============================================================================

void Audio::moved(float distance)
{
    if (distance <= 0.00001f) {

        stepDistance = 0.0f;
        walking = false;

        return;
    }

    // First step when movement starts.
    if (!walking) {

        play(
            Sound::Step,
            0.40f,
            randomRange(
                0.92f,
                1.08f
            )
        );

        walking = true;
    }

    // Prevent enormous movement/teleport values from creating
    // a burst of many footsteps.
    distance =
        std::min(
            distance,
            STEP_DISTANCE * 3.0f
        );

    stepDistance += distance;

    // Handles low-FPS frames better than a single "if".
    int generatedSteps = 0;

    while (
        stepDistance >= STEP_DISTANCE &&
        generatedSteps < 3
    ) {
        stepDistance -=
            STEP_DISTANCE;

        play(
            Sound::Step,
            0.40f,
            randomRange(
                0.92f,
                1.08f
            )
        );

        ++generatedSteps;
    }

    // Numerical safety.
    stepDistance =
        std::max(
            0.0f,
            stepDistance
        );
}

// =============================================================================
// MASTER VOLUME
// =============================================================================

void Audio::setVolume(int percent)
{
    const float newVolume =
        static_cast<float>(
            std::clamp(
                percent,
                0,
                100
            )
        ) /
        100.0f;

    std::lock_guard<std::mutex> lock(mixMutex);

    volume = newVolume;


}

// =============================================================================
// PAUSE
// =============================================================================

void Audio::pause(bool paused)
{
    if (!device)
        return;

    std::lock_guard<std::mutex> lock(mixMutex);
    effectsPaused = paused;
}

// =============================================================================
// CLEAR ACTIVE SOUNDS
// =============================================================================

void Audio::clear()
{
    std::lock_guard<std::mutex> lock(mixMutex);

    for (auto& voice : voices)
        voice = {};

    stepDistance = 0.0f;
    walking = false;


}
