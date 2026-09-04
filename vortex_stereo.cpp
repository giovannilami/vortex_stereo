#include <new>
#include <math.h>
#include <string.h>
#include <distingnt/api.h>
#include "dsp.h"

// --- Algorithm state ---

struct _vortexChannel
{
    vortex::Filter1 f1;
    vortex::Filter2 f2a, f2b;

    void reset()
    {
        f1.reset();
        f2a.reset();
        f2b.reset();
    }
};

// --- Algorithm struct ---

struct _vortexAlgorithm : public _NT_algorithm
{
    // Independent audio state per channel. Coefficients are shared by copying
    // them whenever parameters change, so stereo does not duplicate the
    // expensive coefficient calculations.
    _vortexChannel left;
    _vortexChannel right;

    // Cached parameters (set by parameterChanged)
    int mode;             // 0-11: LP6/LP12/LP24/HP6/HP12/HP24/BP/BP+/Notch/Notch+/AP/AP+
    float cutoffHz;       // 20-20000 Hz
    float damping;        // resonance mapped to damping
    float drive;          // 0.0-1.0
    float mix;            // 0.0-1.0
    float fmDepth;        // -1.0 to 1.0
    float sampleRate;

    // Last coefficient set. Most patches do not modulate cutoff/resonance at
    // audio rate, so recomputing the same sqrt/division-heavy coefficients for
    // every sample wastes most of the CPU time.
    int coefficientMode;
    float coefficientCutoff;
    float coefficientDamping;
    float coefficientSampleRate;
    bool coefficientsValid;

    _vortexAlgorithm()
    {
        mode = 1;           // LP12
        cutoffHz = 632.0f;  // ~mid-range (param 500)
        damping = 0.707f;   // Butterworth
        drive = 0.0f;
        mix = 1.0f;         // fully wet
        fmDepth = 0.0f;
        sampleRate = 48000.0f;

        coefficientMode = -1;
        coefficientCutoff = 0.0f;
        coefficientDamping = 0.0f;
        coefficientSampleRate = 0.0f;
        coefficientsValid = false;
    }
};

// --- Parameter indices ---

enum {
    // I/O. Keep stereo routing contiguous, following the official API examples.
    kParamChannelMode,
    kParamInputL,
    kParamInputR,
    kParamOutputL,
    kParamOutputR,
    kParamOutputMode,

    // Filter (4)
    kParamMode,
    kParamCutoff,
    kParamResonance,
    kParamDrive,

    // Global (3)
    kParamMix,
    kParamFMDepth,
    kParamVersion,

    // CV Inputs (7)
    kParamCVAudioIn,
    kParamCVCutoffVOCT,
    kParamCVCutoffFM,
    kParamCVResonance,
    kParamCVMode,
    kParamCVDrive,
    kParamCVMix,

    kNumParams
};

// --- Enum strings ---

static const char* modeStrings[] = {
    "LP 6dB", "LP 12dB", "LP 24dB",
    "HP 6dB", "HP 12dB", "HP 24dB",
    "BP", "BP+",
    "Notch", "Notch+",
    "AP", "AP+", NULL
};
static const char* versionStrings[] = { VORTEX_VERSION, NULL };
static const char* channelModeStrings[] = { "Mono", "Stereo", NULL };

// --- Parameter definitions ---

static _NT_parameter parameters[] = {
    // I/O
    { "Channels", 0, 1, 0, kNT_unitEnum, 0, channelModeStrings },
    NT_PARAMETER_AUDIO_INPUT( "Input L/Mono", 1, 1 )
    NT_PARAMETER_AUDIO_INPUT( "Input R", 0, 2 )
    NT_PARAMETER_AUDIO_OUTPUT( "Output L/Mono", 1, 13 )
    NT_PARAMETER_AUDIO_OUTPUT( "Output R", 1, 14 )
    NT_PARAMETER_OUTPUT_MODE( "Output" )

    // Filter
    { "Mode",       0,   11,    1, kNT_unitEnum,       0, modeStrings },
    { "Cutoff",     0, 1000,  500, kNT_unitHasStrings, 0, NULL },
    { "Resonance",  0, 1000,    0, kNT_unitHasStrings, kNT_scaling10, NULL },
    { "Drive",      0, 1000,    0, kNT_unitPercent,    kNT_scaling10, NULL },

    // Global
    { "Mix",        0, 1000, 1000, kNT_unitPercent,    kNT_scaling10, NULL },
    { "FM Depth", -1000, 1000,    0, kNT_unitPercent,  kNT_scaling10, NULL },

    // Version (read-only)
    { "Version",    0,    0,   0, kNT_unitEnum,        0, versionStrings },

    // CV Inputs
    NT_PARAMETER_AUDIO_INPUT( "Audio In CV",     0, 0 )
    NT_PARAMETER_CV_INPUT( "Cutoff V/OCT CV",    0, 0 )
    NT_PARAMETER_CV_INPUT( "Cutoff FM CV",       0, 0 )
    NT_PARAMETER_CV_INPUT( "Resonance CV",       0, 0 )
    NT_PARAMETER_CV_INPUT( "Mode CV",            0, 0 )
    NT_PARAMETER_CV_INPUT( "Drive CV",           0, 0 )
    NT_PARAMETER_CV_INPUT( "Mix CV",             0, 0 )
};

