/* GIN prepared-voice adapter host checks. Copyright (C) 2026 Squarewave Studio.
 * GPL-3.0-or-later with OP-1 permission. */
#ifdef NDEBUG
#error Assertions_required
#endif
#include "gin_voice.h"
#include "gin_prepare.h"
#include "gin_factory.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static gin_plugin_voices bank, saved;
static gin_preset_owned_t kit;
static uint32_t receivers[12];
static op1_drum_synth_voice_t context(unsigned voice,unsigned slot,unsigned key)
{
    op1_drum_synth_voice_t c = {&receivers[voice],1,voice,slot,key,127}; return c;
}
static unsigned comparisons;
int main(void)
{
    op1_ds_env_prepared bypass;
    op1_ds_env_settings settings = {0};
    assert(!op1_ds_env_prepare(&bypass,&settings));
    for(unsigned k=0;k<8;k++) {
        assert(!gin_factory_load_index(k,&kit));
        assert(kit.count==24);
        for(unsigned pad=0;pad<24;pad++) for(unsigned side=0;side<3;side++) {
            gin_prepared_t p;po32_patch_params_t patch;
            assert(!gin_integer_prepare_morph(&p,&patch,kit.sounds[pad].endpoint,side*500));
            gin_integer_voice_t reference;gin_resample_2x_state rs;
            assert(!gin_integer_trigger(&reference,&p));gin_resample_2x_reset(&rs);
            gin_plugin_voices_init(&bank);
            op1_drum_synth_voice_t c=context(0,0,pad);uint32_t token;
            assert(!gin_plugin_voice_begin(&bank,&c,&p,&bypass,0,&token));
            for(unsigned block=0;block<16;block++) {
                int16_t core[64], expected[128],actual[130];
                assert(!gin_integer_render64(&reference,core));
                gin_resample_2x_process64(&rs,core,expected);
                actual[0]=12345;actual[129]=-12345;
                assert(!gin_plugin_voice_render(&bank,&c,token,actual+1));
                assert(actual[0]==12345 && actual[129]==-12345);
                assert(!memcmp(actual+1,expected,sizeof expected));comparisons++;
            }
        }
    }
    gin_prepared_t p;assert(!gin_integer_prepare(&p,&tonic_kick,127,0.3f));
    gin_plugin_voices_init(&bank);
    op1_drum_synth_voice_t a=context(0,0,0), b=context(1,0,1), other=context(2,1,0);
    uint32_t ta,tb,to,newer;int16_t out[128];
    assert(!gin_plugin_voice_begin(&bank,&a,&p,&bypass,1,&ta));
    assert(!gin_plugin_voice_render(&bank,&a,ta,out));
    gin_integer_voice_t held=bank.voice[0].dsp;
    gin_prepared_t edited=p;edited.phase_step+=12345;
    assert(!gin_plugin_voice_begin(&bank,&b,&edited,&bypass,0,&tb));
    assert(!memcmp(&held,&bank.voice[0].dsp,sizeof held));
    assert(!gin_plugin_voice_release(&bank,&a,ta)); /* bypass one-shot remains */
    assert(gin_plugin_voice_active(&bank,&a,ta));
    assert(gin_plugin_voice_active(&bank,&b,tb));
    assert(!gin_plugin_voice_begin(&bank,&other,&p,&bypass,1,&to));
    assert(gin_plugin_voice_active(&bank,&a,ta)); /* different slot unaffected */
    assert(!gin_plugin_voice_begin(&bank,&a,&p,&bypass,1,&newer));
    assert(newer!=ta);
    saved=bank;
    assert(gin_plugin_voice_release(&bank,&a,ta)==GIN_PLUGIN_STALE);
    assert(gin_plugin_voice_render(&bank,&a,ta,out)==GIN_PLUGIN_STALE);
    for(unsigned i=0;i<128;i++)assert(!out[i]);
    assert(!memcmp(&saved,&bank,sizeof bank));
    op1_drum_synth_voice_t wrong=a;wrong.incarnation++;
    assert(gin_plugin_voice_release(&bank,&wrong,newer)==GIN_PLUGIN_STALE);
    assert(!memcmp(&saved,&bank,sizeof bank));
    uint32_t untouched=123;gin_prepared_t bad=p;bad.magic=0;
    assert(gin_plugin_voice_begin(&bank,&a,&bad,&bypass,0,&untouched)==GIN_PLUGIN_ARGUMENT);
    assert(untouched==123 && !memcmp(&saved,&bank,sizeof bank));
    gin_plugin_voices_retire(&bank,0);
    assert(!gin_plugin_voice_active(&bank,&a,newer));
    assert(!gin_plugin_voice_active(&bank,&b,tb));
    assert(gin_plugin_voice_active(&bank,&other,to));
    assert(!gin_plugin_voice_render(&bank,&a,newer,out));
    for(unsigned i=0;i<128;i++)assert(!out[i]);
    /* Same group actually stops synthesis and drains a finite PCM fade. */
    gin_plugin_voices_init(&bank);
    assert(!gin_plugin_voice_begin(&bank,&a,&p,&bypass,2,&ta));
    assert(!gin_plugin_voice_render(&bank,&a,ta,out));
    assert(!gin_plugin_voice_begin(&bank,&b,&p,&bypass,2,&tb));
    assert(!bank.voice[0].dsp.active && bank.voice[1].dsp.active);
    assert(!gin_plugin_voice_render(&bank,&a,ta,out));
    assert(!gin_plugin_voice_active(&bank,&a,ta));
    assert(gin_plugin_voice_active(&bank,&b,tb));
    /* A previously self-choked tail must not restart on a later group hit. */
    gin_plugin_voices_init(&bank);
    assert(!gin_plugin_voice_begin(&bank,&a,&p,&bypass,3,&ta));
    assert(!gin_plugin_voice_render(&bank,&a,ta,out));
    op1_drum_synth_voice_t same=b;same.key=a.key;
    assert(!gin_plugin_voice_begin(&bank,&same,&p,&bypass,0,&tb));
    uint32_t remaining=bank.voice[0].fade_left;
    int32_t tail=bank.voice[0].fade_sample;
    op1_drum_synth_voice_t third=context(3,0,2);uint32_t tt;
    assert(!gin_plugin_voice_begin(&bank,&third,&p,&bypass,3,&tt));
    assert(bank.voice[0].fade_left==remaining && bank.voice[0].fade_sample==tail);
    assert(!bank.voice[0].dsp.active && bank.voice[3].dsp.active);
    /* Gate/release settings are independently snapshotted. */
    settings=(op1_ds_env_settings){0,44100,44100,8,1,1};
    op1_ds_env_prepared gated;assert(!op1_ds_env_prepare(&gated,&settings));
    gin_plugin_voices_init(&bank);
    assert(!gin_plugin_voice_begin(&bank,&a,&p,&gated,0,&ta));
    assert(!gin_plugin_voice_render(&bank,&a,ta,out));
    gated.settings.release=44100; /* edit future settings only */
    assert(!gin_plugin_voice_release(&bank,&a,ta));
    for(unsigned i=0;i<3;i++)assert(!gin_plugin_voice_render(&bank,&a,ta,out));
    assert(!gin_plugin_voice_active(&bank,&a,ta));
    /* Natural finite source expiry doesn't inherit bypass's active state. */
    gin_plugin_voices_init(&bank);p.total=65;
    assert(!gin_plugin_voice_begin(&bank,&a,&p,&bypass,0,&ta));
    for(unsigned i=0;i<4;i++)assert(!gin_plugin_voice_render(&bank,&a,ta,out));
    assert(!gin_plugin_voice_active(&bank,&a,ta));
    for(unsigned i=0;i<128;i++)assert(!out[i]);
    /* Four DSP voices, independent from native receiver capacity. */
    gin_plugin_voices_init(&bank);p.total=13230;
    for(unsigned i=0;i<5;i++) {
        op1_drum_synth_voice_t c=context(i,0,i);uint32_t t;
        assert(!gin_plugin_voice_begin(&bank,&c,&p,&bypass,0,&t));
        assert(!gin_plugin_voice_render(&bank,&c,t,out));
    }
    unsigned full=0;for(unsigned i=0;i<12;i++)full+=bank.voice[i].dsp.active!=0;
    assert(full==4 && !bank.voice[0].dsp.active);
    /* Four voices in another slot cannot be stolen or exceeded. */
    gin_plugin_voices_init(&bank);
    for(unsigned i=0;i<4;i++) {
        op1_drum_synth_voice_t c=context(i,1,i);uint32_t t;
        assert(!gin_plugin_voice_begin(&bank,&c,&p,&bypass,0,&t));
    }
    saved=bank;
    op1_drum_synth_voice_t fifth=context(4,0,4);
    assert(gin_plugin_voice_begin(&bank,&fifth,&p,&bypass,0,&untouched)==GIN_PLUGIN_CAPACITY);
    assert(!memcmp(&saved,&bank,sizeof bank));
    /* Reusing an occupied receiver index cannot erase another slot's hit. */
    untouched=123;
    op1_drum_synth_voice_t reused=context(0,0,5);
    assert(gin_plugin_voice_begin(&bank,&reused,&p,&bypass,0,&untouched)==GIN_PLUGIN_CAPACITY);
    assert(untouched==123 && !memcmp(&saved,&bank,sizeof bank));
    /* The same rule applies below the four-DSP limit and to remaining tails. */
    gin_plugin_voices_init(&bank);
    op1_drum_synth_voice_t owner=context(0,1,0);
    assert(!gin_plugin_voice_begin(&bank,&owner,&p,&bypass,0,&to));
    saved=bank;
    assert(gin_plugin_voice_begin(&bank,&reused,&p,&bypass,0,&untouched)==GIN_PLUGIN_CAPACITY);
    assert(untouched==123 && !memcmp(&saved,&bank,sizeof bank));
    bank.voice[0].dsp.active=0;bank.voice[0].drain=1;saved=bank;
    assert(gin_plugin_voice_begin(&bank,&reused,&p,&bypass,0,&untouched)==GIN_PLUGIN_CAPACITY);
    assert(untouched==123 && !memcmp(&saved,&bank,sizeof bank));
    bank.voice[0].drain=0;bank.voice[0].fade_left=1;saved=bank;
    assert(gin_plugin_voice_begin(&bank,&reused,&p,&bypass,0,&untouched)==GIN_PLUGIN_CAPACITY);
    assert(untouched==123 && !memcmp(&saved,&bank,sizeof bank));
    gin_plugin_voices_retire(&bank,1);
    assert(!gin_plugin_voice_begin(&bank,&reused,&p,&bypass,0,&untouched));
    assert(gin_plugin_voice_active(&bank,&reused,untouched));
    bank.choke.serial=UINT32_MAX;saved=bank;
    assert(gin_plugin_voice_begin(&bank,&a,&p,&bypass,0,&untouched)==GIN_PLUGIN_EXHAUSTED);
    assert(!memcmp(&saved,&bank,sizeof bank));
    printf("GIN real-DSP adapter: %u exact PCM blocks, 8 kits x 24 keys x A/mid/B; overlap/retrigger/stale-release/gate/choke/retire tests PASS\n",comparisons);
    return 0;
}
