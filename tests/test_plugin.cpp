#include <math.h>
#include <stdio.h>
#include <string.h>

#include <distingnt/api.h>

extern "C" {
extern const _NT_globals NT_globals = {
    48000,
    256,
    NULL,
    0,
    0,
    0
};

int NT_floatToString( char* buffer, float value, int decimalPlaces )
{
    return snprintf( buffer, kNT_parameterStringSize, "%.*f", decimalPlaces, value );
}
}

#define VORTEX_VERSION "host-test"
#include "../vortex_stereo.cpp"

static int failures = 0;

#define CHECK(condition) do { \
    if ( !(condition) ) { \
        printf( "FAIL line %d: %s\n", __LINE__, #condition ); \
        ++failures; \
    } \
} while ( 0 )

struct TestInstance
{
    alignas(_vortexAlgorithm) unsigned char storage[ sizeof(_vortexAlgorithm) ];
    int16_t values[ kNumParams ];
    _vortexAlgorithm* algorithm;

    TestInstance()
    {
        for ( int i = 0; i < kNumParams; ++i )
            values[i] = parameters[i].def;

        _NT_algorithmMemoryPtrs memory = {};
        memory.sram = storage;
        _NT_algorithmRequirements requirements = {};
        calculateRequirements( requirements, NULL );
        algorithm = static_cast<_vortexAlgorithm*>( construct( memory, requirements, NULL ) );
        algorithm->v = values;

        parameterChanged( algorithm, kParamMode );
        parameterChanged( algorithm, kParamCutoff );
        parameterChanged( algorithm, kParamResonance );
        parameterChanged( algorithm, kParamDrive );
        parameterChanged( algorithm, kParamMix );
        parameterChanged( algorithm, kParamFMDepth );
        parameterChanged( algorithm, kParamChannelMode );
    }
};

static void test_parameter_layout()
{
    CHECK( kNumParams == 20 );
    CHECK( strcmp( parameters[kParamInputL].name, "Input L/Mono" ) == 0 );
    CHECK( parameters[kParamInputL].unit == kNT_unitAudioInput );
    CHECK( strcmp( parameters[kParamInputR].name, "Input R" ) == 0 );
    CHECK( parameters[kParamInputR].unit == kNT_unitAudioInput );
    CHECK( strcmp( parameters[kParamOutputL].name, "Output L/Mono" ) == 0 );
    CHECK( parameters[kParamOutputL].unit == kNT_unitAudioOutput );
    CHECK( strcmp( parameters[kParamOutputR].name, "Output R" ) == 0 );
    CHECK( parameters[kParamOutputR].unit == kNT_unitAudioOutput );
    CHECK( parameters[kParamOutputMode].unit == kNT_unitOutputMode );
    CHECK( strcmp( parameters[kParamFMDepth].name, "FM Depth" ) == 0 );
    CHECK( strcmp( parameters[kParamCVCutoffFM].name, "Cutoff FM CV" ) == 0 );
    CHECK( strcmp( factory.name, "Vortex_stereo vhost-test" ) == 0 );
    CHECK( strstr( factory.description, "Vortex by wintocode" ) != NULL );
}

static void test_stereo_dry_routing()
{
    const int frames = 64;
    TestInstance instance;
    instance.values[kParamChannelMode] = 1;
    instance.values[kParamInputL] = 1;
    instance.values[kParamInputR] = 2;
    instance.values[kParamOutputL] = 13;
    instance.values[kParamOutputR] = 14;
    instance.values[kParamOutputMode] = 1;
    instance.values[kParamMix] = 0;
    parameterChanged( instance.algorithm, kParamChannelMode );
    parameterChanged( instance.algorithm, kParamMix );

    float buses[28 * frames] = {};
    for ( int i = 0; i < frames; ++i )
    {
        buses[i] = 0.25f + i * 0.001f;
        buses[frames + i] = -0.5f - i * 0.002f;
    }

    step( instance.algorithm, buses, frames / 4 );

    for ( int i = 0; i < frames; ++i )
    {
        CHECK( buses[12 * frames + i] == 0.25f + i * 0.001f );
        CHECK( buses[13 * frames + i] == -0.5f - i * 0.002f );
    }
}

static void test_stereo_wet_isolation()
{
    const int frames = 256;
    TestInstance instance;
    instance.values[kParamChannelMode] = 1;
    instance.values[kParamInputL] = 1;
    instance.values[kParamInputR] = 2;
    instance.values[kParamOutputL] = 13;
    instance.values[kParamOutputR] = 14;
    instance.values[kParamOutputMode] = 1;
    instance.values[kParamMix] = 1000;
    parameterChanged( instance.algorithm, kParamChannelMode );
    parameterChanged( instance.algorithm, kParamMix );

    float buses[28 * frames] = {};
    for ( int i = 0; i < frames; ++i )
        buses[i] = 1.0f;

    step( instance.algorithm, buses, frames / 4 );

    float leftEnergy = 0.0f;
    float rightEnergy = 0.0f;
    for ( int i = 0; i < frames; ++i )
    {
        leftEnergy += fabsf( buses[12 * frames + i] );
        rightEnergy += fabsf( buses[13 * frames + i] );
    }
    CHECK( leftEnergy > 1.0f );
    CHECK( rightEnergy == 0.0f );
}

static float render_with_fm_depth( int depth )
{
    const int frames = 256;
    TestInstance instance;
    instance.values[kParamChannelMode] = 0;
    instance.values[kParamInputL] = 1;
    instance.values[kParamOutputL] = 13;
    instance.values[kParamOutputMode] = 1;
    instance.values[kParamCVCutoffFM] = 3;
    instance.values[kParamFMDepth] = depth;
    parameterChanged( instance.algorithm, kParamFMDepth );

    float buses[28 * frames] = {};
    for ( int i = 0; i < frames; ++i )
    {
        buses[i] = i == 0 ? 1.0f : 0.0f;
        buses[2 * frames + i] = 1.0f;
    }

    step( instance.algorithm, buses, frames / 4 );

    float weightedOutput = 0.0f;
    for ( int i = 0; i < frames; ++i )
        weightedOutput += buses[12 * frames + i] * (float)( i + 1 );
    return weightedOutput;
}

static void test_fm_depth_controls_cutoff_fm_cv()
{
    float zeroDepth = render_with_fm_depth( 0 );
    float fullDepth = render_with_fm_depth( 1000 );
    CHECK( fabsf( zeroDepth - fullDepth ) > 0.01f );
}

int main()
{
    test_parameter_layout();
    test_stereo_dry_routing();
    test_stereo_wet_isolation();
    test_fm_depth_controls_cutoff_fm_cv();

    if ( failures == 0 )
        printf( "Vortex host routing tests passed\n" );
    else
        printf( "%d Vortex host routing checks failed\n", failures );
    return failures == 0 ? 0 : 1;
}