// --- Parameter pages ---

static const uint8_t pageIO[] = {
    kParamChannelMode,
    kParamInputL, kParamInputR,
    kParamOutputL, kParamOutputR,
    kParamOutputMode
};
static const uint8_t pageFilter[] = {
    kParamMode, kParamCutoff, kParamResonance, kParamDrive
};
static const uint8_t pageGlobal[] = {
    kParamMix, kParamFMDepth, kParamVersion
};
static const uint8_t pageCV[] = {
    kParamCVAudioIn, kParamCVCutoffVOCT, kParamCVCutoffFM,
    kParamCVResonance, kParamCVMode, kParamCVDrive, kParamCVMix
};

static const _NT_parameterPage pages[] = {
    { .name = "I/O",    .numParams = ARRAY_SIZE(pageIO),      .params = pageIO },
    { .name = "Filter", .numParams = ARRAY_SIZE(pageFilter),  .params = pageFilter },
    { .name = "Global", .numParams = ARRAY_SIZE(pageGlobal),  .params = pageGlobal },
    { .name = "CV",     .numParams = ARRAY_SIZE(pageCV),       .params = pageCV },
};

static const _NT_parameterPages parameterPages = {
    .numPages = ARRAY_SIZE(pages),
    .pages = pages,
};

// --- Lifecycle ---

static void calculateRequirements(
    _NT_algorithmRequirements& req,
    const int32_t* )
{
    req.numParameters = ARRAY_SIZE(parameters);
    req.sram = sizeof( _vortexAlgorithm );
    req.dram = 0;
    req.dtc = 0;
    req.itc = 0;
}

static _NT_algorithm* construct(
    const _NT_algorithmMemoryPtrs& ptrs,
    const _NT_algorithmRequirements&,
    const int32_t* )
{
    static_assert( kNumParams == ARRAY_SIZE(parameters), "parameter table mismatch" );
    _vortexAlgorithm* alg = new ( ptrs.sram ) _vortexAlgorithm();
    alg->parameters = parameters;
    alg->parameterPages = &parameterPages;
    return alg;
}

// --- Parameter changed ---

static void parameterChanged( _NT_algorithm* self, int parameter )
{
    _vortexAlgorithm* p = (_vortexAlgorithm*)self;

    switch ( parameter )
    {
    case kParamMode:
        p->mode = p->v[parameter];
        // Reset filter state when mode changes to avoid transients
        p->left.reset();
        p->right.reset();
        p->coefficientsValid = false;
        break;
    case kParamCutoff:
        p->cutoffHz = vortex::cutoff_param_to_hz( p->v[parameter] );
        p->coefficientsValid = false;
        break;
    case kParamResonance:
        p->damping = vortex::resonance_to_damping( p->v[parameter] );
        p->coefficientsValid = false;
        break;
    case kParamDrive:
        p->drive = (float)p->v[parameter] * 0.001f;
        break;
    case kParamMix:
        p->mix = (float)p->v[parameter] * 0.001f;
        break;
    case kParamFMDepth:
        p->fmDepth = (float)p->v[parameter] * 0.001f;
        break;
    case kParamChannelMode:
        // Keep the left channel continuous while starting the optional right
        // channel from a known state.
        p->right.reset();
        p->coefficientsValid = false;
        break;
    }
}

// --- Audio ---

static inline bool isCascadedMode( int mode )
{
    return mode == 2 || mode == 5 || mode == 7 || mode == 9 || mode == 11;
}

static inline void configureFilterIfNeeded(
    _vortexAlgorithm* p,
    int mode,
    float sampleRate,
    float cutoff,
    float damping )
{
    if ( p->coefficientsValid &&
         p->coefficientMode == mode &&
         p->coefficientCutoff == cutoff &&
         p->coefficientDamping == damping &&
         p->coefficientSampleRate == sampleRate )
        return;

    switch ( mode )
    {
    case 0:
        vortex::filter1_configure_lp( p->left.f1, sampleRate, cutoff );
        vortex::filter1_copy_coefficients( p->left.f1, p->right.f1 );
        break;
    case 3:
        vortex::filter1_configure_hp( p->left.f1, sampleRate, cutoff );
        vortex::filter1_copy_coefficients( p->left.f1, p->right.f1 );
        break;
    default:
    {
        vortex::Filter2Type type = vortex::F2_LP;
        if ( mode == 4 || mode == 5 )
            type = vortex::F2_HP;
        else if ( mode == 6 || mode == 7 )
            type = vortex::F2_BP;
        else if ( mode == 8 || mode == 9 )
            type = vortex::F2_NOTCH;
        else if ( mode == 10 || mode == 11 )
            type = vortex::F2_AP;

        vortex::filter2_configure( p->left.f2a, sampleRate, cutoff, damping, type );
        if ( isCascadedMode( mode ) )
            vortex::filter2_copy_coefficients( p->left.f2a, p->left.f2b );
        vortex::filter2_copy_coefficients( p->left.f2a, p->right.f2a );
        if ( isCascadedMode( mode ) )
            vortex::filter2_copy_coefficients( p->left.f2a, p->right.f2b );
        break;
    }
    }

    p->coefficientMode = mode;
    p->coefficientCutoff = cutoff;
    p->coefficientDamping = damping;
    p->coefficientSampleRate = sampleRate;
    p->coefficientsValid = true;
}

static inline float processFilterSample( _vortexChannel& channel, int mode, float signal )
{
    switch ( mode )
    {
    case 0:
        return channel.f1.process_lp( signal );
    case 1:
        return vortex::filter2_process( channel.f2a, signal, vortex::F2_LP );
    case 2:
    {
        float wet = vortex::filter2_process( channel.f2a, signal, vortex::F2_LP );
        return vortex::filter2_process( channel.f2b, wet, vortex::F2_LP );
    }
    case 3:
        return channel.f1.process_hp( signal );
    case 4:
        return vortex::filter2_process( channel.f2a, signal, vortex::F2_HP );
    case 5:
    {
        float wet = vortex::filter2_process( channel.f2a, signal, vortex::F2_HP );
        return vortex::filter2_process( channel.f2b, wet, vortex::F2_HP );
    }
    case 6:
        return vortex::filter2_process( channel.f2a, signal, vortex::F2_BP );
    case 7:
    {
        float wet = vortex::filter2_process( channel.f2a, signal, vortex::F2_BP );
        return vortex::filter2_process( channel.f2b, wet, vortex::F2_BP );
    }
    case 8:
        return vortex::filter2_process( channel.f2a, signal, vortex::F2_NOTCH );
    case 9:
    {
        float wet = vortex::filter2_process( channel.f2a, signal, vortex::F2_NOTCH );
        return vortex::filter2_process( channel.f2b, wet, vortex::F2_NOTCH );
    }
    case 10:
        return vortex::filter2_process( channel.f2a, signal, vortex::F2_AP );
    case 11:
    {
        float wet = vortex::filter2_process( channel.f2a, signal, vortex::F2_AP );
        return vortex::filter2_process( channel.f2b, wet, vortex::F2_AP );
    }
    }
    return 0.0f;
}

static inline void flushActiveFilterState( _vortexChannel& channel, int mode )
{
    if ( mode == 0 || mode == 3 )
    {
        channel.f1.z = vortex::flush_denormal( channel.f1.z );
        return;
    }

    channel.f2a.z0 = vortex::flush_denormal( channel.f2a.z0 );
    channel.f2a.z1 = vortex::flush_denormal( channel.f2a.z1 );
    if ( isCascadedMode( mode ) )
    {
        channel.f2b.z0 = vortex::flush_denormal( channel.f2b.z0 );
        channel.f2b.z1 = vortex::flush_denormal( channel.f2b.z1 );
    }
}

static inline float mixSample( float dry, float wet, float mix )
{
    if ( mix <= 0.0f )
        return dry;
    if ( mix >= 1.0f )
        return wet;
    return dry * ( 1.0f - mix ) + wet * mix;
}

static void step(
    _NT_algorithm* self,
    float* busFrames,
    int numFramesBy4 )
{
    _vortexAlgorithm* p = (_vortexAlgorithm*)self;
    int numFrames = numFramesBy4 * 4;

    p->sampleRate = (float)NT_globals.sampleRate;

    // Get I/O bus pointers
    const float* audioIn = p->v[kParamInputL]
        ? busFrames + ( p->v[kParamInputL] - 1 ) * numFrames : NULL;
    float* out = busFrames + ( p->v[kParamOutputL] - 1 ) * numFrames;
    bool replace = p->v[kParamOutputMode];
    bool stereo = p->v[kParamChannelMode] != 0;
    const float* audioInR = stereo && p->v[kParamInputR]
        ? busFrames + ( p->v[kParamInputR] - 1 ) * numFrames : NULL;
    float* outR = stereo
        ? busFrames + ( p->v[kParamOutputR] - 1 ) * numFrames : NULL;

    // Read CV buses (0 = not connected)
    const float* cvAudioIn = p->v[kParamCVAudioIn]
        ? busFrames + ( p->v[kParamCVAudioIn] - 1 ) * numFrames : NULL;
    const float* cvVOCT = p->v[kParamCVCutoffVOCT]
        ? busFrames + ( p->v[kParamCVCutoffVOCT] - 1 ) * numFrames : NULL;
    const float* cvFM = p->v[kParamCVCutoffFM]
        ? busFrames + ( p->v[kParamCVCutoffFM] - 1 ) * numFrames : NULL;
    const float* cvResonance = p->v[kParamCVResonance]
        ? busFrames + ( p->v[kParamCVResonance] - 1 ) * numFrames : NULL;
    const float* cvMode = p->v[kParamCVMode]
        ? busFrames + ( p->v[kParamCVMode] - 1 ) * numFrames : NULL;
    const float* cvDrive = p->v[kParamCVDrive]
        ? busFrames + ( p->v[kParamCVDrive] - 1 ) * numFrames : NULL;
    const float* cvMix = p->v[kParamCVMix]
        ? busFrames + ( p->v[kParamCVMix] - 1 ) * numFrames : NULL;

    float fs = p->sampleRate;

    for ( int i = 0; i < numFrames; ++i )
    {
        // --- Read input ---
        float input = 0.0f;
        if ( audioIn )
            input = audioIn[i];
        else if ( cvAudioIn )
            input = cvAudioIn[i];
        float dry = input;
        float inputR = audioInR ? audioInR[i] : input;
        float dryR = inputR;

        // --- Compute effective mode ---
        int mode = p->mode;
        if ( cvMode )
        {
            // CV mode: ±5V range, quantize to 0-11
            int modeOffset = (int)( cvMode[i] * 2.4f );  // ~5V = 12 steps
            mode = mode + modeOffset;
            if ( mode < 0 ) mode = 0;
            if ( mode > 11 ) mode = 11;
        }

        // --- Compute effective cutoff ---
        float cutoff = p->cutoffHz;

        // Both inputs are exponential. FM Depth attenuates or inverts only
        // Cutoff FM CV, exactly as in the original Vortex implementation.
        if ( cvVOCT || cvFM )
        {
            float octaveOffset = 0.0f;
            if ( cvVOCT )
                octaveOffset += cvVOCT[i];
            if ( cvFM )
                octaveOffset += cvFM[i] * p->fmDepth;
            cutoff *= vortex::voct_to_mult( octaveOffset );
        }

        // Clamp cutoff to safe range
        if ( cutoff < 20.0f ) cutoff = 20.0f;
        if ( cutoff > 20000.0f ) cutoff = 20000.0f;

        // --- Compute effective resonance/damping ---
        float damping = p->damping;
        if ( cvResonance )
        {
            // CV adds to resonance (reduces damping)
            float resoAdd = cvResonance[i] * 0.2f;  // ±5V -> ±1.0 damping range
            damping -= resoAdd;
            if ( damping < 0.01f ) damping = 0.01f;
            if ( damping > 0.707f ) damping = 0.707f;
        }

        // --- Compute effective drive ---
        float drv = p->drive;
        if ( cvDrive )
        {
            drv += cvDrive[i] * 0.2f;
            if ( drv < 0.0f ) drv = 0.0f;
            if ( drv > 1.0f ) drv = 1.0f;
        }

        // --- Compute effective mix ---
        float mix = p->mix;
        if ( cvMix )
        {
            mix += cvMix[i] * 0.2f;
            if ( mix < 0.0f ) mix = 0.0f;
            if ( mix > 1.0f ) mix = 1.0f;
        }

        // --- Apply drive (pre-filter saturation) ---
        float signal = input;
        float signalR = inputR;
        if ( drv > 0.0f )
        {
            float driveGain = 1.0f + drv * 9.0f;  // 1x to 10x gain
            signal = vortex::soft_clip( signal * driveGain );
            if ( stereo )
                signalR = vortex::soft_clip( signalR * driveGain );
        }

        // --- Process through filter ---
        configureFilterIfNeeded( p, mode, fs, cutoff, damping );
        float wet = processFilterSample( p->left, mode, signal );
        float wetR = stereo ? processFilterSample( p->right, mode, signalR ) : 0.0f;

        // Flush only the state used by the active mode.
        flushActiveFilterState( p->left, mode );
        if ( stereo )
            flushActiveFilterState( p->right, mode );

        // --- Dry/wet mix ---
        float result = mixSample( dry, wet, mix );
        float resultR = stereo ? mixSample( dryR, wetR, mix ) : 0.0f;

        // --- Write output ---
        if ( replace )
            out[i] = result;
        else
            out[i] += result;

        if ( stereo )
        {
            if ( replace )
                outR[i] = resultR;
            else
                outR[i] += resultR;
        }
    }
}

// --- Parameter string display ---

static int parameterString( _NT_algorithm*, int param, int val, char* buff )
{
    // Cutoff: display as Hz
    if ( param == kParamCutoff )
    {
        float hz = vortex::cutoff_param_to_hz( val );
        int len;
        if ( hz >= 1000.0f )
        {
            // Display as kHz
            float khz = hz / 1000.0f;
            len = NT_floatToString( buff, khz, (khz < 10.0f) ? 2 : 1 );
            const char* suffix = " kHz";
            while ( *suffix ) buff[len++] = *suffix++;
        }
        else
        {
            len = NT_floatToString( buff, hz, (hz < 100.0f) ? 1 : 0 );
            const char* suffix = " Hz";
            while ( *suffix ) buff[len++] = *suffix++;
        }
        buff[len] = '\0';
        return len;
    }

    // Resonance: display as percentage
    if ( param == kParamResonance )
    {
        float pct = (float)val * 0.1f;
        int len = NT_floatToString( buff, pct, 1 );
        buff[len++] = '%';
        buff[len] = '\0';
        return len;
    }

    return 0;
}

// --- Factory ---

static const _NT_factory factory = {
    .guid = NT_MULTICHAR('V', 't', 'x', 'S'),
    .name = "Vortex_stereo v" VORTEX_VERSION,
    .description = "Unofficial stereo adaptation of Vortex by wintocode, v" VORTEX_VERSION,
    .numSpecifications = 0,
    .specifications = NULL,
    .calculateStaticRequirements = NULL,
    .initialise = NULL,
    .calculateRequirements = calculateRequirements,
    .construct = construct,
    .parameterChanged = parameterChanged,
    .step = step,
    .draw = NULL,
    .midiRealtime = NULL,
    .midiMessage = NULL,
    .tags = kNT_tagEffect | kNT_tagFilterEQ,
    .hasCustomUi = NULL,
    .customUi = NULL,
    .setupUi = NULL,
    .serialise = NULL,
    .deserialise = NULL,
    .midiSysEx = NULL,
    .parameterUiPrefix = NULL,
    .parameterString = parameterString,
};

// --- Entry point ---

extern "C"
uintptr_t pluginEntry( _NT_selector selector, uint32_t data )
{
    switch ( selector )
    {
    case kNT_selector_version:
        return kNT_apiVersionCurrent;
    case kNT_selector_numFactories:
        return 1;
    case kNT_selector_factoryInfo:
        return (uintptr_t)( ( data == 0 ) ? &factory : NULL );
    }
    return 0;
}
